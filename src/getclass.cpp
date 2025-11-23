#include "getclass.h"
#include "control.h"
#include "semantic.h"
#include "access.h"
#include "zipfile.h"
#include "jikesapi.h"
#include "option.h"
#include "code.h"

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

inline u1 Semantic::GetU1(const char *buffer)
{
    return *buffer;
}

inline u2 Semantic::GetU2(const char *buffer)
{
    u2 i = (u1) *buffer++;
    return (i << 8) + (u1) *buffer;
}

inline u4 Semantic::GetU4(const char *buffer)
{
    u4 i = (u1) *buffer++;
    i = (i << 8) + (u1) *buffer++;
    i = (i << 8) + (u1) *buffer++;
    return (i << 8) + (u1) *buffer;
}

inline u1 Semantic::GetAndSkipU1(const char *&buffer)
{
    return (u1) *buffer++;
}

inline u2 Semantic::GetAndSkipU2(const char *&buffer)
{
    u2 i = (u1) *buffer++;
    return (i << 8) + (u1) *buffer++;
}

inline u4 Semantic::GetAndSkipU4(const char *&buffer)
{
    u4 i = (u1) *buffer++;
    i = (i << 8) + (u1) *buffer++;
    i = (i << 8) + (u1) *buffer++;
    return (i << 8) + (u1) *buffer++;
}

inline void Semantic::Skip(const char *&buffer, int n)
{
    buffer += n;
}


TypeSymbol *Semantic::ProcessNestedType(TypeSymbol *base_type,
                                        NameSymbol *name_symbol,
                                        LexStream::TokenIndex tok)
{
    TypeSymbol *inner_type = base_type -> FindTypeSymbol(name_symbol);
    if (! inner_type)
    {
        int length = base_type -> ExternalNameLength() + 1 +
            name_symbol -> NameLength(); // +1 for $,... +1 for $
        wchar_t *external_name = new wchar_t[length + 1]; // +1 for '\0';
        wcscpy(external_name, base_type -> ExternalName());
        wcscat(external_name, StringConstant::US_DS);
        wcscat(external_name, name_symbol -> Name());
        NameSymbol *external_name_symbol =
            control.FindOrInsertName(external_name, length);

        delete [] external_name;

        inner_type = base_type -> InsertNestedTypeSymbol(name_symbol);
        inner_type -> outermost_type = base_type -> outermost_type;
        inner_type -> supertypes_closure = new SymbolSet;
        inner_type -> subtypes = new SymbolSet;
        inner_type -> SetExternalIdentity(external_name_symbol);
        inner_type -> SetOwner(base_type);
        inner_type -> SetSignature(control);

        FileSymbol *file_symbol =
            Control::GetFile(control, base_type -> ContainingPackage(),
                             external_name_symbol);
        if (file_symbol)
        {
            inner_type -> file_symbol = file_symbol;
            inner_type -> SetLocation();

            ReadClassFile(inner_type, tok);
        }
        else
        {
            // this symbol table will only contain a default constructor
            inner_type -> SetSymbolTable(1);
            inner_type -> super = control.Object();
            inner_type -> MarkBad();
            AddDefaultConstructor(inner_type);
            ReportSemError(SemanticError::TYPE_NOT_FOUND,
                           tok,
                           tok,
                           inner_type -> ContainingPackage() -> PackageName(),
                           inner_type -> ExternalName());
        }
    }

    return inner_type;
}


TypeSymbol *Semantic::RetrieveNestedTypes(TypeSymbol *base_type,
                                          wchar_t *signature,
                                          LexStream::TokenIndex tok)
{
    int len;
    for (len = 0;
         signature[len] != U_NULL && signature[len] != U_DOLLAR; len++)
        ;
    NameSymbol *name_symbol = control.FindOrInsertName(signature, len);
    TypeSymbol *inner_type = ProcessNestedType(base_type, name_symbol, tok);

    return (signature[len] == U_DOLLAR
            ? RetrieveNestedTypes(inner_type, &signature[len + 1], tok)
            : inner_type);
}


