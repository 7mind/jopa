#pragma once

#include "platform.h"
#include "access.h"
#include "tuple.h"
#include "long.h"
#include "double.h"


namespace Jopa { // Open namespace Jopa block
//
// This header defines the .class file format, for both reading in and writing
// out valid classfiles.
//
class ClassFile;
class ConstantPool;
class Control;
class TypeSymbol;


//
// The prefix CP stands for Constant Pool. This is the base class of all
// constant pool information; subclasses give more detail. JVMS 4.4.
//
class CPInfo
{
public:
    enum ConstantPoolTag
    {
        CONSTANT_Utf8 = 1,
        CONSTANT_Integer = 3,
        CONSTANT_Float = 4,
        CONSTANT_Long = 5,
        CONSTANT_Double = 6,
        CONSTANT_Class = 7,
        CONSTANT_String = 8,
        CONSTANT_Fieldref = 9,
        CONSTANT_Methodref = 10,
        CONSTANT_InterfaceMethodref = 11,
        CONSTANT_NameAndType = 12,
        CONSTANT_MethodHandle = 15,
        CONSTANT_MethodType = 16,
        CONSTANT_Dynamic = 17,
        CONSTANT_InvokeDynamic = 18,
        CONSTANT_Module = 19,
        CONSTANT_Package = 20
    };

protected:
    const ConstantPoolTag tag;

public:
    CPInfo(ConstantPoolTag _tag) : tag(_tag) {}
    virtual ~CPInfo() {}

    ConstantPoolTag Tag() const { return tag; }
    bool Large() const
    {
        return tag == CONSTANT_Long || tag == CONSTANT_Double;
    }
    bool Constant() const
    {
        return ((tag >= CPInfo::CONSTANT_Integer &&
                 tag <= CPInfo::CONSTANT_Double) ||
                tag == CPInfo::CONSTANT_String);
    }

    //
    // This reads the next constant pool entry from buffer, advancing it to
    // the end of the entry. The user is responsible for deleting the result.
    //
    static CPInfo* AllocateCPInfo(ClassFile&);

    //
    // Subclasses should override, to write an entry to a .class file.
    //
    virtual void Put(OutputBuffer& out) const
    {
        out.PutU1((u1) tag);
    }

    //
    // Allows subclasses to perform data validation, once the entire constant
    // pool has been read. Return false on failure.
    //
    virtual bool Check(const ConstantPool&) const { return true; }

#ifdef JOPA_DEBUG
    //
    // Prints all information about the constant value, ending the line.
    //
    virtual void Print(const ConstantPool&) const
    {
        Coutput << "unknown tag: " << (unsigned) tag << endl;
    }

    //
    // Prints shortened information, no newline.
    //
    virtual void Describe(const ConstantPool&) const
    {
        Coutput << "unknown tag: " << (unsigned) tag;
    }
#endif // JOPA_DEBUG
};


//
// Specialization of Tuple to more gracefully handle invalid indices. This
// is because slot 0 and slots following a long or double entry are invalid.
//
class ConstantPool : private Tuple<CPInfo*>
{
    class CPInvalid : public CPInfo
    {
    public:
        CPInvalid() : CPInfo((ConstantPoolTag) 0) {}
        ~CPInvalid() {}
        virtual void Put(OutputBuffer&) const
        {
            assert(false && "trying to put invalid ConstantPool entry");
        }
        virtual bool Check(const ConstantPool&) const { return false; }

#ifdef JOPA_DEBUG
        virtual void Print(const ConstantPool&) const
        {
            Coutput << "invalid index" << endl;
        }
        virtual void Describe(const ConstantPool&) const
        {
            Coutput << "(invalid)";
        }
#endif // JOPA_DEBUG
    } invalid;

public:
    ConstantPool(unsigned estimate = 0)
        : Tuple<CPInfo*>(estimate)
    {
        Next() = NULL; // The 0th entry is unused.
    }
    ConstantPool(unsigned log_blksize, unsigned base_increment)
        : Tuple<CPInfo*>(log_blksize, base_increment)
    {
        Next() = NULL; // The 0th entry is unused.
    }
    ~ConstantPool() {}

    // Inherit only the methods we know are safe.
    using Tuple<CPInfo*>::Length;
    using Tuple<CPInfo*>::SpaceAllocated;
    using Tuple<CPInfo*>::SpaceUsed;

    //
    // Note that unlike in Tuple<>, ConstantPool[] does not return an lvalue;
    // use SetNext(CPInfo*) instead.
    //
    inline const CPInfo* operator[](const unsigned i) const
    {
        if (i >= top)
            return &invalid;
        const CPInfo* result = Tuple<CPInfo*>::operator[](i);
        return result ? result : &invalid;
    }

    inline void Reset() { Tuple<CPInfo*>::Reset(); }

    void SetNext(CPInfo*);

    inline bool Valid(const unsigned i)
    {
        return i < top && Tuple<CPInfo*>::operator[](i);
    }
    bool Check() const;
};


//
// Describes a UTF8 sequence. JVMS 4.4.7.
//
class CPUtf8Info : public CPInfo
{
    // u1 tag; // inherited from CPInfo
    const u2 len;
    u1* bytes; // bytes[length]

    bool valid;

public:
    CPUtf8Info(const char* _bytes, unsigned length)
        : CPInfo(CONSTANT_Utf8)
        , len(length)
        , valid(true)
    {
        bytes = new u1[len + 1];
        for (unsigned i = 0; i < len; i++)
            bytes[i] = (u1) _bytes[i];
        bytes[len] = U_NULL;
        Init(len);
        assert(valid && length <= 0xffff);
    }
    CPUtf8Info(ClassFile&);
    virtual ~CPUtf8Info()
    {
        delete [] bytes;
    }

    //
    // If this UTF8 was valid, then Bytes returns a NULL-terminated string
    // that can be passed to strcmp() and friends, since Java's modified UTF8
    // encoding ensures there are no embedded \0. However, since we already
    // know Length(), calling strlen() on the result is inefficient.
    //
    const char* Bytes() const { return (const char*) bytes; }
    u2 Length() const { return len; }

    virtual void Put(OutputBuffer& out) const
    {
        CPInfo::Put(out);
        out.PutU2(len);
        out.PutN(bytes, len);
    }

    virtual bool Check(const ConstantPool&) const { return valid; }

private:
    void Init(u2);

#ifdef JOPA_DEBUG
    //
    // When debugging, contents represents the String literal (less the
    // enclosing "") made of printing ASCII characters that will regenerate
    // the same sequence of bytes, provided the UTF8 was valid.  Otherwise it
    // contains some hints as to where the UTF8 went wrong. It is a
    // NULL-terminated sequence, with no embedded NULLs.
    //
    ConvertibleArray<char> contents;

public:
    virtual void Print(const ConstantPool&) const
    {
        Coutput << "Utf8: length: " << (unsigned) len
                << ", bytes[]: " << contents.Array() << endl;
    }

    virtual void Describe(const ConstantPool&) const
    {
        Coutput << contents.Array();
    }
#endif // JOPA_DEBUG
};


//
// Describes a class or interface name. JVMS 4.4.1.
//
class CPClassInfo : public CPInfo
{
    // u1 tag; // inherited from CPInfo
    const u2 name_index;

    TypeSymbol* type; // the associated type

public:
    CPClassInfo(u2 _name_index)
        : CPInfo(CONSTANT_Class)
        , name_index(_name_index)
        , type(NULL)
    {}
    CPClassInfo(ClassFile&);
    virtual ~CPClassInfo() {}

    virtual void Put(OutputBuffer& out) const
    {
        CPInfo::Put(out);
        out.PutU2(name_index);
    }

    virtual bool Check(const ConstantPool& constant_pool) const
    {
        return constant_pool[name_index] -> Tag() == CONSTANT_Utf8;
    }

    const char* TypeName(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[name_index] -> Tag() == CONSTANT_Utf8);
        return ((const CPUtf8Info*) constant_pool[name_index]) -> Bytes();
    }
    u2 TypeNameLength(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[name_index] -> Tag() == CONSTANT_Utf8);
        return ((const CPUtf8Info*) constant_pool[name_index]) -> Length();
    }

    void SetType(TypeSymbol* t)
    {
        assert(! type);
        type = t;
    }
    TypeSymbol* Type() { return type; }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool) const
    {
        Coutput << "Class: name: " << (unsigned) name_index;
        if (constant_pool[name_index] -> Tag() == CONSTANT_Utf8)
        {
            Coutput << "=\"";
            constant_pool[name_index] -> Describe(constant_pool);
            Coutput << '"';
        }
        else Coutput << "(invalid)";
        Coutput << endl;
    }

    virtual void Describe(const ConstantPool& constant_pool) const
    {
        if (constant_pool[name_index] -> Tag() == CONSTANT_Utf8)
            constant_pool[name_index] -> Describe(constant_pool);
        else Coutput << "(invalid)";
    }
#endif // JOPA_DEBUG
};


//
// Describes a field or method's declaring class, name, and type. JVMS 4.4.2.
//
class CPMemberInfo : public CPInfo
{
    // u1 tag; // inherited from CPInfo
    const u2 class_index;
    const u2 name_and_type_index;

public:
    CPMemberInfo(ConstantPoolTag _tag, u2 _class_index,
                 u2 _name_and_type_index)
        : CPInfo(_tag)
        , class_index(_class_index)
        , name_and_type_index(_name_and_type_index)
    {}
    CPMemberInfo(ConstantPoolTag, ClassFile&);
    virtual ~CPMemberInfo() {}

    virtual void Put(OutputBuffer& out) const
    {
        CPInfo::Put(out);
        out.PutU2(class_index);
        out.PutU2(name_and_type_index);
    }

    virtual bool Check(const ConstantPool& constant_pool) const
    {
        return constant_pool[class_index] -> Tag() == CONSTANT_Class &&
            (constant_pool[name_and_type_index] -> Tag() ==
             CONSTANT_NameAndType);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool) const
    {
        switch (tag)
        {
        case CONSTANT_Fieldref:
            Coutput << "Field";
            break;
        case CONSTANT_InterfaceMethodref:
            Coutput << "Interface";
            // fallthrough
        case CONSTANT_Methodref:
            Coutput << "Method";
            break;
        default:
            assert(false && "bad constant pool member tag");
        }
        Coutput << ": class: " << (unsigned) class_index;
        if (constant_pool[class_index] -> Tag() == CONSTANT_Class)
        {
            Coutput << '=';
            constant_pool[class_index] -> Describe(constant_pool);
        }
        else Coutput << "(invalid)";
        Coutput << endl << "        name_and_type: "
                << (unsigned) name_and_type_index;
        if (constant_pool[name_and_type_index] -> Tag() ==
            CONSTANT_NameAndType)
        {
            Coutput << "=<";
            constant_pool[name_and_type_index] -> Describe(constant_pool);
            Coutput << ">";
        }
        else Coutput << "(invalid)";
        Coutput << endl;
    }

    virtual void Describe(const ConstantPool& constant_pool) const
    {
        if (constant_pool[class_index] -> Tag() == CONSTANT_Class)
            constant_pool[class_index] -> Describe(constant_pool);
        else Coutput << "(invalid)";
        Coutput << ".";
        if (constant_pool[name_and_type_index] -> Tag() ==
            CONSTANT_NameAndType)
        {
            constant_pool[name_and_type_index] -> Describe(constant_pool);
        }
        else Coutput << "(invalid)";
    }
#endif // JOPA_DEBUG
};


//
// Describes a String literal. JVMS 4.4.3.
//
class CPStringInfo : public CPInfo
{
    // u1 tag; // inherited from CPInfo
    const u2 string_index;

public:
    CPStringInfo(u2 _string_index)
        : CPInfo(CONSTANT_String)
        , string_index(_string_index)
    {}
    CPStringInfo(ClassFile&);
    virtual ~CPStringInfo() {}

