#ifndef getclass_INCLUDED
#define getclass_INCLUDED

#include "platform.h"
#include "semantic.h"
#include "long.h"
#include "double.h"


namespace Jikes { // Open namespace Jikes block
class Cp_Info
{
public:
    enum
    {
        CONSTANT_Class              = 7,
        CONSTANT_Fieldref           = 9,
        CONSTANT_Methodref          = 10,
        CONSTANT_InterfaceMethodref = 11,
        CONSTANT_String             = 8,
        CONSTANT_Integer            = 3,
        CONSTANT_Float              = 4,
        CONSTANT_Long               = 5,
        CONSTANT_Double             = 6,
        CONSTANT_NameAndType        = 12,
        CONSTANT_Utf8               = 1
    };

    static u1 Tag(const char *buffer) { return (u1) *buffer; }
};

class Constant_Utf8_info : public Cp_Info
{
public:
    static u2 Length(const char *buffer) { return Semantic::GetU2(buffer + 1); } // skip tag
    static const char *Bytes(const char *buffer) { return buffer + 3; } // skip tag and length
};


class Constant_Class_info : public Cp_Info
{
public:
    static u2 NameIndex(const char *buffer) { return Semantic::GetU2(buffer + 1); }
};


class Constant_Fieldref_info : public Cp_Info
{
public:
    static u2 ClassIndex(const char *buffer) { return Semantic::GetU2(buffer + 1); }
    static u2 NameAndTypeIndex(const char *buffer) { return Semantic::GetU2(buffer + 3); }
};


class Constant_Methodref_info : public Cp_Info
{
public:
    static u2 ClassIndex(const char *buffer) { return Semantic::GetU2(buffer + 1); }
    static u2 NameAndTypeIndex(const char *buffer) { return Semantic::GetU2(buffer + 3); }
};


class Constant_InterfaceMethodref_info : public Cp_Info
{
public:
    static u2 ClassIndex(const char *buffer) { return Semantic::GetU2(buffer + 1); }
    static u2 NameAndTypeIndex(const char *buffer) { return Semantic::GetU2(buffer + 3); }
};


class Constant_NameAndType_info : public Cp_Info
{
public:
    static u2 NameIndex(const char *buffer) { return Semantic::GetU2(buffer + 1); }
    static u2 DescriptorIndex(const char *buffer) { return Semantic::GetU2(buffer + 3); }
};


class Constant_String_info : public Cp_Info
{
public:
    static u2 StringIndex(const char *buffer) { return Semantic::GetU2(buffer + 1); }
};


class Constant_Integer_info : public Cp_Info
{
public:
    static u4 Bytes(const char *buffer) { return Semantic::GetU4(buffer + 1); }

    static int Value(const char *buffer)
    {
        union field
        {
            u4 u;
            int i;
        } value;
        value.u = Bytes(buffer);
        return value.i;
    }
};


class Constant_Float_info : public Cp_Info
{
public:
    static u4 Bytes(const char *buffer) { return Semantic::GetU4(buffer + 1); }

    static IEEEfloat Value(const char *buffer)
    {
        return IEEEfloat(Semantic::GetU4(buffer+1));
    }
};


class Constant_Long_info : public Cp_Info
{
public:
    static u4 HighBytes(const char *buffer) { return Semantic::GetU4(buffer + 1); }
    static u4 LowBytes(const char *buffer) { return Semantic::GetU4(buffer + 5); }

    static LongInt Value(const char *buffer)
    {
        return LongInt(HighBytes(buffer), LowBytes(buffer));
    }
};


class Constant_Double_info : public Cp_Info
{
public:
    static u4 HighBytes(const char *buffer) { return Semantic::GetU4(buffer + 1); }
    static u4 LowBytes(const char *buffer) { return Semantic::GetU4(buffer + 5); }

    static IEEEdouble Value(const char *buffer)
    {
        return IEEEdouble(HighBytes(buffer), LowBytes(buffer));
    }
};


} // Close namespace Jikes block

#endif // getclass_INCLUDED