TypeSymbol *Semantic::ReadTypeFromSignature(TypeSymbol *base_type,
                                            const char *utf8_signature,
                                            int length,
                                            LexStream::TokenIndex tok)
{
    TypeSymbol *type = control.type_table.FindType(utf8_signature, length);

    if (type)
    {
        if (type -> SourcePending())
            control.ProcessHeaders(type -> file_symbol);
    }
    else
    {
        wchar_t *signature = new wchar_t[length + 1];
        Control::ConvertUtf8ToUnicode(signature, utf8_signature, length);

        int total_length;
        for (total_length = 0;
             signature[total_length] != U_NULL &&
                 signature[total_length] != U_DOLLAR; total_length++)
            ;

        if (signature[total_length] != U_NULL &&
            Code::IsDigit(signature[total_length + 1]))
        {
            // an anonymous or a local type?
            for (total_length += 2;
                 Code::IsDigit(signature[total_length]); total_length++)
                // will stop at next '$' or '\0' !!!
                ;
            if (signature[total_length] != U_NULL)
            {
                // not an anonymous type? then scan local type name
                for (total_length++;
                     signature[total_length] != U_NULL &&
                         signature[total_length] != U_DOLLAR; total_length++)
                    ;
            }
        }

        int len;
        for (len = total_length - 1;
             len >= 0 && signature[len] != U_SLASH; len--)
            ;

        wchar_t *name = &(signature[len + 1]);

        //
        // When a package name is specified in the signature, we look for the
        // type in question in that package, i.e., we redefine package.
        // Otherwise, we search for the type in the unnamed package.
        //
        PackageSymbol *package = NULL;

        //
        // Which package?
        //
        if (len >= 0)
        {
            wchar_t *package_name = new wchar_t[len + 1];
            for (int i = 0; i < len; i++)
                package_name[i] = signature[i];
            package_name[len] = U_NULL;
            package = control.ProcessPackage(package_name);

            if (package -> directory.Length() == 0)
            {
                ReportSemError(SemanticError::PACKAGE_NOT_FOUND,
                               tok,
                               tok,
                               package -> PackageName());
            }
            delete [] package_name;
        }
        else package = control.unnamed_package;

        //
        // Process type
        //
        NameSymbol *name_symbol =
            control.FindOrInsertName(name, total_length - (len + 1));
        type = package -> FindTypeSymbol(name_symbol);
        if (type)
        {
            if (type -> SourcePending())
                control.ProcessHeaders(type -> file_symbol);
        }
        else
        {
            FileSymbol *file_symbol = Control::GetFile(control, package,
                                                       name_symbol);

            //
            // If we are dealing with the unnamed package, ...
            //
            if ((! file_symbol) && package == control.unnamed_package)
                file_symbol = Control::GetFile(control,
                                               control.unnamed_package,
                                               name_symbol);

            //
            // If a file_symbol was not found, ReadType will issue an error
            // message
            //
            type = ReadType(file_symbol, package, name_symbol, tok);

            //
            // If we have to do a full check and we have a case where a
            // ".class" file depends on a ".java" file then we should signal
            // that the ".java" file associated with the ".class" file should
            // be recompiled.
            //
            if (control.option.full_check && ! control.option.depend &&
                file_symbol && file_symbol -> IsJava() &&
                ! file_symbol -> IsZip())
            {
                control.recompilation_file_set.AddElement(file_symbol);
                if (! control.option.incremental && control.option.pedantic)
                {
                    ReportSemError(SemanticError::RECOMPILATION,
                                   tok,
                                   tok,
                                   base_type -> ContainingPackage() -> Name(),
                                   base_type -> ExternalName(),
                                   type -> ContainingPackage() -> Name(),
                                   type -> ExternalName());
                }
            }
        }

        if (signature[total_length] == U_DOLLAR)
            type = RetrieveNestedTypes(type, &signature[total_length + 1],
                                       tok);

        delete [] signature;
    }

    //
    // Establish a dependence from base_type (read from a class file) to type.
    //
    AddDependence(base_type, type);

    return type;
}


TypeSymbol *Semantic::ProcessSignature(TypeSymbol *base_type,
                                       const char *signature,
                                       LexStream::TokenIndex tok)
{
    TypeSymbol *type;
    int num_dimensions = 0;

    for (; *signature == U_LEFT_BRACKET; signature++)
        num_dimensions++;

    switch (*signature)
    {
        case U_B:
             type = control.byte_type;
             break;
        case U_C:
             type = control.char_type;
             break;
        case U_D:
             type = control.double_type;
             break;
        case U_F:
             type = control.float_type;
             break;
        case U_I:
             type = control.int_type;
             break;
        case U_J:
             type = control.long_type;
             break;
        case U_L:
             {
                 //
                 // The signature is of the form: "L<filename>;" - So, +1 to
                 // skip the 'L' ReadTypeFromSignature considers a semicolon
                 // to be a terminator.
                 //
                 const char *str = ++signature;
                 while (*str != U_SEMICOLON)
                     str++;
                 type = ReadTypeFromSignature(base_type, signature,
                                              str - signature, tok);
             }
             break;
        case U_S:
             type = control.short_type;
             break;
        case U_Z:
             type = control.boolean_type;
             break;
        case U_V:
             type = control.void_type;
             break;
        default:
            assert(false && "don't know what to do with signature");
            break;
    }

    return (num_dimensions == 0 ? type : type -> GetArrayType(this,
                                                              num_dimensions));
}


inline TypeSymbol *Semantic::GetClassPool(TypeSymbol *base_type,
                                          TypeSymbol **class_pool,
                                          const char **constant_pool,
                                          int index,
                                          LexStream::TokenIndex tok)
{
    if (! class_pool[index])
    {
        u2 name_index = Constant_Class_info::NameIndex(constant_pool[index]);
        const char *str = Constant_Utf8_info::Bytes(constant_pool[name_index]);

        if (*str == U_LEFT_BRACKET)
            class_pool[index] = ProcessSignature(base_type, str, tok);
        else
        {
            u2 length = Constant_Utf8_info::Length(constant_pool[name_index]);
            const char *p;
            for (p = &str[length - 1]; Code::IsDigit(*p); p--)
                ;
            if (*p != U_DOLLAR) // not an anonymous class
                class_pool[index] = ReadTypeFromSignature(base_type, str,
                                                          length, tok);
        }
    }

    return class_pool[index];
}