    virtual void Put(OutputBuffer& out) const
    {
        CPInfo::Put(out);
        out.PutU2(string_index);
    }

    virtual bool Check(const ConstantPool& constant_pool) const
    {
        return constant_pool[string_index] -> Tag() == CONSTANT_Utf8;
    }

    const char* Bytes(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[string_index] -> Tag() == CONSTANT_Utf8);
        return ((const CPUtf8Info*) constant_pool[string_index]) -> Bytes();
    }
    u2 Length(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[string_index] -> Tag() == CONSTANT_Utf8);
        return ((const CPUtf8Info*) constant_pool[string_index]) -> Length();
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool) const
    {
        Coutput << "String: string: " << (unsigned) string_index;
        if (constant_pool[string_index] -> Tag() == CONSTANT_Utf8)
        {
            Coutput << "=\"";
            constant_pool[string_index] -> Describe(constant_pool);
            Coutput << '"';
        }
        else Coutput << "(invalid)";
        Coutput << endl;
    }

    virtual void Describe(const ConstantPool& constant_pool) const
    {
        if (constant_pool[string_index] -> Tag() == CONSTANT_Utf8)
        {
            Coutput << '"';
            constant_pool[string_index] -> Describe(constant_pool);
            Coutput << '"';
        }
        else Coutput << "(invalid)";
    }
#endif // JOPA_DEBUG
};


//
// Describes an int literal. JVMS 4.4.4.
//
class CPIntegerInfo : public CPInfo
{
    // u1 tag; // inherited from CPInfo
    const u4 bytes;

public:
    CPIntegerInfo(u4 _bytes)
        : CPInfo(CONSTANT_Integer)
        , bytes(_bytes)
    {}
    CPIntegerInfo(ClassFile&);
    virtual ~CPIntegerInfo() {}

    virtual void Put(OutputBuffer& out) const
    {
        CPInfo::Put(out);
        out.PutU4(bytes);
    }

    i4 Value() const { return (i4) bytes; }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool&) const
    {
        Coutput << "Integer: bytes: 0x"
                << IntToString(bytes, 8).String() << " (" << (int) bytes
                << ')' << endl;
    }

    virtual void Describe(const ConstantPool&) const
    {
        Coutput << (int) bytes;
    }
#endif // JOPA_DEBUG
};


//
// Describes a float literal. JVMS 4.4.4.
//
class CPFloatInfo : public CPInfo
{
    // u1 tag; // inherited from CPInfo
    const IEEEfloat value; // u4 bytes

public:
    CPFloatInfo(u4 bytes)
        : CPInfo(CONSTANT_Float)
        , value(bytes)
    {}
    CPFloatInfo(ClassFile&);
    virtual ~CPFloatInfo() {}

    virtual void Put(OutputBuffer& out) const
    {
        CPInfo::Put(out);
        out.PutU4(value.Word());
    }

    const IEEEfloat& Value() const { return value; }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool&) const
    {
        Coutput << "Float: bytes: 0x"
                << IntToString(value.Word(), 8).String() << " ("
                << FloatToString(value).String() << "F)" << endl;
    }

    virtual void Describe(const ConstantPool&) const
    {
        Coutput << FloatToString(value).String() << 'F';
    }
#endif // JOPA_DEBUG
};


//
// Describes a long literal. JVMS 4.4.5.
//
class CPLongInfo : public CPInfo
{
    // u1 tag; // inherited from CPInfo
    const LongInt value; // u4 high_bytes, low_bytes;

public:
    CPLongInfo(u4 high_bytes, u4 low_bytes)
        : CPInfo(CONSTANT_Long)
        , value(high_bytes, low_bytes)
    {}
    CPLongInfo(ClassFile&);
    virtual ~CPLongInfo() {}

    virtual void Put(OutputBuffer& out) const
    {
        CPInfo::Put(out);
        out.PutU4(value.HighWord());
        out.PutU4(value.LowWord());
    }

    const LongInt& Value() const { return value; }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool&) const
    {
        Coutput << "Long: high: 0x"
                << IntToString(value.HighWord(), 8).String()
                << ", low: 0x" << IntToString(value.LowWord(), 8).String()
                << " (" << LongToString(value).String() << "L)" << endl;
    }

    virtual void Describe(const ConstantPool&) const
    {
        Coutput << LongToString(value).String() << 'L';
    }
#endif // JOPA_DEBUG
};


//
// Describes a double literal. JVMS 4.4.5.
//
class CPDoubleInfo : public CPInfo
{
    // u1 tag; // inherited from CPInfo
    const IEEEdouble value; // u4 high_bytes, low_bytes;

public:
    CPDoubleInfo(u4 high_bytes, u4 low_bytes)
        : CPInfo(CONSTANT_Double)
        , value(high_bytes, low_bytes)
    {}
    CPDoubleInfo(ClassFile&);
    virtual ~CPDoubleInfo() {}

    virtual void Put(OutputBuffer& out) const
    {
        CPInfo::Put(out);
        out.PutU4(value.HighWord());
        out.PutU4(value.LowWord());
    }

    const IEEEdouble& Value() const { return value; }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool&) const
    {
        Coutput << "Double: high: 0x"
                << IntToString(value.HighWord(), 8).String()
                << ", low: 0x" << IntToString(value.LowWord(), 8).String()
                << " (" << DoubleToString(value).String() << ')' << endl;
    }

    virtual void Describe(const ConstantPool&) const
    {
        Coutput << DoubleToString(value).String();
    }
#endif // JOPA_DEBUG
};


//
// Describes a member name and its signature. JVMS 4.4.6.
//
class CPNameAndTypeInfo : public CPInfo
{
    // u1 tag; // inherited from CPInfo
    const u2 name_index;
    const u2 descriptor_index;

public:
    CPNameAndTypeInfo(u2 _name_index, u2 _descriptor_index)
        : CPInfo(CONSTANT_NameAndType)
        , name_index(_name_index)
        , descriptor_index(_descriptor_index)
    {}
    CPNameAndTypeInfo(ClassFile&);
    virtual ~CPNameAndTypeInfo() {}

    const char* Signature(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[descriptor_index] -> Tag() ==
               CPInfo::CONSTANT_Utf8);
        return ((const CPUtf8Info*) constant_pool[descriptor_index]) ->
            Bytes();
    }
    u2 SignatureLength(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[descriptor_index] -> Tag() ==
               CPInfo::CONSTANT_Utf8);
        return ((const CPUtf8Info*) constant_pool[descriptor_index]) ->
            Length();
    }

    virtual void Put(OutputBuffer& out) const
    {
        CPInfo::Put(out);
        out.PutU2(name_index);
        out.PutU2(descriptor_index);
    }

    virtual bool Check(const ConstantPool& constant_pool) const
    {
        return constant_pool[name_index] -> Tag() == CONSTANT_Utf8 &&
            constant_pool[descriptor_index] -> Tag() == CONSTANT_Utf8;
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool) const
    {
        Coutput << "NameAndType: name: "
                << (unsigned) name_index;
        if (constant_pool[name_index] -> Tag() == CONSTANT_Utf8)
        {
            Coutput << "=\"";
            constant_pool[name_index] -> Describe(constant_pool);
            Coutput << '"';
        }
        else Coutput << "(invalid)";
        Coutput << endl << "        type: " << (unsigned) descriptor_index;
        if (constant_pool[descriptor_index] -> Tag() == CONSTANT_Utf8)
        {
            Coutput << "=\"";
            constant_pool[descriptor_index] -> Describe(constant_pool);
            Coutput << '"';
        }
        else Coutput << "(invalid)";
        Coutput << endl;
    }

    virtual void Describe(const ConstantPool& constant_pool) const
    {
        if (constant_pool[name_index] -> Tag() == CONSTANT_Utf8)
            constant_pool[name_index] -> Describe(constant_pool);
        else Coutput << "(invalid)";
        Coutput << " ";
        if (constant_pool[descriptor_index] -> Tag() == CONSTANT_Utf8)
            constant_pool[descriptor_index] -> Describe(constant_pool);
        else Coutput << "(invalid)";
    }
#endif // JOPA_DEBUG
};


//
// FieldInfo and MethodInfo come next in the JVMS, but must appear after
// we know about attributes.
//
class FieldInfo;
class MethodInfo;

//
// Describes an arbitrary attribute. We subclass this for known attributes.
//
class AttributeInfo
{
public:
    enum AttributeInfoTag
    {
        ATTRIBUTE_ConstantValue,
        ATTRIBUTE_Code,
        ATTRIBUTE_Exceptions,
        ATTRIBUTE_InnerClasses,
        ATTRIBUTE_Synthetic,
        ATTRIBUTE_SourceFile,
        ATTRIBUTE_LineNumberTable,
        ATTRIBUTE_LocalVariableTable,
        ATTRIBUTE_Deprecated,
        ATTRIBUTE_Signature, // defined in JSR 14
        ATTRIBUTE_Bridge, // defined in JSR 14?
        ATTRIBUTE_EnclosingMethod, // defined in JSR 14
        ATTRIBUTE_LocalVariableTypeTable, // defined in JSR 14
        ATTRIBUTE_StackMap, // defined in JSR 139
        ATTRIBUTE_StackMapTable, // defined in JSR 202 (Java SE 6+)
        ATTRIBUTE_RuntimeVisibleAnnotations, // defined in JSR 175
        ATTRIBUTE_RuntimeInvisibleAnnotations, // defined in JSR 175
        ATTRIBUTE_RuntimeVisibleParameterAnnotations, // defined in JSR 175
        ATTRIBUTE_RuntimeInvisibleParameterAnnotations, // defined in JSR 175
        ATTRIBUTE_AnnotationDefault, // defined in JSR 175
        // all others are currently unknown to jikes
        ATTRIBUTE_Generic

    };

protected:
    const AttributeInfoTag tag;
    const u2 attribute_name_index;
    u4 attribute_length;

    AttributeInfo(AttributeInfoTag _tag, u2 _name_index, u4 length = 0)
        : tag(_tag)
        , attribute_name_index(_name_index)
        , attribute_length(length)
    {
        assert(tag != ATTRIBUTE_Generic);
    }
    AttributeInfo(AttributeInfoTag, ClassFile&);
public:
    virtual ~AttributeInfo() {}

    AttributeInfoTag Tag() const { return tag; }
    u2 AttributeNameIndex() const { return attribute_name_index; }
    u4 AttributeLength() const { return attribute_length; }

    static AttributeInfoTag Tag(const CPUtf8Info* name);

    //
    // This reads the next attribute from the buffer, advancing it to the
    // end of the attribute. The user is responsible for deleting the result.
    //
    static AttributeInfo* AllocateAttributeInfo(ClassFile&);

    // Subclasses must override if attribute_length != 0.
    virtual void Put(OutputBuffer& out) const
    {
        assert(tag != ATTRIBUTE_Generic);
        assert(attribute_name_index);
        out.PutU2(attribute_name_index);
        out.PutU4(attribute_length);
    }

#ifdef JOPA_DEBUG
    void PrintPrefix(const char* category, const ConstantPool& constant_pool,
                     int fill) const
    {
        Coutput.width(fill);
        Coutput << "" << category << ": name: "
                << (unsigned) attribute_name_index;
        if (constant_pool[attribute_name_index] -> Tag() ==
            CPInfo::CONSTANT_Utf8)
        {
            Coutput << "=\"";
            constant_pool[attribute_name_index] -> Describe(constant_pool);
            Coutput << '"';
        }
        else Coutput << "(invalid)";
        Coutput << ", length: " << (unsigned) attribute_length;
    }

    virtual void Print(const ConstantPool& constant_pool,
                       int fill = 0) const = 0;
#endif // JOPA_DEBUG
};


//
// An unknown attribute - we are allowed to read them from existing .class
// files, but should never generate or write one.
//
class UnknownAttribute : public AttributeInfo
{
    u1* info; // u1 info[attribute_length];
    u4 info_length; // in corrupted file, info_length < attribute_length

public:
    UnknownAttribute(ClassFile&);
    virtual ~UnknownAttribute() { delete [] info; }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill) const
    {
        PrintPrefix("Unrecognized attribute", constant_pool, fill);
        Coutput << endl;
        Coutput.width(fill);
        Coutput << "" << " info: " << (unsigned) info_length << ' ';
        for (unsigned i = 0; i < info_length; i++)
            if (info[i] <= 0x20 || info[i] >= 0x7f)
                Coutput << "\\x" << IntToString(info[i], 2).String();
            else Coutput << (char) info[i];
        Coutput << endl;
    }
#endif // JOPA_DEBUG
};


//
// Valid only on final fields, this marks the initial value of a constant
// variable. JVMS 4.7.2.
//
class ConstantValueAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    const u2 constantvalue_index;

public:
    ConstantValueAttribute(u2 _name_index, u2 _constantvalue_index)
        : AttributeInfo(ATTRIBUTE_ConstantValue, _name_index, 2)
        , constantvalue_index(_constantvalue_index)
    {}
    ConstantValueAttribute(ClassFile&);
    virtual ~ConstantValueAttribute() {}

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU2(constantvalue_index);
    }

    const CPInfo* Value(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[constantvalue_index] -> Constant());
        return constant_pool[constantvalue_index];
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix("ConstantValue", constant_pool, 0);
        Coutput << ", value: " << (unsigned) constantvalue_index << " (";
        if (constant_pool[constantvalue_index] -> Constant())
        {
            constant_pool[constantvalue_index] -> Describe(constant_pool);
        }
        else Coutput << "invalid";
        Coutput << ')' << endl;
    }
#endif // JOPA_DEBUG
};


//
// Valid only on methods, this is the throws clause. JVMS 4.7.4.
//
class ExceptionsAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    // u2 number_of_exceptions; // computed as exception_index_table.Length() 
    // exception_index_table[number_of_exceptions]
    Tuple<u2> exception_index_table;

public:
    ExceptionsAttribute(u2 _name_index)
        : AttributeInfo(ATTRIBUTE_Exceptions, _name_index, 2)
        // +2 for number_of_exceptions
        , exception_index_table(6, 16)
    {}
    ExceptionsAttribute(ClassFile&);
    virtual ~ExceptionsAttribute() {}

    u2 NumberOfExceptions() const { return exception_index_table.Length(); }
    const CPClassInfo* Exception(u2 i, const ConstantPool& constant_pool) const
    {
        assert(constant_pool[exception_index_table[i]] -> Tag() ==
               CPInfo::CONSTANT_Class);
        return (const CPClassInfo*) constant_pool[exception_index_table[i]];
    }

    void AddExceptionIndex(u2 index)
    {
        attribute_length += 2;
        exception_index_table.Next() = index;
    }

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU2(exception_index_table.Length());
        for (unsigned i = 0; i < exception_index_table.Length(); i++)
            out.PutU2(exception_index_table[i]);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix("Exceptions", constant_pool, 0);
        Coutput << ", count: " << (unsigned) exception_index_table.Length()
                << endl;
        for (unsigned i = 0; i < exception_index_table.Length(); i++)
        {
            Coutput << "  " << (unsigned) exception_index_table[i];
            if (constant_pool[exception_index_table[i]] -> Tag() ==
                CPInfo::CONSTANT_Class)
            {
                Coutput << '=';
                constant_pool[exception_index_table[i]] ->
                    Describe(constant_pool);
            }
            else Coutput << "(invalid)";                    
            Coutput << endl;
        }
    }
#endif // JOPA_DEBUG
};


//
// Valid only on classes, this lists information about all nested classes
// mentioned in the constant pool. JVMS 4.7.5.
//
class InnerClassesAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    // u2 number_of_classes; // computed as classes.Length()
    struct InnerClassesElement
    {
        u2 inner_class_info_index;
        u2 outer_class_info_index;
        u2 inner_name_index;
        AccessFlags inner_class_access_flags; // u2 inner_class_access_flags;
    };
    Tuple<InnerClassesElement> classes; // classes[number_of_classes]

public:
    InnerClassesAttribute(u2 _name_index)
        : AttributeInfo(ATTRIBUTE_InnerClasses, _name_index, 2)
        // +2 for number_of_classes
        , classes(6, 16)
    {}
    InnerClassesAttribute(ClassFile&);
    virtual ~InnerClassesAttribute() {}

    u2 InnerClassesLength() const { return classes.Length();}

    void AddInnerClass(u2 inner_class_info_index, u2 outer_class_info_index,
                       u2 inner_name_index, AccessFlags inner_class_flags)
    {
        attribute_length += 8;
        InnerClassesElement& entry = classes.Next();
        entry.inner_class_info_index = inner_class_info_index;
        entry.outer_class_info_index = outer_class_info_index;
        entry.inner_name_index = inner_name_index;
        entry.inner_class_access_flags.SetFlags(inner_class_flags);
    }

    const CPClassInfo* Inner(u2 i, const ConstantPool& constant_pool) const
    {
        assert(constant_pool[classes[i].inner_class_info_index] -> Tag() ==
               CPInfo::CONSTANT_Class);
        return (const CPClassInfo*)
            constant_pool[classes[i].inner_class_info_index];
    }
    const CPClassInfo* Outer(u2 i, const ConstantPool& constant_pool) const
    {
        assert(! classes[i].outer_class_info_index ||
               (constant_pool[classes[i].outer_class_info_index] -> Tag() ==
                CPInfo::CONSTANT_Class));
        return classes[i].outer_class_info_index
            ? ((const CPClassInfo*)
               constant_pool[classes[i].outer_class_info_index])
            : (const CPClassInfo*) NULL;
    }
    const char* Name(u2 i, const ConstantPool& constant_pool) const
    {
        assert(! classes[i].inner_name_index ||
               (constant_pool[classes[i].inner_name_index] -> Tag() ==
                CPInfo::CONSTANT_Utf8));
        if (! classes[i].inner_name_index)
            return NULL;
        return ((const CPUtf8Info*)
                constant_pool[classes[i].inner_name_index]) -> Bytes();
    }
    u2 NameLength(u2 i, const ConstantPool& constant_pool) const
    {
        assert(! classes[i].inner_name_index ||
               (constant_pool[classes[i].inner_name_index] -> Tag() ==
                CPInfo::CONSTANT_Utf8));
        return classes[i].inner_name_index
            ? ((const CPUtf8Info*)
               constant_pool[classes[i].inner_name_index]) -> Length() : 0;
    }
    const AccessFlags& Flags(u2 i) const
    {
        return classes[i].inner_class_access_flags;
    }

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU2(classes.Length());
        for (unsigned i = 0; i < classes.Length(); i++)
        {
            out.PutU2(classes[i].inner_class_info_index);
            out.PutU2(classes[i].outer_class_info_index);
            out.PutU2(classes[i].inner_name_index);
            out.PutU2(classes[i].inner_class_access_flags.Flags());
        }
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix("InnerClasses", constant_pool, 0);
        Coutput << ", count: " << (unsigned) classes.Length() << endl;
        for (unsigned i = 0; i < classes.Length(); i++)
        {
            Coutput << "  inner: "
                    << (unsigned) classes[i].inner_class_info_index;
            if (constant_pool[classes[i].inner_class_info_index] -> Tag() ==
                CPInfo::CONSTANT_Class)
            {
                Coutput << '=';
                constant_pool[classes[i].inner_class_info_index] ->
                    Describe(constant_pool);
            }
            else Coutput << "(invalid)";
            Coutput << ", outer: "
                    << (unsigned) classes[i].outer_class_info_index;
            if (! classes[i].outer_class_info_index)
                Coutput << "(inner is local)";
            else if (constant_pool[classes[i].outer_class_info_index] ->
                     Tag() == CPInfo::CONSTANT_Class)
            {
                Coutput << '=';
                constant_pool[classes[i].outer_class_info_index] ->
                    Describe(constant_pool);
            }
            else Coutput << "(invalid)";
            Coutput << endl << "   name: "
                    << (unsigned) classes[i].inner_name_index;
            if (! classes[i].inner_name_index)
                Coutput << "(anonymous)";
            else if (constant_pool[classes[i].inner_name_index] -> Tag() ==
                     CPInfo::CONSTANT_Utf8)
            {
                Coutput << '=';
                constant_pool[classes[i].inner_name_index] ->
                    Describe(constant_pool);
            }
            else Coutput << "(invalid)";
            Coutput << ", ";
            classes[i].inner_class_access_flags.
                Print(AccessFlags::ACCESS_TYPE);
        }
    }
#endif // JOPA_DEBUG
};


//
// Valid for classes, methods, and fields, this marks a compiler-generated
// entity that has no counterpart in source. JVMS 4.7.6.
//
class SyntheticAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo

public:
    SyntheticAttribute(u2 _name_index)
        : AttributeInfo(ATTRIBUTE_Synthetic, _name_index)
    {}
    SyntheticAttribute(ClassFile&);
    virtual ~SyntheticAttribute() {}

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix("Synthetic", constant_pool, 0);
        Coutput << endl;
    }
#endif // JOPA_DEBUG
};


//
// Valid only for classes, this names the source file. JVMS 4.7.7.
//
class SourceFileAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    const u2 sourcefile_index;

public:
    SourceFileAttribute(u2 _name_index, u2 _sourcefile_index)
        : AttributeInfo(ATTRIBUTE_SourceFile, _name_index, 2)
        , sourcefile_index(_sourcefile_index)
    {}
    SourceFileAttribute(ClassFile&);
    virtual ~SourceFileAttribute() {}

    const char* SourceFile(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[sourcefile_index] -> Tag() ==
               CPInfo::CONSTANT_Utf8);
        return ((CPUtf8Info*) constant_pool[sourcefile_index]) -> Bytes();
    }
    u2 SourceFileLength(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[sourcefile_index] -> Tag() ==
               CPInfo::CONSTANT_Utf8);
        return ((CPUtf8Info*) constant_pool[sourcefile_index]) -> Length();
    }

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU2(sourcefile_index);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix("SourceFile", constant_pool, 0);
        Coutput << ", name: " << (unsigned) sourcefile_index;
        if (constant_pool[sourcefile_index] -> Tag() == CPInfo::CONSTANT_Utf8)
        {
            Coutput << "=\"";
            constant_pool[sourcefile_index] -> Describe(constant_pool);
            Coutput << '"';
        }
        else Coutput << "(invalid)";
        Coutput << endl;
    }
#endif // JOPA_DEBUG
};


//
// Valid only for code, this correlates bytecode offsets to source file line
// numbers. JVMS 4.7.8.
//
class LineNumberTableAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    // u2 line_number_table_length; // computed as line_number_table.Length()
    struct LineNumberElement
    {
        u2 start_pc;
        u2 line_number;
    };
    // line_number_table[line_number_table_length]
    Tuple<LineNumberElement> line_number_table;