void Semantic::ReadClassFile(TypeSymbol *type, LexStream::TokenIndex tok)
{
#ifdef JIKES_DEBUG
    control.class_files_read++;
#endif

    FileSymbol *file_symbol = type -> file_symbol;

    if (control.option.verbose)  {
        Coutput << "[read "
                << file_symbol -> FileName()
                << "]" << endl;
    }

    if (file_symbol -> IsZip())
    {
        ZipFile *zipfile = new ZipFile(file_symbol);

        if (zipfile -> Buffer() == NULL)
        {
            // this symbol table will only contain a default constructor
            type -> SetSymbolTable(1);
            if (type != control.Object())
                type -> super = (type == control.Throwable()
                                 ? control.Object() : control.Throwable());
            type -> MarkBad();
            AddDefaultConstructor(type);

            ReportSemError(SemanticError::COMPRESSED_ZIP_FILE,
                           tok,
                           tok,
                           file_symbol -> PathSym() -> Name(),
                           type -> ContainingPackage() -> PackageName(),
                           type -> ExternalName());
        }
        else if (! ProcessClassFile(type, zipfile -> Buffer(),
                                    file_symbol -> uncompressed_size, tok))
            ProcessBadClass(type, tok);

        delete zipfile;
    }
    else
    {
        // Get a ReadObject from the API that contains the filew data.
        JikesAPI::FileReader *classFile =
            JikesAPI::getInstance()->read(file_symbol->FileName());
        if (classFile == NULL)
        {
            // this symbol table will only contain a default constructor
            type -> SetSymbolTable(1);
            if (type != control.Object())
                type -> super = (type == control.Throwable()
                                 ? control.Object() : control.Throwable());
            type -> MarkBad();
            AddDefaultConstructor(type);

            ReportSemError(SemanticError::CANNOT_OPEN_CLASS_FILE,
                           tok,
                           tok,
                           type -> ContainingPackage() -> PackageName(),
                           type -> ExternalName());
        }

        else
        {
            // Process the file data.
            size_t   size  = classFile->getBufferSize();

#if defined(WIN32_FILE_SYSTEM)
            size = ((0xFFFFFFFF && GetLastError()) != NO_ERROR) ? 0 : size;
#endif
            if (! ProcessClassFile(type, classFile -> getBuffer(), size, tok))
                ProcessBadClass(type, tok);

            delete classFile;
        }
    }
}


void Semantic::ProcessBadClass(TypeSymbol *type, LexStream::TokenIndex tok)
{
    if (! type -> Table()) // if there is no symbol table, add one
        // this symbol table will only contain a default constructor
        type -> SetSymbolTable(1);
    // if there is no super type, add one
    if ((! type -> super) && type != control.Object())
        type -> super = (type == control.Throwable()
                         ? control.Object() : control.Throwable());
    type -> MarkBad();
    // if there are no constructors, add a default one
    if (! type -> FindConstructorSymbol())
        AddDefaultConstructor(type);

    ReportSemError(SemanticError::INVALID_CLASS_FILE,
                   tok,
                   tok,
                   type -> ContainingPackage() -> PackageName(),
                   type -> ExternalName());
}