public:
    LineNumberTableAttribute(u2 _name_index)
        : AttributeInfo(ATTRIBUTE_LineNumberTable, _name_index, 2)
        // +2 for line_number_table_length
        , line_number_table(6, 16)
    {}
    LineNumberTableAttribute(ClassFile&);
    virtual ~LineNumberTableAttribute() {}

    u2 LineNumberTableLength() const
    {
        return line_number_table.Length();
    }

    void AddLineNumber(u2 start_pc, u2 line_number)
    {
        if (line_number_table.Length() > 1)
        {
            LineNumberElement& other = line_number_table.Top();
            if (start_pc == other.start_pc)
            {
                other.line_number = line_number;
                return;
            }
        }
        attribute_length += 4;
        LineNumberElement& entry = line_number_table.Next();
        entry.start_pc = start_pc;
        entry.line_number = line_number;
    }

    void SetMax(u2 max_pc)
    {
        unsigned i = line_number_table.Length();
        while(--i > 0 && line_number_table[i].start_pc > max_pc)
            line_number_table[i].start_pc = max_pc;
    }

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU2(line_number_table.Length());
        for (unsigned i = 0; i < line_number_table.Length(); i++)
        {
            out.PutU2(line_number_table[i].start_pc);
            out.PutU2(line_number_table[i].line_number);
        }
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 2) const
    {
        assert(fill == 2);
        PrintPrefix("LineNumberTable", constant_pool, 2);
        Coutput << ", count: " << (unsigned) line_number_table.Length()
                << endl;
        for (unsigned i = 0; i < line_number_table.Length(); i++)
        {
            Coutput << "    pc: "
                    << (unsigned) line_number_table[i].start_pc
                    << ", line_number: "
                    << (unsigned) line_number_table[i].line_number << endl;
        }
    }
#endif // JOPA_DEBUG
};


//
// Valid only for code, LocalVariableTable gives local variable name, scope,
// and type information relative to bytecode offsets. JVMS 4.7.10.
// Also, the LocalVariableTypeTable gives generic type information.
//
class LocalVariableTableAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    // u2 local_variable_length; // computed as local_variable_table.Length()
    struct LocalVariableElement
    {
        u2 start_pc;
        u2 length;
        u2 name_index;
        u2 descriptor_index;
        u2 index;
    };
    // local_variable_table[local_variable_table_length]
    Tuple<LocalVariableElement> local_variable_table;

public:
    LocalVariableTableAttribute(u2 _name_index, bool generic = false)
        : AttributeInfo(generic ? ATTRIBUTE_LocalVariableTypeTable
                        : ATTRIBUTE_LocalVariableTable, _name_index, 2)
        // +2 for local_variable_table_length
        , local_variable_table(6, 16)
    {}
    LocalVariableTableAttribute(ClassFile&, bool generic);
    virtual ~LocalVariableTableAttribute() {}

    u2 LocalVariableTableLength() const
    {
        return local_variable_table.Length();
    }

    //
    // make entry in local variable table
    //
    void AddLocalVariable(u2 start, u2 end, u2 name, u2 descriptor, u2 index)
    {
        assert(end >= start);

        if (end > start)
        {
            attribute_length += 10;
            LocalVariableElement& entry = local_variable_table.Next();
            entry.start_pc = start;
            entry.length = end - start;
            entry.name_index = name;
            entry.descriptor_index = descriptor;
            entry.index = index;
        }
    }

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU2(local_variable_table.Length());
        for (unsigned i = 0; i < local_variable_table.Length(); i++)
        {
            out.PutU2(local_variable_table[i].start_pc);
            out.PutU2(local_variable_table[i].length);
            out.PutU2(local_variable_table[i].name_index);
            out.PutU2(local_variable_table[i].descriptor_index);
            out.PutU2(local_variable_table[i].index);
        }
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 2) const
    {
        assert(fill == 2);
        PrintPrefix(tag == ATTRIBUTE_LocalVariableTable ? "LocalVariableTable"
                    : "LocalVariableTypeTable", constant_pool, 2);
        Coutput << ", count: " << (unsigned) local_variable_table.Length()
                << endl;
        for (unsigned i = 0; i < local_variable_table.Length(); i++)
        {
            unsigned index = local_variable_table[i].name_index;
            Coutput << "    pc: "
                    << (unsigned) local_variable_table[i].start_pc
                    << ", length: "
                    << (unsigned) local_variable_table[i].length << "(pc: "
                    << (unsigned) (local_variable_table[i].start_pc + 
                                   local_variable_table[i].length)
                    << "), name: " << index;
            if (constant_pool[index] -> Tag() == CPInfo::CONSTANT_Utf8)
            {
                Coutput << '=';
                constant_pool[index] -> Describe(constant_pool);
            }
            else Coutput << "(invalid)";
            index = local_variable_table[i].descriptor_index;
            Coutput << endl << "     type: " << index;
            if (constant_pool[index] -> Tag() == CPInfo::CONSTANT_Utf8)
            {
                Coutput << '=';
                constant_pool[index] -> Describe(constant_pool);
            }
            else Coutput << "(invalid)";
            Coutput << ", index: " << (unsigned) local_variable_table[i].index
                    << endl;
        }
    }
#endif // JOPA_DEBUG
};


//
// Valid for classes, methods, and fields, this marks an entity as
// deprecated. JVMS 4.7.10.
//
class DeprecatedAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo

public:
    DeprecatedAttribute(u2 _name_index)
        : AttributeInfo(ATTRIBUTE_Deprecated, _name_index)
    {}
    DeprecatedAttribute(ClassFile&);
    virtual ~DeprecatedAttribute() {}

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix("Deprecated", constant_pool, 0);
        Coutput << endl;
    }
#endif // JOPA_DEBUG
};


//
// Valid for classes, methods, and fields, this gives the parameterized type
// signature for a generic type. JSR 14 section 7.
//
class SignatureAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    const u2 signature_index;

public:
    SignatureAttribute(u2 name_index, u2 index)
        : AttributeInfo(ATTRIBUTE_Signature, name_index, 2)
        , signature_index(index)
    {}
    SignatureAttribute(ClassFile&);
    virtual ~SignatureAttribute() {}

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU2(signature_index);
    }

    const CPUtf8Info* Signature(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[signature_index] -> Tag() ==
               CPInfo::CONSTANT_Utf8);
        return (const CPUtf8Info*) constant_pool[signature_index];
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix("Signature", constant_pool, 0);
        Coutput << endl << " signature: " << (unsigned) signature_index;
        if (constant_pool[signature_index] -> Tag() == CPInfo::CONSTANT_Utf8)
        {
            Coutput << '=';
            constant_pool[signature_index] -> Describe(constant_pool);
        }
        else Coutput << "(invalid)";
        Coutput << endl;
    }
#endif // JOPA_DEBUG
};


//
// Valid for methods, this marks the method as a bridge for covariant returns.
// JSR 14? (mentioned in JSR 200).
//
class BridgeAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo

public:
    BridgeAttribute(u2 name_index, u2 /*index*/)
        : AttributeInfo(ATTRIBUTE_Bridge, name_index)
    {}
    BridgeAttribute(ClassFile&);
    virtual ~BridgeAttribute() {}

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix("Bridge", constant_pool, 0);
        Coutput << endl;
    }
#endif // JOPA_DEBUG
};


//
// Valid for code, this speeds up verification by listing the types of all
// locals and stack elements at the start of each basic block. JSR 139
// appendix 1 section 2.1 (or JSR 30 section 5.3.1.2).
//
class StackMapAttribute : public AttributeInfo
{
public:
    class StackMapFrame
    {
        class VerificationTypeInfo
        {
        public:
            enum VerificationTypeInfoTag
            {
                TYPE_Top = 0, // also called Bogus
                TYPE_Integer = 1,
                TYPE_Float = 2,
                TYPE_Long = 4,
                TYPE_Double = 3,
                TYPE_Null = 5,
                TYPE_UninitializedThis = 6,
                TYPE_Object = 7,
                TYPE_Uninitialized = 8
            };

        private:
            VerificationTypeInfoTag tag;
            // u2 cpool_index for Object,
            // u2 offset for Unitialized
            // unused for remaining types
            u2 info;

        public:
            VerificationTypeInfo()
                : tag(TYPE_Top)
            {}
            void Read(ClassFile&);

            u2 Size() const { return tag >= TYPE_Object ? 3 : 1; }

            void Put(OutputBuffer& out) const
            {
                out.PutU1((u1) tag);
                if (tag >= TYPE_Object)
                    out.PutU2(info);
            }

#ifdef JOPA_DEBUG
            void Print(const ConstantPool& constant_pool,
                       unsigned& index) const
            {
                Coutput << index++ << ':';
                switch (tag)
                {
                case TYPE_Top:
                    Coutput << "void";
                    break;
                case TYPE_Integer:
                    Coutput << "int";
                    break;
                case TYPE_Float:
                    Coutput << "float";
                    break;
                case TYPE_Long:
                    Coutput << "long";
                    index++;
                    break;
                case TYPE_Double:
                    Coutput << "double";
                    index++;
                    break;
                case TYPE_Null:
                    Coutput << "null";
                    break;
                case TYPE_UninitializedThis:
                    Coutput << "unitialized this";
                    break;
                case TYPE_Object:
                    Coutput << (unsigned) info;
                    if (constant_pool[info] -> Tag() == CPInfo::CONSTANT_Class)
                    {
                        Coutput << '=';
                        constant_pool[info] -> Describe(constant_pool);
                    }
                    else Coutput << "(invalid)";
                    break;
                case TYPE_Uninitialized:
                    Coutput << "unitialized since " << (unsigned) info;
                    break;
                default:
                    assert(false && "invalid VerificationTypeInfoTag");
                }
            }
#endif // JOPA_DEBUG
        };

    private:
        u2 offset;
        // u2 locals_size; // computed as locals.Length()
        Tuple<VerificationTypeInfo> locals;
        // u2 stack_size; // computed as stack.Length()
        Tuple<VerificationTypeInfo> stack;

        u2 frame_size; // needed since VerificationTypeInfo is variable size

    public:
        StackMapFrame(u2 _offset)
            : offset(_offset)
            , locals(6, 16)
            , stack(6, 16)
            , frame_size(6)
            // +2 for offset, +2 for locals_size, +2 for stack_size
        {}
        StackMapFrame(ClassFile&);

        // Note that LocalsSize() + StackSize() + 6 <= FrameSize()
        u2 LocalsSize() const { return locals.Length(); }
        u2 StackSize() const { return stack.Length(); }
        u2 FrameSize() const { return frame_size; }

        void AddLocal(const VerificationTypeInfo& entry)
        {
            frame_size += entry.Size();
            locals.Next() = entry;
        }
        void AddStack(const VerificationTypeInfo& entry)
        {
            frame_size += entry.Size();
            stack.Next() = entry;
        }

        void Put(OutputBuffer& out) const
        {
            out.PutU2(offset);
            out.PutU2(locals.Length());
            for (unsigned i = 0; i < locals.Length(); i++)
                locals[i].Put(out);
            out.PutU2(stack.Length());
            for (unsigned j = 0; j < stack.Length(); j++)
                stack[j].Put(out);
        }

#ifdef JOPA_DEBUG
        void Print(const ConstantPool& constant_pool) const
        {
            Coutput << "    pc: " << (unsigned) offset << ", locals_size: "
                    << (unsigned) locals.Length() << ", stack_size: "
                    << (unsigned) stack.Length() << endl;
            unsigned index = 0;
            if (locals.Length())
            {
                Coutput << "      locals: ";
                locals[0].Print(constant_pool, index);
                for (unsigned i = 1; i < locals.Length(); i++)
                {
                    Coutput << ", ";
                    locals[i].Print(constant_pool, index);
                }
                Coutput << endl;
            }
            index = 0;
            if (stack.Length())
            {
                Coutput << "      stack: ";
                stack[0].Print(constant_pool, index);
                for (unsigned i = 1; i < stack.Length(); i++)
                {
                    Coutput << ", ";
                    stack[i].Print(constant_pool, index);
                }
                Coutput << endl;
            }
        }
#endif // JOPA_DEBUG
    };

private:
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    // u2 frame_count; // computed as frames.Length();
    Tuple<StackMapFrame*> frames; // stack_map_frame[frame_count]

public:
    StackMapAttribute(u2 name)
        : AttributeInfo(ATTRIBUTE_StackMap, name, 2)
        // +2 for frame_count
        , frames(6, 16)
    {}
    StackMapAttribute(ClassFile&);
    virtual ~StackMapAttribute()
    {
        for (unsigned i = 0; i < frames.Length(); i++)
            delete frames[i];
    }

    u2 FrameCount() const { return frames.Length(); }

    void AddFrame(StackMapFrame* frame)
    {
        frames.Next() = frame;
        attribute_length += frame -> FrameSize();
    }

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU2(frames.Length());
        for (unsigned i = 0; i < frames.Length(); i++)
            frames[i] -> Put(out);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 2) const
    {
        assert(fill == 2);
        PrintPrefix("StackMap", constant_pool, 2);
        Coutput << ", frames: "
                << (unsigned) frames.Length() << endl;
        for (unsigned i = 0; i < frames.Length(); i++)
            frames[i] -> Print(constant_pool);
    }
#endif // JOPA_DEBUG
};


//
// Valid only on non-abstract non-native methods, this is the method
// implementation. JVMS 4.7.3.
//
class CodeAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    u2 max_stack;
    u2 max_locals;
    // u4 code_length; // computed as code.Length();
    Tuple<u1> code; // code[code_length]
    // u2 exception_table_length; // computed as exception_table.Length();
    struct ExceptionElement
    {
        u2 start_pc;
        u2 end_pc;
        u2 handler_pc;
        u2 catch_type;
    };
    // exception_table[exception_table_length]
    Tuple<ExceptionElement> exception_table;
    // u2 attribute_count; // computed as attributes.Length();
    Tuple<AttributeInfo*> attributes; // attributes[attributes_count]

    // Remember location of known attributes.
    LineNumberTableAttribute* attr_linenumbers;
    LocalVariableTableAttribute* attr_locals;
    LocalVariableTableAttribute* attr_local_types;
    StackMapAttribute* attr_stackmap;

public:
    CodeAttribute(u2 _name_index, u2 _max_locals)
        : AttributeInfo(ATTRIBUTE_Code, _name_index, 12)
        // +2 for max_stack, +2 for max_locals, +4 for code_length,
        // +2 for exception_table_length, +2 for attributes_count
        , max_stack(0) // filled in later
        , max_locals(_max_locals)
        , code(8, 4)
        , exception_table(6, 16)
        , attributes(6, 16)
        , attr_linenumbers(NULL)
        , attr_locals(NULL)
        , attr_stackmap(NULL)
    {}
    CodeAttribute(ClassFile&);
    virtual ~CodeAttribute()
    {
        for (unsigned i = 0; i < attributes.Length(); i++)
            delete attributes[i];
    }

    u2 MaxStack() const { return max_stack; }
    void SetMaxStack(u2 val) { max_stack = val; }

    u2 MaxLocals() const { return max_locals; }
    void SetMaxLocals(u2 val) { max_locals = val; }

    u2 CodeLength() const
    {
        return CodeLengthExceeded() ? 65535 : (u2) code.Length();
    }
    bool CodeLengthExceeded() const { return code.Length() > 65535; }

    void ResetCode(unsigned i, u1 byte) { code[i] = byte; }

    void AddCode(u1 byte)
    {
        code.Next() = byte;
        attribute_length++;
    }

    void DeleteCode(unsigned count)
    {
        assert(count <= code.Length());
        code.Reset(code.Length() - count);
        attribute_length -= count;
    }

    u2 ExceptionTableLength() const { return exception_table.Length(); }

    void AddException(u2 start_pc, u2 end_pc, u2 handler_pc, u2 catch_type)
    {
        if (start_pc == end_pc)
            return;
        if (exception_table.Length())
        {
            ExceptionElement& other = exception_table.Top();
            if (end_pc == other.start_pc && handler_pc == other.handler_pc &&
                catch_type == other.catch_type)
            {
                other.start_pc = start_pc;
                return;
            }
        }
        attribute_length += 8;
        ExceptionElement& entry = exception_table.Next();
        entry.start_pc = start_pc;
        entry.end_pc = end_pc;
        entry.handler_pc = handler_pc;
        entry.catch_type = catch_type;
    }

    u2 AttributesCount() const { return attributes.Length(); }

    void AddAttribute(AttributeInfo* attribute)
    {
        attributes.Next() = attribute;
        attribute_length += 6 + attribute -> AttributeLength();
        // +6 for name and length of attribute
        switch (attribute -> Tag())
        {
        case AttributeInfo::ATTRIBUTE_LineNumberTable:
            assert(! attr_linenumbers);
            attr_linenumbers = (LineNumberTableAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_LocalVariableTable:
            assert(! attr_locals);
            attr_locals = (LocalVariableTableAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_LocalVariableTypeTable:
            assert(! attr_local_types);
            attr_local_types = (LocalVariableTableAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_StackMap:
            assert(! attr_stackmap);
            attr_stackmap = (StackMapAttribute*) attribute;
            break;
        default:
            assert(false && "adding unexpected code attribute");
        }
    }

    virtual void Put(OutputBuffer& out) const
    {
        unsigned i;
        AttributeInfo::Put(out);
        out.PutU2(max_stack);
        out.PutU2(max_locals);
        out.PutU4(code.Length());
        for (i = 0; i < code.Length(); i++)
            out.PutU1(code[i]);
        out.PutU2(exception_table.Length());
        for (i = 0; i < exception_table.Length(); i++)
        {
            out.PutU2(exception_table[i].start_pc);
            out.PutU2(exception_table[i].end_pc);
            out.PutU2(exception_table[i].handler_pc);
            out.PutU2(exception_table[i].catch_type);
        }
        out.PutU2(attributes.Length());
        for (i = 0; i < attributes.Length(); i++)
            attributes[i] -> Put(out);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const;
#endif // JOPA_DEBUG
};


//
// Forward declaration, since AnnotationComponentValue and Annotation are
// mutually referential.
//
class Annotation;

//
// Describes an annotation value, as used inside AnnotationsAttribute,
// ParameterAnnotationsAttribute, and AnnotationDefaultAttribute. JSR 175
// section IX.
//
class AnnotationComponentValue
{
public:
    enum AnnotationComponentValueTag
    {
        COMPONENT_byte = U_B,
        COMPONENT_char = U_C,
        COMPONENT_double = U_D,
        COMPONENT_float = U_F,
        COMPONENT_int = U_I,
        COMPONENT_long = U_J,
        COMPONENT_short = U_S,
        COMPONENT_boolean = U_Z,
        COMPONENT_string = U_s,
        COMPONENT_enum = U_e,
        COMPONENT_class = U_c,
        COMPONENT_annotation = U_AT,
        COMPONENT_array = U_LB
    };
protected:
    const AnnotationComponentValueTag tag;

public:
    AnnotationComponentValue(AnnotationComponentValueTag _tag)
        : tag(_tag)
    {}
    virtual ~AnnotationComponentValue() {}

    //
    // This reads the next component from the buffer, advancing it to the
    // end of the component. The user is responsible for deleting the result.
    //
    static AnnotationComponentValue* AllocateAnnotationComponentValue(ClassFile&);

    AnnotationComponentValueTag Tag() const { return tag; }

    // Subclasses must override.
    virtual u2 Length() const { return 1; } // +1 tag

    // Subclasses must override.
    virtual void Put(OutputBuffer& out) const
    {
        out.PutU1((u1) tag);
    }

#ifdef JOPA_DEBUG
    // Subclasses must override.
    virtual void Print(const ConstantPool&) const
    {
        Coutput << "<invalid tag:" << (unsigned) tag << '>';
    }
#endif // JOPA_DEBUG
};


//
// Represents boolean, byte, short, char, int, long, float, double, String,
// and Class annotation values.
//
class AnnotationComponentConstant : public AnnotationComponentValue
{
    // u1 tag; // inherited from AnnotationComponentValue
    // u2 const_value_index for boolean, byte, short, char, int, long, float,
    //    double, String,
    // u2 class_info_index for Class
    const u2 index;

public:
    AnnotationComponentConstant(AnnotationComponentValueTag _tag, u2 _index)
        : AnnotationComponentValue(_tag)
        , index(_index)
    {
        assert(tag != COMPONENT_annotation && tag != COMPONENT_array &&
               tag != COMPONENT_enum);
    }
    AnnotationComponentConstant(ClassFile&, AnnotationComponentValueTag);
    virtual ~AnnotationComponentConstant() {}

    virtual u2 Length() const { return 3; } // +1 tag, +2 index

    virtual void Put(OutputBuffer& out) const
    {
        AnnotationComponentValue::Put(out);
        out.PutU2(index);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& pool) const
    {
        Coutput << (unsigned) index << '=';
        pool[index] -> Describe(pool);
        switch (tag)
        {
        case COMPONENT_boolean:
            if (pool[index] -> Tag() == CPInfo::CONSTANT_Integer)
            {
                i4 value = ((CPIntegerInfo*) pool[index]) -> Value();
                if (value == 0)
                    Coutput << "(false)";
                else if (value == 0)
                    Coutput << "(true)";
                else Coutput << "(invalid)";
            }
            else Coutput << "(invalid)";
            break;
        case COMPONENT_char:
            if (pool[index] -> Tag() == CPInfo::CONSTANT_Integer)
            {
                i4 value = ((CPIntegerInfo*) pool[index]) -> Value();
                if (value == '\n')
                    Coutput << "('\\n')";
                else if (value == '\r')
                    Coutput << "('\\r')";
                else if (value == '\'')
                    Coutput << "('\\'')";
                else if (value == '\\')
                    Coutput << "('\\\\')";
                else if (value > 0x1f && value < 0x7f)
                    Coutput << "('" << (char) value << "')";
                else if (value >= 0 && value <= 0xffff)
                {
                    Coutput << "('\\u" << IntToString(value, 4).String()
                            << "')";
                }
                else Coutput << "(invalid)";
            }
            else Coutput << "(invalid)";
            break;
        case COMPONENT_byte:
            if (pool[index] -> Tag() == CPInfo::CONSTANT_Integer)
            {
                i4 value = ((CPIntegerInfo*) pool[index]) -> Value();
                if (value < -128 || value > 127)
                    Coutput << "(invalid)";
            }
            else Coutput << "(invalid)";
            break;
        case COMPONENT_short:
            if (pool[index] -> Tag() == CPInfo::CONSTANT_Integer)
            {
                i4 value = ((CPIntegerInfo*) pool[index]) -> Value();
                if (value < -32768 || value > 32767)
                    Coutput << "(invalid)";
            }
            else Coutput << "(invalid)";
            break;
        case COMPONENT_int:
            if (pool[index] -> Tag() != CPInfo::CONSTANT_Integer)
                Coutput << "(invalid)";
            break;
        case COMPONENT_long:
            if (pool[index] -> Tag() != CPInfo::CONSTANT_Long)
                Coutput << "(invalid)";
            break;
        case COMPONENT_float:
            if (pool[index] -> Tag() != CPInfo::CONSTANT_Float)
                Coutput << "(invalid)";
            break;
        case COMPONENT_double:
            if (pool[index] -> Tag() != CPInfo::CONSTANT_Double)
                Coutput << "(invalid)";
            break;
        case COMPONENT_string:
            if (pool[index] -> Tag() != CPInfo::CONSTANT_String)
                Coutput << "(invalid)";
            break;
        case COMPONENT_class:
            if (pool[index] -> Tag() != CPInfo::CONSTANT_Class)
                Coutput << "(invalid)";
            break;
        default:
            assert(false && "invalid constant-valued attribute");
        }
    }
#endif // JOPA_DEBUG
};


//
// Represents enum constant annotation values.
//
class AnnotationComponentEnum : public AnnotationComponentValue
{
    // u1 tag; // inherited from AnnotationComponentValue
    const u2 type_name_index;
    const u2 const_name_index;

public:
    AnnotationComponentEnum(u2 type, u2 name)
        : AnnotationComponentValue(COMPONENT_enum)
        , type_name_index(type)
        , const_name_index(name)
    {}
    AnnotationComponentEnum(ClassFile&);
    virtual ~AnnotationComponentEnum() {}

    virtual u2 Length() const { return 5; } // +1 tag, +2 type, +2 name

    virtual void Put(OutputBuffer& out) const
    {
        AnnotationComponentValue::Put(out);
        out.PutU2(type_name_index);
        out.PutU2(const_name_index);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& pool) const
    {
        Coutput << (unsigned) type_name_index << '.'
                << (unsigned) const_name_index << '=';
        pool[type_name_index] -> Describe(pool);
        if (pool[type_name_index] -> Tag() != CPInfo::CONSTANT_Class)
            Coutput << "(invalid)";
        Coutput << '.';
        pool[const_name_index] -> Describe(pool);
        if (pool[type_name_index] -> Tag() != CPInfo::CONSTANT_Utf8)
            Coutput << "(invalid)";
    }
#endif // JOPA_DEBUG
};


//
// Represents nested annotation values.
//
class AnnotationComponentAnnotation : public AnnotationComponentValue
{
    // u1 tag; // inherited from AnnotationComponentValue
    Annotation* attr_value;

public:
    AnnotationComponentAnnotation(Annotation* nested)
        : AnnotationComponentValue(COMPONENT_annotation)
        , attr_value(nested)
    {}
    AnnotationComponentAnnotation(ClassFile&);
    virtual ~AnnotationComponentAnnotation();

    virtual u2 Length() const;

    virtual void Put(OutputBuffer&) const;

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool&) const;
#endif // JOPA_DEBUG
};


//
// Represents array annotation values.
//
class AnnotationComponentArray : public AnnotationComponentValue
{
    // u1 tag; // inherited from AnnotationComponentValue
    // u2 num_values; // computed as values.Length()
    Tuple<AnnotationComponentValue*> values; // values[num_values]

    u2 len; // cache the length

public:
    AnnotationComponentArray()
        : AnnotationComponentValue(COMPONENT_array)
        , values(6, 16)
        , len(3) // +1 tag, +2 num_values
    {}
    AnnotationComponentArray(ClassFile&);
    virtual ~AnnotationComponentArray()
    {
        unsigned i = values.Length();
        while (i--)
            delete values[i];
    }

    u2 NumValues() const { return values.Length(); }

    void AddValue(AnnotationComponentValue* value)
    {
        values.Next() = value;
        len += value -> Length();
    }

    virtual u2 Length() const { return len; }

    virtual void Put(OutputBuffer& out) const
    {
        AnnotationComponentValue::Put(out);
        out.PutU2(values.Length());
        for (unsigned i = 0; i < values.Length(); i++)
            values[i] -> Put(out);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& pool) const
    {
        Coutput << '{';
        unsigned count = values.Length();
        if (count)
            values[0] -> Print(pool);
        for (unsigned i = 1; i < count; i++)
        {
            Coutput << ", ";
            values[i] -> Print(pool);
        }
        Coutput << '}';
    }
#endif // JOPA_DEBUG
};


//
// Describes an annotation, as used inside AnnotationComponentValue,
// ParameterAnnotationsAttribute, and AnnotationDefaultAttribute. JSR 175
// section IX.
//
class Annotation
{
    u2 type_index;
    // u2 num_components; // computed as components.Length()
    struct Component
    {
        u2 component_name_index;
        AnnotationComponentValue* component_value;
    };
    Tuple<Component> components; // components[num_components]

public:
    Annotation(u2 index)
        : type_index(index)
        , components(6, 16)
    {}
    Annotation(ClassFile&);
    ~Annotation()
    {
        unsigned i = components.Length();
        while (i--)
            delete components[i].component_value;
    }

    u2 Length() const
    {
        u2 size = 4; // +2 type_index, +2 num_components
        unsigned i = components.Length();
        while (i--)
            // +2 member_name_index
            size += 2 + components[i].component_value -> Length();
        return size;
    }

    u1 NumComponents() const { return components.Length(); }

    void AddComponent(u2 name_index, AnnotationComponentValue* value)
    {
        Component& component = components.Next();
        component.component_name_index = name_index;
        component.component_value = value;
    }

    void Put(OutputBuffer& out) const
    {
        out.PutU2(type_index);
        unsigned i = components.Length();
        out.PutU2(i);  // Fixed: should be U2, not U1 (matches GetU2 in constructor)
        while (i--)
        {
            out.PutU2(components[i].component_name_index);
            components[i].component_value -> Put(out);
        }
    }

#ifdef JOPA_DEBUG
    void Print(const ConstantPool& pool) const
    {
        Coutput << (unsigned) type_index << "=@";
        pool[type_index] -> Describe(pool);
        unsigned num_components = components.Length();
        Coutput << ", length:" << num_components << '(';
        if (num_components)
        {
            Coutput << (unsigned) components[0].component_name_index << ':';
            pool[components[0].component_name_index] -> Describe(pool);
            Coutput << '=';
            components[0].component_value -> Print(pool);
        }
        for (unsigned i = 1; i < num_components; i++)
        {
            Coutput << ", " << (unsigned) components[i].component_name_index
                    << ':';
            pool[components[i].component_name_index] -> Describe(pool);
            Coutput << '=';
            components[i].component_value -> Print(pool);
        }
        Coutput << ')';
    }
#endif // JOPA_DEBUG
};


//
// Valid on all classes, fields, and methods, this lists the RuntimeVisible
// and RuntimeInvisible annotations tied to the item. JSR 175, section IX.
//
class AnnotationsAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    // u2 num_annotations; // computed as annotations.Length()
    Tuple<Annotation*> annotations; // annotations[num_annotations]

public:
    AnnotationsAttribute(u2 _name_index, bool visible)
        : AttributeInfo((visible ? ATTRIBUTE_RuntimeVisibleAnnotations
                         : ATTRIBUTE_RuntimeInvisibleAnnotations),
                        _name_index, 2)
        // +2 for num_annotations
        , annotations(6, 16)
    {}
    AnnotationsAttribute(ClassFile&, bool visible);
    virtual ~AnnotationsAttribute()
    {
        unsigned i = annotations.Length();
        while (i--)
            delete annotations[i];
    }

    u2 NumAnnotations() const { return annotations.Length(); }

    void AddAnnotation(Annotation* annotation)
    {
        annotations.Next() = annotation;
        attribute_length += annotation -> Length();
    }

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU2(annotations.Length());
        for (unsigned i = 0; i < annotations.Length(); i++)
            annotations[i] -> Put(out);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix((Tag() == ATTRIBUTE_RuntimeVisibleAnnotations
                     ? "RuntimeVisibleAnnotations"
                     : "RuntimeInvisibleAnnotations"), constant_pool, 0);
        Coutput << ", num_annotations: " << (unsigned) annotations.Length()
                << endl;
        for (unsigned i = 0; i < annotations.Length(); i++)
        {
            Coutput << "  ";
            annotations[i] -> Print(constant_pool);
            Coutput << endl;
        }
    }
#endif // JOPA_DEBUG
};


//
// Valid on all methods, this lists the RuntimeVisible and RuntimeInvisible
// annotations tied to each parameter. JSR 175, section IX.
//
class ParameterAnnotationsAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    const u1 num_parameters;
    // u2 num_annotations; // computed as parameters[i].Length()
    // annotation annotations[num_annotations]; // computed as parameters[i]
    // parameter_annotations[num_parameters]
    Tuple<Annotation*>* parameters;

public:
    ParameterAnnotationsAttribute(u2 _name_index, bool visible, u1 params)
        : AttributeInfo((visible ? ATTRIBUTE_RuntimeVisibleParameterAnnotations
                         : ATTRIBUTE_RuntimeInvisibleParameterAnnotations),
                        _name_index, 1 + 2 * params)
        // +1 num_parameters, +2 num_annotations (num_parameters times)
        , num_parameters(params)
        , parameters(NULL)
    {
        if (params)
            parameters = new Tuple<Annotation*>[params];
    }
    ParameterAnnotationsAttribute(ClassFile&, bool visible);
    virtual ~ParameterAnnotationsAttribute()
    {
        for (unsigned i = num_parameters; i--; )
            for (unsigned j = parameters[i].Length(); j--; )
                delete parameters[i][j];
        delete [] parameters;
    }

    u1 NumParameters() const { return num_parameters; }
    u2 NumAnnotations(u1 param) const
    {
        assert(param < num_parameters);
        return parameters[param].Length();
    }

    void AddAnnotation(Annotation* annotation, u1 param)
    {
        assert(param < num_parameters);
        parameters[param].Next() = annotation;
        attribute_length += annotation -> Length();
    }

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU1(num_parameters);
        for (unsigned i = 0; i < num_parameters; i++)
        {
            out.PutU2(parameters[i].Length());
            for (unsigned j = 0; j < parameters[i].Length(); j++)
                parameters[i][j] -> Put(out);
        }
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix((Tag() == ATTRIBUTE_RuntimeVisibleParameterAnnotations
                     ? "RuntimeVisibleParameterAnnotations"
                     : "RuntimeInvisibleParameterAnnotations"),
                    constant_pool, 0);
        Coutput << ", num_parameters: " << (unsigned) num_parameters << endl;
        for (unsigned i = 0; i < num_parameters; i++)
        {
            Coutput << " param " << i << ": num_annotations: "
                    << (unsigned) parameters[i].Length() << endl;
            for (unsigned j = 0; j < parameters[i].Length(); j++)
            {
                Coutput << "  ";
                parameters[i][j] -> Print(constant_pool);
                Coutput << endl;
            }
        }
    }
#endif // JOPA_DEBUG
};


//
// Valid only on method members of annotation types, this lists the default
// return value for the annotation method. JSR 175, section IX.
//
class AnnotationDefaultAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    AnnotationComponentValue* default_value;

public:
    AnnotationDefaultAttribute(u2 _name_index, AnnotationComponentValue* value)
        : AttributeInfo(ATTRIBUTE_AnnotationDefault, _name_index,
                        value -> Length())
        , default_value(value)
    {}
    AnnotationDefaultAttribute(ClassFile&);
    virtual ~AnnotationDefaultAttribute() { delete default_value; }

    const AnnotationComponentValue* DefaultValue() const
    {
        return default_value;
    }

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        default_value -> Put(out);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix("AnnotationDefault", constant_pool, 0);
        Coutput << ", ";
        default_value -> Print(constant_pool);
        Coutput << endl;
    }
#endif // JOPA_DEBUG
};


//
// Valid only on local and anonymous types, this lists the enclosing method
// of the type. JSR 202? (mentioned in JSR 200).
//
class EnclosingMethodAttribute : public AttributeInfo
{
    // u2 attribute_name_index; // inherited from AttributeInfo
    // u4 attribute_length; // inherited from AttributeInfo
    //
    // JSR 202 does not use a methodref, because classes defined in static or
    // instance initializers are given a name_and_type_index of 0.  You can
    // determine whether a class with an unlisted enclosing method was
    // declared in <clinit> or all <init>s by the presence of a this$0 member.
    //
    const u2 class_index;
    const u2 name_and_type_index;

public:
    EnclosingMethodAttribute(u2 _name_index, u2 type, u2 name)
        : AttributeInfo(ATTRIBUTE_EnclosingMethod, _name_index, 4)
        , class_index(type)
        , name_and_type_index(name)
    {}
    EnclosingMethodAttribute(ClassFile&);
    virtual ~EnclosingMethodAttribute() {}

    virtual void Put(OutputBuffer& out) const
    {
        AttributeInfo::Put(out);
        out.PutU2(class_index);
        out.PutU2(name_and_type_index);
    }

#ifdef JOPA_DEBUG
    virtual void Print(const ConstantPool& constant_pool, int fill = 0) const
    {
        assert(! fill);
        PrintPrefix("EnclosingMethod", constant_pool, 0);
        Coutput << endl << " method: " << (unsigned) class_index << '.'
                << (unsigned) name_and_type_index;
        if (constant_pool[class_index] -> Tag() == CPInfo::CONSTANT_Class &&
            (! name_and_type_index ||
             (constant_pool[name_and_type_index] -> Tag() ==
              CPInfo::CONSTANT_NameAndType)))
        {
            Coutput << '=';
            constant_pool[class_index] -> Describe(constant_pool);
            Coutput << '.';
            if (name_and_type_index)
                constant_pool[name_and_type_index] -> Describe(constant_pool);
            else Coutput << "<initializer>";
        }
        else Coutput << "(invalid)";
        Coutput << endl;
    }
#endif // JOPA_DEBUG
};


//
// Describes a member field. JVMS 4.5.
//
class FieldInfo : public AccessFlags
{
    // u2 access_flags; // inherited from AccessFlags
    u2 name_index;
    u2 descriptor_index;
    // u2 attributes_count; // computed as attributes.Length();
    Tuple<AttributeInfo*> attributes; // attributes[attributes_count]

    // Remember location of known attributes.
    SyntheticAttribute* attr_synthetic;
    DeprecatedAttribute* attr_deprecated;
    SignatureAttribute* attr_signature;
    ConstantValueAttribute* attr_constantvalue;
    AnnotationsAttribute* attr_visible_annotations;
    AnnotationsAttribute* attr_invisible_annotations;

public:
    FieldInfo()
        : name_index(0)
        , descriptor_index(0)
        , attr_synthetic(NULL)
        , attr_deprecated(NULL)
        , attr_signature(NULL)
        , attr_constantvalue(NULL)
        , attr_visible_annotations(NULL)
        , attr_invisible_annotations(NULL)
    {}
    FieldInfo(ClassFile&);
    ~FieldInfo()
    {
        for (unsigned i = 0; i < attributes.Length(); i++)
            delete attributes[i];
    }

    inline void SetNameIndex(u2 _name_index) { name_index = _name_index; }
    const char* Name(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[name_index] -> Tag() == CPInfo::CONSTANT_Utf8);
        return ((const CPUtf8Info*) constant_pool[name_index]) -> Bytes();
    }
    u2 NameLength(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[name_index] -> Tag() == CPInfo::CONSTANT_Utf8);
        return ((const CPUtf8Info*) constant_pool[name_index]) -> Length();
    }

    inline void SetDescriptorIndex(u2 index) { descriptor_index = index; }
    const char* Signature(const ConstantPool&, const Control&) const;
    u2 SignatureLength(const ConstantPool&, const Control&) const;

    inline u2 AttributesCount() const { return attributes.Length(); }
    inline void AddAttribute(AttributeInfo* attribute)
    {
        attributes.Next() = attribute;
        switch (attribute -> Tag())
        {
        case AttributeInfo::ATTRIBUTE_Synthetic:
            //
            // We must be adding the attribute because we are targetting an
            // older VM that doesn't recognize the access flag.
            //
            assert(! attr_synthetic && ACC_SYNTHETIC());
            ResetACC_SYNTHETIC();
            attr_synthetic = (SyntheticAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_Deprecated:
            assert(! attr_deprecated);
            attr_deprecated = (DeprecatedAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_Signature:
            assert(! attr_signature);
            attr_signature = (SignatureAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_ConstantValue:
            assert(! attr_constantvalue);
            attr_constantvalue = (ConstantValueAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_RuntimeVisibleAnnotations:
            assert(! attr_visible_annotations);
            attr_visible_annotations = (AnnotationsAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_RuntimeInvisibleAnnotations:
            assert(! attr_invisible_annotations);
            attr_invisible_annotations = (AnnotationsAttribute*) attribute;
            break;
        default:
            assert(false && "adding unexpected field attribute");
        }
    }
    bool Synthetic() const { return attr_synthetic || ACC_SYNTHETIC(); }
    bool Deprecated() const { return attr_deprecated != NULL; }
    const CPInfo* ConstantValue(const ConstantPool& constant_pool) const
    {
        return attr_constantvalue ? attr_constantvalue -> Value(constant_pool)
            : (const CPInfo*) NULL;
    }
    

    inline void Put(OutputBuffer& out) const
    {
        out.PutU2(access_flags);
        out.PutU2(name_index);
        out.PutU2(descriptor_index);
        out.PutU2(attributes.Length());
        for (unsigned i = 0; i < attributes.Length(); i++)
            attributes[i] -> Put(out);
    }

#ifdef JOPA_DEBUG
    void Print(const ConstantPool& constant_pool) const
    {
        Coutput << "field: ";
        AccessFlags::Print(ACCESS_VARIABLE);
        Coutput << ", name: " << (unsigned) name_index;
        if (constant_pool[name_index] -> Tag() == CPInfo::CONSTANT_Utf8)
        {
            Coutput << '=';
            constant_pool[name_index] -> Describe(constant_pool);
        }
        else Coutput << "(invalid)";
        Coutput << endl << " type: " << (unsigned) descriptor_index;
        if (constant_pool[descriptor_index] -> Tag() == CPInfo::CONSTANT_Utf8)
        {
            Coutput << '=';
            constant_pool[descriptor_index] -> Describe(constant_pool);
        }
        else Coutput << "(invalid)";
        Coutput << ", attributes: " << (unsigned) attributes.Length() << endl;
        for (unsigned i = 0; i < attributes.Length(); i++)
            attributes[i] -> Print(constant_pool);
        Coutput << endl;
    }
#endif // JOPA_DEBUG
};


//
// Describes a member method, including constructors and the static
// initializer. JVMS 4.6.
//
class MethodInfo : public AccessFlags
{
    // u2 access_flags; // inherited from AccessFlags
    u2 name_index;
    u2 descriptor_index;
    // u2 attributes_count; // computed as attributes.Length();
    Tuple<AttributeInfo*> attributes; // attributes[attributes_count]

    // Remember location of known attributes.
    SyntheticAttribute* attr_synthetic;
    DeprecatedAttribute* attr_deprecated;
    SignatureAttribute* attr_signature;
    BridgeAttribute* attr_bridge;
    CodeAttribute* attr_code;
    ExceptionsAttribute* attr_exceptions;
    AnnotationsAttribute* attr_visible_annotations;
    AnnotationsAttribute* attr_invisible_annotations;
    ParameterAnnotationsAttribute* attr_param_visible_annotations;
    ParameterAnnotationsAttribute* attr_param_invisible_annotations;
    AnnotationDefaultAttribute* attr_annotation_default;

public:
    MethodInfo()
        : name_index(0)
        , descriptor_index(0)
        , attr_synthetic(NULL)
        , attr_deprecated(NULL)
        , attr_signature(NULL)
        , attr_bridge(NULL)
        , attr_code(NULL)
        , attr_exceptions(NULL)
        , attr_visible_annotations(NULL)
        , attr_invisible_annotations(NULL)
        , attr_param_visible_annotations(NULL)
        , attr_param_invisible_annotations(NULL)
        , attr_annotation_default(NULL)
    {}
    MethodInfo(ClassFile&);
    ~MethodInfo()
    {
        for (unsigned i = 0; i < attributes.Length(); i++)
            delete attributes[i];
    }

    inline void SetNameIndex(u2 _name_index) { name_index = _name_index; }
    const char* Name(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[name_index] -> Tag() == CPInfo::CONSTANT_Utf8);
        return ((const CPUtf8Info*) constant_pool[name_index]) -> Bytes();
    }
    u2 NameLength(const ConstantPool& constant_pool) const
    {
        assert(constant_pool[name_index] -> Tag() == CPInfo::CONSTANT_Utf8);
        return ((const CPUtf8Info*) constant_pool[name_index]) -> Length();
    }

    inline void SetDescriptorIndex(u2 index) { descriptor_index = index; }
    const char* Signature(const ConstantPool&, const Control&) const;
    u2 SignatureLength(const ConstantPool&, const Control&) const;

    inline u2 AttributesCount() const { return attributes.Length(); }
    inline void AddAttribute(AttributeInfo* attribute)
    {
        attributes.Next() = attribute;
        switch (attribute -> Tag())
        {
        case AttributeInfo::ATTRIBUTE_Synthetic:
            //
            // We must be adding the attribute because we are targetting an
            // older VM that doesn't recognize the access flag.
            //
            assert(! attr_synthetic && ACC_SYNTHETIC());
            ResetACC_SYNTHETIC();
            attr_synthetic = (SyntheticAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_Deprecated:
            assert(! attr_deprecated);
            attr_deprecated = (DeprecatedAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_Signature:
            assert(! attr_signature);
            attr_signature = (SignatureAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_Bridge:
            assert(! attr_bridge);
            attr_bridge = (BridgeAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_Code:
            assert(! attr_code);
            attr_code = (CodeAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_Exceptions:
            assert(! attr_exceptions);
            attr_exceptions = (ExceptionsAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_RuntimeVisibleAnnotations:
            assert(! attr_visible_annotations);
            attr_visible_annotations = (AnnotationsAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_RuntimeInvisibleAnnotations:
            assert(! attr_invisible_annotations);
            attr_invisible_annotations = (AnnotationsAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_RuntimeVisibleParameterAnnotations:
            assert(! attr_param_visible_annotations);
            attr_param_visible_annotations =
                (ParameterAnnotationsAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_RuntimeInvisibleParameterAnnotations:
            assert(! attr_param_invisible_annotations);
            attr_param_invisible_annotations =
                (ParameterAnnotationsAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_AnnotationDefault:
            assert(! attr_annotation_default);
            attr_annotation_default = (AnnotationDefaultAttribute*) attribute;
            break;
        default:
            assert(false && "adding unexpected method attribute");
        }
    }
    bool Synthetic() const { return attr_synthetic || ACC_SYNTHETIC(); }
    bool Deprecated() const { return attr_deprecated != NULL; }
    bool Bridge() const { return attr_bridge != NULL; }
    const CodeAttribute* Code() const { return attr_code; }
    const ExceptionsAttribute* Exceptions() const { return attr_exceptions; }

    inline void Put(OutputBuffer& out) const
    {
        out.PutU2(access_flags);
        out.PutU2(name_index);
        out.PutU2(descriptor_index);
        out.PutU2(attributes.Length());
        for (unsigned i = 0; i < attributes.Length(); i++)
            attributes[i] -> Put(out);
    }

#ifdef JOPA_DEBUG
     void Print(const ConstantPool& constant_pool) const
     {
        Coutput << "method: ";
        AccessFlags::Print(ACCESS_METHOD);
        Coutput << " name: " << (unsigned) name_index;
        if (constant_pool[name_index] -> Tag() == CPInfo::CONSTANT_Utf8)
        {
            Coutput << '=';
            constant_pool[name_index] -> Describe(constant_pool);
        }
        else Coutput << "(invalid)";
        Coutput << endl << " type: " << (unsigned) descriptor_index;
        if (constant_pool[descriptor_index] -> Tag() == CPInfo::CONSTANT_Utf8)
        {
            Coutput << '=';
            constant_pool[descriptor_index] -> Describe(constant_pool);
        }
        else Coutput << "(invalid)";
        Coutput << ", attributes: " << (unsigned) attributes.Length() << endl;
        for (unsigned i = 0; i < attributes.Length(); i++)
            attributes[i] -> Print(constant_pool);
        Coutput << endl;
    }
#endif // JOPA_DEBUG
};


//
// Here we have the complete definition of the .class file format. JVMS 4.1.
//
class ClassFile : public AccessFlags
{
    enum
    {
        MAGIC = 0xcafebabe
    };
    const char* problem; // why this file is invalid (NULL if it's okay)
    const char* buffer; // current location during input (NULL for output)
    const char* const buffer_tail; // last location during input

protected:
    u4 magic;
    u2 minor_version;
    u2 major_version;

    // u2 constant_pool_count; // computed as constant_pool.Length()
    // cp_info constant_pool[constant_pool_count - 1]
    ConstantPool constant_pool; // note that constant_pool[0] is invalid

    // u2 access_flags; // inherited from AccessFlags
    u2 this_class;
    u2 super_class;

    // u2 interfaces_count // computed as interfaces.Length()
    Tuple<u2> interfaces; // u2 interfaces[interfaces_count]

    // u2 fields_count // computed as fields.Length()
    Tuple<FieldInfo*> fields; // field_info fields[fields_count]

    // u2 methods_count // computed as methods.Length()
    Tuple<MethodInfo*> methods; // method_info methods[methods_count]

    // u2 attributes_count // computed as attributes.Length()
    // u2 attribute_info attributes[attributes_count]
    Tuple<AttributeInfo*> attributes;

    // Remember location of known attributes.
    SyntheticAttribute* attr_synthetic;
    DeprecatedAttribute* attr_deprecated;
    SignatureAttribute* attr_signature;
    SourceFileAttribute* attr_sourcefile;
    InnerClassesAttribute* attr_innerclasses;
    AnnotationsAttribute* attr_visible_annotations;
    AnnotationsAttribute* attr_invisible_annotations;
    EnclosingMethodAttribute* attr_enclosing_method;

public:
    //
    // Construct a class file for output, given the finished type.
    //
    ClassFile()
        : problem(NULL)
        , buffer(NULL)
        , buffer_tail(NULL)
        , magic(MAGIC)
        , constant_pool(8, 4)
        , interfaces(6, 16)
        , fields(6, 16)
        , methods(6, 16)
        , attributes(6, 16)
        , attr_synthetic(NULL)
        , attr_deprecated(NULL)
        , attr_signature(NULL)
        , attr_sourcefile(NULL)
        , attr_innerclasses(NULL)
        , attr_visible_annotations(NULL)
        , attr_invisible_annotations(NULL)
        , attr_enclosing_method(NULL)
    {}

    //
    // Read a class from input. Note that we try to parse to the end of the
    // file, even after encountering an error, for purposes of decompiling.
    //
    ClassFile(const char*, unsigned);

    ~ClassFile()
    {
        unsigned i;
        for (i = 1; i < constant_pool.Length(); i++)
            if (constant_pool.Valid(i))
                delete constant_pool[i];
        for (i = 0; i < fields.Length(); i++)
            delete fields[i];
        for (i = 0; i < methods.Length(); i++)
            delete methods[i];
        for (i = 0; i < attributes.Length(); i++)
            delete attributes[i];
    }

    u2 ConstantPoolCount() const { return constant_pool.Length(); }
    const ConstantPool& Pool() const { return constant_pool; }
    CPClassInfo* ThisClass()
    {
        return constant_pool[this_class] -> Tag() == CPInfo::CONSTANT_Class
            ? (CPClassInfo*) constant_pool[this_class]
            : (CPClassInfo*) NULL;
    }
    CPClassInfo* SuperClass()
    {
        return constant_pool[super_class] -> Tag() == CPInfo::CONSTANT_Class
            ? (CPClassInfo*) constant_pool[super_class]
            : (CPClassInfo*) NULL;
    }
    u2 InterfacesCount() const { return interfaces.Length(); }
    CPClassInfo* Interface(u2 i)
    {
        return constant_pool[interfaces[i]] -> Tag() == CPInfo::CONSTANT_Class
            ? (CPClassInfo*) constant_pool[interfaces[i]]
            : (CPClassInfo*) NULL;
    }
    u2 FieldsCount() const { return fields.Length(); }
    const FieldInfo* Field(u2 i) const { return fields[i]; }
    u2 MethodsCount() const { return methods.Length();}
    const MethodInfo* Method(u2 i) const { return methods[i]; }

    u2 AttributesCount() const { return attributes.Length(); }
    const AttributeInfo* Attribute(u2 i) const { return attributes[i]; }
    inline void AddAttribute(AttributeInfo* attribute)
    {
        attributes.Next() = attribute;
        switch (attribute -> Tag())
        {
        case AttributeInfo::ATTRIBUTE_Synthetic:
            //
            // We must be adding the attribute because we are targetting an
            // older VM that doesn't recognize the access flag.
            //
            assert(! attr_synthetic && ACC_SYNTHETIC());
            ResetACC_SYNTHETIC();
            attr_synthetic = (SyntheticAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_Deprecated:
            assert(! attr_deprecated);
            attr_deprecated = (DeprecatedAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_Signature:
            assert(! attr_signature);
            attr_signature = (SignatureAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_SourceFile:
            assert(! attr_sourcefile);
            attr_sourcefile = (SourceFileAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_InnerClasses:
            assert(! attr_innerclasses);
            attr_innerclasses = (InnerClassesAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_RuntimeVisibleAnnotations:
            assert(! attr_visible_annotations);
            attr_visible_annotations = (AnnotationsAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_RuntimeInvisibleAnnotations:
            assert(! attr_invisible_annotations);
            attr_invisible_annotations = (AnnotationsAttribute*) attribute;
            break;
        case AttributeInfo::ATTRIBUTE_EnclosingMethod:
            assert(! attr_enclosing_method);
            attr_enclosing_method = (EnclosingMethodAttribute*) attribute;
            break;
        default:
            assert(false && "adding unexpected class attribute");
        }
    }
    bool Synthetic() const { return attr_synthetic || ACC_SYNTHETIC(); }
    bool Deprecated() const { return attr_deprecated != NULL; }
    const char* SourceFile() const
    {
        return attr_sourcefile ? attr_sourcefile -> SourceFile(constant_pool)
            : (const char*) NULL;
    }
    u2 SourceFileLength() const
    {
        return attr_sourcefile
            ? attr_sourcefile -> SourceFileLength(constant_pool) : 0;
    }
    const InnerClassesAttribute* InnerClasses() const
    {
        return attr_innerclasses;
    }
    const SignatureAttribute* Signature() const { return attr_signature; }

    void Write(TypeSymbol* unit_type) const;

    bool Valid() const { return (problem == NULL); }
    void MarkInvalid(const char* reason) { problem = reason; }
    const char* DescribeProblem() const { return problem; }

    inline u1 GetU1()
    {
        if (buffer == buffer_tail)
        {
            MarkInvalid("couldn't read u1");
            return 0;
        }
        return (u1) *buffer++;
    }
    inline u2 GetU2()
    {
        if (buffer + 1 >= buffer_tail)
        {
            MarkInvalid("couldn't read u2");
            buffer = buffer_tail;
            return 0;
        }
        u2 i = (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        return i;
    }
    inline u2 PeekU2()
    {
        if (buffer + 1 >= buffer_tail)
        {
            MarkInvalid("couldn't peek u2");
            return 0;
        }
        u2 i = (u1) *buffer;
        i = (i << 8) | (u1) buffer[1];
        return i;
    }
    inline u4 GetU4()
    {
        if (buffer + 3 >= buffer_tail)
        {
            MarkInvalid("couldn't get u4");
            buffer = buffer_tail;
            return 0;
        }
        u4 i = (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        return i;
    }
    inline BaseLong GetU8()
    {
        if (buffer + 7 >= buffer_tail)
        {
            MarkInvalid("couldn't get u8");
            buffer = buffer_tail;
            return 0;
        }
        BaseLong i = (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        i = (i << 8) | (u1) *buffer++;
        return i;
    }
    //
    // Gets max(len, rest of buffer) bytes, and stores it in a new u1[len + 1]
    // in the previously uninitialized bytes.  The extra byte is set to U_NULL,
    // so if bytes had no embedded NULLs, it can be cast to (const char*) and
    // used in methods like strlen(). Returns the size of bytes, and caller
    // is responsible for calling delete[] on bytes.
    //
    inline u4 GetN(u1*& bytes, u4 len)
    {
        if (buffer + len > buffer_tail)
        {
            MarkInvalid("couldn't get n bytes");
            len = buffer_tail - buffer;
        }
        bytes = new u1[len + 1];
        if (len)
            memcpy(bytes, buffer, len);
        bytes[len] = U_NULL;
        buffer += len;
        return len;
    }
    inline void SkipN(u4 len)
    {
        if (buffer + len > buffer_tail)
        {
            MarkInvalid("couldn't skip n bytes");
            buffer = buffer_tail;
        }
        else buffer += len;
    }

#ifdef JOPA_DEBUG
public:
    void Print()
    {
        unsigned i;
        if (problem)
            Coutput << " *** Not a valid Java .class file (" << problem
                    << ")! ***" << endl;

        Coutput << "*** Magic number: 0x";
        for (i = 32; i; i -= 4)
        {
            char c = (magic >> (i - 4)) & 0xf;
            Coutput << (char) (c < 10 ? c + '0' : c + 'a' - 10);
        }
        Coutput << ", version: " << major_version << '.' << minor_version
                << endl << endl << "### Constant pool: constant_pool_count: "
                << (unsigned) constant_pool.Length() << endl;
        for (i = 1; i < constant_pool.Length(); i++)
        {
            if (! constant_pool.Valid(i))
                continue;
            Coutput.width(5);
            Coutput << i << ": ";
            constant_pool[i] -> Print(constant_pool);
        }
        Coutput << endl << "### ";
        AccessFlags::Print(ACCESS_TYPE);
        Coutput << "this_class: " << (unsigned) this_class;
        if (constant_pool[this_class] -> Tag() == CPInfo::CONSTANT_Class)
        {
            Coutput << '=';
            constant_pool[this_class] -> Describe(constant_pool);
        }
        else Coutput << "(invalid)";
        Coutput << endl << "super_class: " << (unsigned) super_class;
        if (! super_class)
            Coutput << "(none)";
        else if (constant_pool[super_class] -> Tag() == CPInfo::CONSTANT_Class)
        {
            Coutput << '=';
            constant_pool[super_class] -> Describe(constant_pool);
        }
        else Coutput << "(invalid)";
        Coutput << endl << "interfaces_count: "
                << (unsigned) interfaces.Length() << endl;
        for (i = 0; i < interfaces.Length(); i++)
        {
            Coutput << "  " << (unsigned) interfaces[i];
            if (constant_pool[interfaces[i]] -> Tag() ==
                CPInfo::CONSTANT_Class)
            {
                Coutput << '=';
                constant_pool[interfaces[i]] -> Describe(constant_pool);
            }
            else Coutput << "(invalid)";
            Coutput << endl;
        }
        Coutput << endl << "### Fields: fields_count: "
                << (unsigned) fields.Length() << endl;
        for (i = 0; i < fields.Length(); i++)
            fields[i] -> Print(constant_pool);
        Coutput << endl << "### Methods: methods_count: "
                << (unsigned) methods.Length() << endl;
        for (i = 0; i < methods.Length(); i++)
            methods[i] -> Print(constant_pool);
        Coutput << endl << "### Attributes: attributes_count: "
                << (unsigned) attributes.Length() << endl;
        for (i = 0; i < attributes.Length(); i++)
            attributes[i] -> Print(constant_pool);
    }
#endif // JOPA_DEBUG
};


} // Close namespace Jopa block