bool Semantic::ProcessClassFile(TypeSymbol *type, const char *buffer,
                                int buffer_size, LexStream::TokenIndex tok)
{
    const char *buffer_tail = &buffer[buffer_size];

    type -> MarkHeaderProcessed();
    type -> MarkConstructorMembersProcessed();
    type -> MarkMethodMembersProcessed();
    type -> MarkFieldMembersProcessed();
    type -> MarkLocalClassProcessingCompleted();
    type -> MarkSourceNoLongerPending();

    Skip(buffer, 8); // u4 magic;
                     // u2 minor_version;
                     // u2 major_version;

    if (! InRange(buffer, buffer_tail, 2))
        return false;
    u2 constant_pool_count = GetAndSkipU2(buffer);
    const char **constant_pool = new const char*[constant_pool_count];
    TypeSymbol **class_pool =
        (TypeSymbol **) memset(new TypeSymbol*[constant_pool_count], 0,
                               constant_pool_count * sizeof(TypeSymbol *));

    constant_pool[0] = NULL;
    int *next_pool_index = new int[constant_pool_count],
        Class_root = 0,
        NameAndType_root = 0;

    for (int i = 1; i < constant_pool_count; i++)
    {
        constant_pool[i] = buffer;
        if (! InRange(buffer, buffer_tail, 1))
            return false;
        u1 tag = GetAndSkipU1(buffer);
        if (tag == Cp_Info::CONSTANT_Long || tag == Cp_Info::CONSTANT_Double)
            ++i; // skip the next entry for eight-byte constants

        u2 length = 0;
        switch (tag)
        {
            case Cp_Info::CONSTANT_Utf8:
                 if (! InRange(buffer, buffer_tail, 2))
                     return false;
                 length = GetAndSkipU2(buffer);
                 break;
            case Cp_Info::CONSTANT_Class:
                 length = 2; // set_NameIndex(GetU2(classfile));
                 // save index of class constant
                 next_pool_index[i] = Class_root;
                 Class_root = i;
                 break;
            case Cp_Info::CONSTANT_String:
                 length = 2; // set_NameIndex(GetU2(classfile));
                 break;
            case Cp_Info::CONSTANT_NameAndType:
                 length = 4;
                 // set_class_index(GetU2(classfile)); set_name_and_type_index(GetU2(classfile));
                 //                                     or
                 // set_NameIndex(GetU2(classfile)); set_DescriptorIndex(GetU2(classfile));
                 //                                     or
                 // SetBytes(GetU4(classfile));

                 // save index of class constant
                 next_pool_index[i] = NameAndType_root;
                 NameAndType_root = i;
                 break;
            case Cp_Info::CONSTANT_Fieldref:
            case Cp_Info::CONSTANT_Methodref:
            case Cp_Info::CONSTANT_InterfaceMethodref:
            case Cp_Info::CONSTANT_Integer:
            case Cp_Info::CONSTANT_Float:
                 length = 4;
                 // set_class_index(GetU2(classfile)); set_name_and_type_index(GetU2(classfile));
                 //                                     or
                 // set_NameIndex(GetU2(classfile)); set_DescriptorIndex(GetU2(classfile));
                 //                                     or
                 // SetBytes(GetU4(classfile));
                 break;
            case Cp_Info::CONSTANT_Long:
            case Cp_Info::CONSTANT_Double:
                 length = 8; // set_HighBytes(GetU4(classfile));
                             // set_LowBytes(GetU4(classfile));

                 break;
            default:
                fprintf(stderr, "%s%d%s", "chaos: CODE \"", (int) tag,
                        "\" is an invalid tag !!!\n");
                fflush(stderr);
                break;
        }

        Skip(buffer, length);
    }

    //
    // We are now ready to process this class file. If type is a nested
    // class, we will set its untransformed access flags properly later...
    //
    if (! InRange(buffer, buffer_tail, 2))
        return false;
    type -> SetFlags(GetAndSkipU2(buffer));

    if (! InRange(buffer, buffer_tail, 2))
        return false;
    u2 this_class_index = GetAndSkipU2(buffer); // The index of this class
    u2 name_index =
        Constant_Class_info::NameIndex(constant_pool[this_class_index]);
    const char *type_name =
        Constant_Utf8_info::Bytes(constant_pool[name_index]);
    u2 type_name_length =
        Constant_Utf8_info::Length(constant_pool[name_index]);

    int n;
    for (n = type_name_length; n >= 0 && type_name[n] != U_SLASH; n--)
        ;

    bool matched_package_names;
    if (type -> ContainingPackage() -> PackageNameLength() == n)
    {
        wchar_t *package_name = type -> ContainingPackage() -> PackageName();
        int i;
        for (i = 0; i < n && package_name[i] == type_name[i]; i++)
            ;
        matched_package_names = (i == n);
    }
    else matched_package_names = (n < 0 && (type -> ContainingPackage() ==
                                            control.unnamed_package));

    if (! matched_package_names)
    {
        // this symbol table will only contain a default constructor
        type -> SetSymbolTable(1);
        if (type != control.Object())
            type -> super = (type == control.Throwable()
                             ? control.Object() : control.Throwable());
        type -> MarkBad();
        AddDefaultConstructor(type);

        if (n < 0)
            n = 0;
        wchar_t *package_name = new wchar_t[n + 1];
        for (int i = 0; i < n; i++)
            package_name[i] = type_name[i];
        package_name[n] = U_NULL;

        if (type -> ContainingPackage() == control.unnamed_package)
             ReportSemError(SemanticError::TYPE_NOT_IN_UNNAMED_PACKAGE,
                            tok,
                            tok,
                            type -> ExternalName(),
                            type -> file_symbol -> directory_symbol -> PathSym() -> Name(),
                            package_name);
        else ReportSemError(SemanticError::TYPE_IN_WRONG_PACKAGE,
                            tok,
                            tok,
                            type -> ExternalName(),
                            type -> ContainingPackage() -> PackageName(),
                            package_name);

        delete [] package_name;
    }
    else
    {
        // An outermost type contained in the unnamed package?
        if (! type -> IsNested() && n < 0)
        {
            TypeSymbol *old_type = (TypeSymbol *)
                control.unnamed_package_types.Image(type -> Identity());
            if (! old_type)
                control.unnamed_package_types.AddElement(type);
            else
            {
                ReportSemError(SemanticError::DUPLICATE_TYPE_DECLARATION,
                               tok,
                               tok,
                               type -> Name(),
                               old_type -> FileLoc());
            }
        }

        //
        // On systems such as NT and Win-95 that are not case-sensitive,
        // we need to confirm that the type name specified matches the name
        // in the class file.
        //
        assert(type_name_length - (n + 1) == type -> ExternalNameLength());

        int i;
        for (i = 0; i < type -> ExternalNameLength(); i++)
        {
            if (type_name[(n + 1) + i] != type -> ExternalName()[i])
                break;
        }
        if (i < type -> ExternalNameLength())
        {
            wchar_t *name = new wchar_t[type_name_length + 1];
            for (int k = 0; k < type_name_length; k++)
                name[k] = type_name[k];
            name[type_name_length] = U_NULL;
            ReportSemError(SemanticError::TYPE_NAME_MISMATCH,
                           tok,
                           tok,
                           type -> ContainingPackage() -> PackageName(),
                           type -> ExternalName(),
                           name);
            delete [] name;
        }

        //
        // ... Start doing real work !!!
        //
        if (! InRange(buffer, buffer_tail, 2))
            return false;
        u2 super_class = GetAndSkipU2(buffer);
        if (super_class)
        {
            type -> super = GetClassPool(type, class_pool, constant_pool,
                                         super_class, tok);

            type -> supertypes_closure -> AddElement(type -> super);
            type -> supertypes_closure -> Union(*type -> super ->
                                                supertypes_closure);
        }

        if (! InRange(buffer, buffer_tail, 2))
            return false;
        for (int j = GetAndSkipU2(buffer); j > 0; j--)
        {
            if (! InRange(buffer, buffer_tail, 2))
                return false;
            u2 interface_index = GetAndSkipU2(buffer);
            type -> AddInterface(GetClassPool(type, class_pool, constant_pool,
                                              interface_index, tok));

            type -> supertypes_closure -> AddElement(type -> super);
            type -> supertypes_closure -> Union(*type -> super ->
                                                supertypes_closure);
        }

        //
        // Read all the fields
        //
        if (! InRange(buffer, buffer_tail, 2))
            return false;
        u2 fields_count = GetAndSkipU2(buffer);
        VariableSymbol **fields = new VariableSymbol*[fields_count];
        for (int k = 0; k < fields_count; k++)
        {
            if (! InRange(buffer, buffer_tail, 6))
                return false;
            u2 access_flags = GetAndSkipU2(buffer);
            u2 name_index = GetAndSkipU2(buffer);
            u2 descriptor_index = GetAndSkipU2(buffer);

            NameSymbol *name_symbol =
                control.ConvertUtf8ToUnicode(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                                             Constant_Utf8_info::Length(constant_pool[name_index]));

            VariableSymbol *symbol = new VariableSymbol(name_symbol);
            fields[k] = symbol;

            symbol -> SetOwner(type);
            symbol -> MarkComplete();
            symbol -> MarkInitialized();
            symbol -> SetFlags(access_flags);
            symbol -> SetSignatureString(Constant_Utf8_info::Bytes(constant_pool[descriptor_index]),
                                         Constant_Utf8_info::Length(constant_pool[descriptor_index]));

            if (! InRange(buffer, buffer_tail, 2))
                return false;
            int j = GetAndSkipU2(buffer);
            for (; j > 0; j--)
            {
                if (! InRange(buffer, buffer_tail, 2))
                    return false;
                u2 name_index = GetAndSkipU2(buffer);

                if ((Constant_Utf8_info::Length(constant_pool[name_index]) ==
                     StringConstant::U8S_Synthetic_length) &&
                    memcmp(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                           StringConstant::U8S_Synthetic,
                           StringConstant::U8S_Synthetic_length * sizeof(char)) == 0)
                {
                    symbol -> MarkSynthetic();
                    if (! InRange(buffer, buffer_tail, 4))
                        return false;
                    Skip(buffer, 4);
                    // u4 attribute_length() { return attribute_length_; }
                    // there is no info associated with a Synthetic attribute
                    // If the field is synthetic, remaining attributes don't
                    // matter...
                    break;
                }
                else if ((Constant_Utf8_info::Length(constant_pool[name_index]) ==
                          StringConstant::U8S_Deprecated_length) &&
                         memcmp(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                                StringConstant::U8S_Deprecated,
                                StringConstant::U8S_Deprecated_length * sizeof(char)) == 0)
                {
                    symbol -> MarkDeprecated();
                    if (! InRange(buffer, buffer_tail, 4))
                        return false;
                    Skip(buffer, 4);
                    // u4 attribute_length() { return attribute_length_; }
                    // there is no info associated with a Deprecated attribute
                }
                else if ((Constant_Utf8_info::Length(constant_pool[name_index]) ==
                          StringConstant::U8S_ConstantValue_length) &&
                         memcmp(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                                StringConstant::U8S_ConstantValue,
                                StringConstant::U8S_ConstantValue_length * sizeof(char)) == 0)
                {
                    Skip(buffer, 4); // u4 attribute_length;
                    if (! InRange(buffer, buffer_tail, 2))
                        return false;
                    u2 constantvalue_index = GetAndSkipU2(buffer);

                    u1 tag = Cp_Info::Tag(constant_pool[constantvalue_index]);
                    if (tag == Cp_Info::CONSTANT_Integer)
                    {
                        int value = Constant_Integer_info::Value(constant_pool[constantvalue_index]);
                        symbol -> initial_value =
                            control.int_pool.FindOrInsert(value);
                    }
                    else if (tag == Cp_Info::CONSTANT_Long)
                    {
                        LongInt value = Constant_Long_info::Value(constant_pool[constantvalue_index]);
                        symbol -> initial_value =
                            control.long_pool.FindOrInsert(value);
                    }
                    else if (tag == Cp_Info::CONSTANT_Float)
                    {
                        IEEEfloat value = Constant_Float_info::Value(constant_pool[constantvalue_index]);
                        symbol -> initial_value =
                            control.float_pool.FindOrInsert(value);
                    }
                    else if (tag == Cp_Info::CONSTANT_Double)
                    {
                        IEEEdouble value = Constant_Double_info::Value(constant_pool[constantvalue_index]);
                        symbol -> initial_value =
                            control.double_pool.FindOrInsert(value);
                    }
                    else if (tag == Cp_Info::CONSTANT_String)
                    {
                        u2 string_index = Constant_String_info::StringIndex(constant_pool[constantvalue_index]);
                        u2 length = Constant_Utf8_info::Length(constant_pool[string_index]);
                        const char *value = Constant_Utf8_info::Bytes(constant_pool[string_index]);
                        symbol -> initial_value =
                            control.Utf8_pool.FindOrInsert(value, length);
                    }
                    else if (tag == Cp_Info::CONSTANT_Utf8)
                    {
                        u2 length = Constant_Utf8_info::Length(constant_pool[constantvalue_index]);
                        const char *value = Constant_Utf8_info::Bytes(constant_pool[constantvalue_index]);
                        symbol -> initial_value =
                            control.Utf8_pool.FindOrInsert(value, length);
                    }
                    else
                    {
                        fprintf(stderr, "%s%d%s", "chaos: Constant tag \"",
                                (int) tag, "\" is an invalid tag !!!\n");
                        fflush(stderr);
                    }
                }
                else
                {
                    if (! InRange(buffer, buffer_tail, 4))
                        return false;
                    Skip(buffer, GetAndSkipU4(buffer));
                    // u4 attribute_length() { return attribute_length_; }
                    // u1 *info; /* info[attribute_length] */
                }
            }

            //
            // Skip remaining attributes
            //
            while (--j > 0)
            {
                Skip(buffer, 2);                    // u2 name_index
                if (! InRange(buffer, buffer_tail, 4))
                    return false;
                Skip(buffer, GetAndSkipU4(buffer));
                // u4 attribute_length() { return attribute_length_; }
                // u1 *info; /* info[attribute_length] */
            }
        }

        //
        // Read all the methods
        //
        if (! InRange(buffer, buffer_tail, 2))
            return false;
        u2 methods_count = GetAndSkipU2(buffer);
        MethodSymbol **methods = new MethodSymbol*[methods_count];
        for (int l = 0; l < methods_count; l++)
        {
            if (! InRange(buffer, buffer_tail, 6))
                return false;
            u2 access_flags = GetAndSkipU2(buffer);
            u2 name_index = GetAndSkipU2(buffer);
            u2 descriptor_index = GetAndSkipU2(buffer);

            NameSymbol *name_symbol = control.ConvertUtf8ToUnicode(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                                                               Constant_Utf8_info::Length(constant_pool[name_index]));

            if (! InRange(buffer, buffer_tail, 2))
                return false;
            int j = GetAndSkipU2(buffer); // number of attributes

            if (name_symbol == control.clinit_name_symbol)
            {
                methods[l] = NULL;
                j++; // see the loop (way) below that skips the remaining attributes
            }
            else
            {
                MethodSymbol *method = new MethodSymbol(name_symbol);
                methods[l] = method;

                method -> SetContainingType(type);
                method -> SetFlags(access_flags);

                const char *signature =
                    Constant_Utf8_info::Bytes(constant_pool[descriptor_index]);
                int length = Constant_Utf8_info::Length(constant_pool[descriptor_index]);

                method -> SetSignature(control.Utf8_pool.FindOrInsert(signature, length));

                //
                // Read the exception that can be thrown by this method
                //
                for (; j > 0; j--)
                {
                    if (! InRange(buffer, buffer_tail, 2))
                        return false;
                    u2 name_index = GetAndSkipU2(buffer);

                    if ((Constant_Utf8_info::Length(constant_pool[name_index]) ==
                         StringConstant::U8S_Synthetic_length) &&
                        memcmp(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                               StringConstant::U8S_Synthetic,
                               StringConstant::U8S_Synthetic_length * sizeof(char)) == 0)
                    {
                        method -> MarkSynthetic();
                        if (! InRange(buffer, buffer_tail, 4))
                            return false;
                        Skip(buffer, 4);
                        // u4 attribute_length() { return attribute_length_; }
                        // there is no info associated with a Synthetic
                        // attribute. If the field is synthetic, remaining
                        // attributes don't matter...
                        methods[l] = NULL;
                        delete method;
                        break;
                    }
                    else if ((Constant_Utf8_info::Length(constant_pool[name_index]) ==
                              StringConstant::U8S_Deprecated_length) &&
                             memcmp(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                                    StringConstant::U8S_Deprecated,
                                    StringConstant::U8S_Deprecated_length * sizeof(char)) == 0)
                    {
                        method -> MarkDeprecated();
                        if (! InRange(buffer, buffer_tail, 4))
                            return false;
                        Skip(buffer, 4);
                        // u4 attribute_length() { return attribute_length_; }
                        // there is no info associated with a Deprecated
                        // attribute
                    }
                    else if ((Constant_Utf8_info::Length(constant_pool[name_index]) ==
                              StringConstant::U8S_Exceptions_length) &&
                             memcmp(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                                    StringConstant::U8S_Exceptions,
                                    StringConstant::U8S_Exceptions_length * sizeof(char)) == 0)
                    {
                        Skip(buffer, 4);
                        // attribute_length = GetAndSkipU4(buffer);
                        if (! InRange(buffer, buffer_tail, 2))
                            return false;
                        for (int k = GetAndSkipU2(buffer); k > 0; k--)
                        {
                            if (! InRange(buffer, buffer_tail, 2))
                                return false;
                            int index = GetAndSkipU2(buffer);
                            u2 name_index = Constant_Class_info::NameIndex(constant_pool[index]);
                            method -> AddThrowsSignature(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                                                         Constant_Utf8_info::Length(constant_pool[name_index]));
                        }
                    }
                    else
                    {
                        if (! InRange(buffer, buffer_tail, 4))
                            return false;
                        Skip(buffer, GetAndSkipU4(buffer));
                        // u4 attribute_length() { return attribute_length_; }
                        // u1 *info; /* info[attribute_length] */
                    }
                }
            }

            //
            // Skip remaining attributes
            //
            while (--j > 0)
            {
                Skip(buffer, 2);                     // u2 name_index
                if (! InRange(buffer, buffer_tail, 4))
                    return false;
                Skip(buffer, GetAndSkipU4(buffer));
                // u4 attribute_length() { return attribute_length_; }
                // u1 *info; /* info[attribute_length] */
            }
        }

        //
        // Process InnerClasses attributes
        //
        if (! InRange(buffer, buffer_tail, 2))
            return false;
        u2 attributes_count = GetAndSkipU2(buffer);
        Tuple<u2> inner_name_indexes(8);
        for (int a = 0; a < attributes_count; a++)
        {
            if (! InRange(buffer, buffer_tail, 2))
                return false;
            u2 name_index = GetAndSkipU2(buffer);

            if ((Constant_Utf8_info::Length(constant_pool[name_index]) ==
                 StringConstant::U8S_InnerClasses_length) &&
                memcmp(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                       StringConstant::U8S_InnerClasses,
                       StringConstant::U8S_InnerClasses_length * sizeof(char)) == 0)
            {
                Skip(buffer, 4); // attribute_length = GetAndSkipU4(buffer);
                if (! InRange(buffer, buffer_tail, 2))
                    return false;
                for (int k = GetAndSkipU2(buffer); k > 0; k--)
                {
                    if (! InRange(buffer, buffer_tail, 8))
                        return false;
                    u2 inner_class_info_index   = GetAndSkipU2(buffer);
                    u2 outer_class_info_index   = GetAndSkipU2(buffer);
                    u2 inner_name_index         = GetAndSkipU2(buffer);
                    u2 inner_class_access_flags = GetAndSkipU2(buffer);

                    //
                    // Recall that the untransformed access flag is the one
                    // specified in the inner_class attribute. (See 1.1
                    // document)
                    //
                    if (inner_class_info_index == this_class_index)
                        type -> SetFlags(inner_class_access_flags);
                    //
                    // This guard statement is used to identify only inner
                    // classes that are not anonymous classes and are
                    // immediately contained within this class.
                    // Recall that an inner class may not be enclosed
                    // (directly on indirectly) in another class with the
                    // same name. Therefore when outer_class_info_index
                    // matches this_class_index they both refer to "this"
                    // class (that we are currently processing).
                    //
                    else if (outer_class_info_index == this_class_index &&
                             inner_class_info_index != 0 &&
                             inner_name_index != 0)
                    {
                        //
                        // When length is 0, the inner class in question is
                        // "this" class ? the one we are currently reading ?
                        //
                        u2 length = Constant_Utf8_info::Length(constant_pool[inner_name_index]);
                        if (length > 0)
                            inner_name_indexes.Next() = inner_name_index;
                    }
                }
            }
            else if ((Constant_Utf8_info::Length(constant_pool[name_index]) ==
                      StringConstant::U8S_Deprecated_length) &&
                     memcmp(Constant_Utf8_info::Bytes(constant_pool[name_index]),
                            StringConstant::U8S_Deprecated,
                            StringConstant::U8S_Deprecated_length * sizeof(char)) == 0)
            {
                type -> MarkDeprecated();
                if (! InRange(buffer, buffer_tail, 4))
                    return false;
                Skip(buffer, 4);
                // u4 attribute_length() { return attribute_length_; }
                // there is no info associated with a Deprecated attribute
            }
            else
            {
                if (! InRange(buffer, buffer_tail, 4))
                    return false;
                Skip(buffer, GetAndSkipU4(buffer));
                // u4 attribute_length() { return attribute_length_; }
                // u1 *info; /* info[attribute_length] */
            }
        }

        //
        // We now have enough information to make a good estimate for the
        // size of the symbol table we need for this class.
        //
        type -> SetSymbolTable(fields_count + methods_count +
                               inner_name_indexes.Length());

        //
        //   . Read in all class files that are referenced in CONSTANT_Class
        //     structures in this class file.
        //
        //   . Read in all class files that are referenced in
        //     CONSTANT_NameAndType structures in this class file.
        //
        if (control.option.full_check &&
            (control.option.unzip || ! type -> file_symbol -> IsZip()))
        {
            for (int h = Class_root; h != 0; h = next_pool_index[h])
                GetClassPool(type, class_pool, constant_pool, h, tok);

            for (int j = NameAndType_root; j != 0; j = next_pool_index[j])
            {
                u2 descriptor_index = Constant_NameAndType_info::DescriptorIndex(constant_pool[j]);
                const char *signature = Constant_Utf8_info::Bytes(constant_pool[descriptor_index]);

                if (! class_pool[descriptor_index])
                {
                    if (*signature != U_LEFT_PARENTHESIS)
                        // ')' indicates a field descriptor
                        class_pool[descriptor_index] =
                            ProcessSignature(type,
                                             Constant_Utf8_info::Bytes(constant_pool[descriptor_index]),
                                             tok);
                    else // a method descriptor
                    {
                        signature++; // +1 to skip initial '('
                        while (*signature != U_RIGHT_PARENTHESIS)
                        {
                            const char *str;
                            for (str = signature;
                                 *str == U_LEFT_BRACKET; str++)
                                ;

                            if (*str == U_L)
                            {
                                for (str++; *str != U_SEMICOLON; str++)
                                    ;
                            }

                            int len = str - signature + 1;
                            // make signature point to next type
                            signature += len;
                        }
                        signature++; // skip L')'

                        // save the return type in first spot
                        class_pool[descriptor_index] =
                            ProcessSignature(type, signature, tok);
                    }
                }
            }

            //
            // Process all the fields, then the methods, then the inner types.
            // Read in all the types associated with the signatures.
            //
            for (int k = 0; k < fields_count; k++)
            {
                type -> InsertVariableSymbol(fields[k]);
                fields[k] -> ProcessVariableSignature((Semantic *) this, tok);
            }

            for (int l = 0; l < methods_count; l++)
            {
                if (methods[l])
                {
                    MethodSymbol *method =
                        type -> FindMethodSymbol(methods[l] -> name_symbol);

                    if (! method)
                    {
                         if (methods[l] -> name_symbol ==
                             control.init_name_symbol)
                         {
                              type -> InsertConstructorSymbol(methods[l]);
                         }
                         else type -> InsertMethodSymbol(methods[l]);
                    }
                    else type -> Overload(method, methods[l]);
                    methods[l] -> ProcessMethodSignature(this, tok);
                }
            }

            for (int m = 0; m < inner_name_indexes.Length(); m++)
            {
                u2 inner_name_index = inner_name_indexes[m];
                type -> AddNestedTypeSignature(Constant_Utf8_info::Bytes(constant_pool[inner_name_index]),
                                               Constant_Utf8_info::Length(constant_pool[inner_name_index]));
            }
            type -> ProcessNestedTypeSignatures((Semantic *) this, tok);
        }
        else
        {
            //
            // Process all the fields, then the methods, then the inner types.
            //
            for (int k = 0; k < fields_count; k++)
                type -> InsertVariableSymbol(fields[k]);

            for (int l = 0; l < methods_count; l++)
            {
                if (methods[l])
                {
                    MethodSymbol *method =
                        type -> FindMethodSymbol(methods[l] -> name_symbol);

                    if (! method)
                    {
                         if (methods[l] -> name_symbol ==
                             control.init_name_symbol)
                         {
                              type -> InsertConstructorSymbol(methods[l]);
                         }
                         else type -> InsertMethodSymbol(methods[l]);
                    }
                    else type -> Overload(method, methods[l]);
                }
            }

            for (int m = 0; m < inner_name_indexes.Length(); m++)
            {
                u2 inner_name_index = inner_name_indexes[m];
                type -> AddNestedTypeSignature(Constant_Utf8_info::Bytes(constant_pool[inner_name_index]),
                                               Constant_Utf8_info::Length(constant_pool[inner_name_index]));
            }
        }

        delete [] fields;
        delete [] methods;

        //
        // Release extra space. This is an optimization.
        //
        type -> CompressSpace();
    }

    delete [] next_pool_index;
    delete [] class_pool;
    delete [] constant_pool;

    return true;
}

#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif

