#include "symbol.h"
#include "stream.h"
#include "control.h"
#include "ast.h"
#include "semantic.h"
#include "table.h"
#include "zipfile.h"
#include "set.h"
#include "case.h"
#include "option.h"
#include "paramtype.h"
#include "typeparam.h"


namespace Jopa { // Open namespace Jopa block
const char* FileSymbol::java_suffix = StringConstant::U8S_DO_java;
unsigned FileSymbol::java_suffix_length = strlen(java_suffix);
const char* FileSymbol::class_suffix = StringConstant::U8S_DO_class;
unsigned FileSymbol::class_suffix_length = strlen(class_suffix);

wchar_t* MethodSymbol::Header()
{
    assert(type_);

    if (! header)
    {
        bool is_constructor = Name()[0] == U_LESS && Name()[1] == U_i;
        unsigned num_parameters = NumFormalParameters();
        int length = (Type() -> ContainingPackage() -> PackageNameLength() +
                      Type() -> ExternalNameLength() +
                      (is_constructor ? containing_type -> NameLength()
                       : NameLength())
                      + 5); // +5 for '.' after package_name, ' ' after type,
                            // '(' after name, ')' after all parameters,
                            // ';' to terminate
        for (unsigned i = 0; i < num_parameters; i++)
        {
            VariableSymbol* formal = FormalParameter(i);
            length += (formal -> Type() -> ContainingPackage() -> PackageNameLength() +
                       formal -> Type() -> ExternalNameLength() +
                       formal -> NameLength() + 4);
            // +4 for '.' after package_name, ' ' after type; ',' and ' ' to
            // separate this formal parameter from the next one. Last
            // parameter may need '...' instead of '[]', but doesn't need ', '.
        }

        if (throws_signatures && NumThrowsSignatures())
        {
            length += 7; // for " throws"
            for (unsigned j = 0; j < NumThrowsSignatures(); j++)
                length += strlen(ThrowsSignature(j)) + 2; // +2 for ", "
        }
        else if (NumThrows())
        {
            length += 7; // for " throws"
            for (unsigned j = 0; j < NumThrows(); j++)
            {
                TypeSymbol* exception = Throws(j);
                length += (exception -> ContainingPackage() ->
                           PackageNameLength() +
                           exception -> ExternalNameLength() + 3);
                // +3 for " throws", '.' after package_name, and ',' and
                // ' ' to separate this throws clause from the next one
            }
        }

        header = new wchar_t[length + 1]; // +1 for '\0'
        wchar_t* s = header;
        const wchar_t* s2;

        if (is_constructor)
        {
            for (s2 = containing_type -> Name(); *s2; s2++)
                 *s++ = *s2;
        }
        else
        {
            PackageSymbol* package = Type() -> ContainingPackage();
            wchar_t* package_name = package -> PackageName();
            if (package -> PackageNameLength() > 0 &&
                wcscmp(package_name, StringConstant::US_DOT) != 0)
            {
                while (*package_name)
                {
                    *s++ = (*package_name == U_SLASH ? (wchar_t) U_DOT
                            : *package_name);
                    package_name++;
                }
                *s++ = U_DOT;
            }

            for (s2 = Type() -> ExternalName(); *s2; s2++)
                 *s++ = *s2;
            *s++ = U_SPACE;
            for (s2 = Name(); *s2; s2++)
                 *s++ = *s2;
        }
        *s++ = U_LEFT_PARENTHESIS;
        if (num_parameters > 0)
        {
            for (unsigned k = 0; k < num_parameters; k++)
            {
                VariableSymbol* formal = FormalParameter(k);

                PackageSymbol* package =
                    formal -> Type() -> ContainingPackage();
                wchar_t* package_name = package -> PackageName();
                if (package -> PackageNameLength() > 0 &&
                    wcscmp(package_name, StringConstant::US_DOT) != 0)
                {
                    while (*package_name)
                    {
                        *s++ = (*package_name == U_SLASH ? (wchar_t) U_DOT
                                : *package_name);
                        package_name++;
                    }
                    *s++ = U_DOT;
                }

                for (s2 = formal -> Type() -> ExternalName(); *s2; s2++)
                {
                    *s++ = *s2;
                }
                if (k == num_parameters - 1 && ACC_VARARGS())
                {
                    if (s[-2] == U_LB && s[-1] == U_RB)
                    {
                        s[-2] = U_DOT;
                        s[-1] = U_DOT;
                        *s++ = U_DOT;
                    }
                    else
                    {
                        // Fallback: just append ... if we didn't find the expected [] suffix
                        *s++ = U_DOT;
                        *s++ = U_DOT;
                        *s++ = U_DOT;
                    }
                }
                *s++ = U_SPACE;
                for (s2 = formal -> Name(); *s2; s2++)
                     *s++ = *s2;
                *s++ = U_COMMA;
                *s++ = U_SPACE;
            }

            s -= 2; // remove the last ',' and ' '
        }
        *s++ = U_RIGHT_PARENTHESIS;

        if (throws_signatures && NumThrowsSignatures())
        {
            *s++ = U_SPACE;
            *s++ = U_t;
            *s++ = U_h;
            *s++ = U_r;
            *s++ = U_o;
            *s++ = U_w;
            *s++ = U_s;
            for (unsigned k = 0; k < NumThrowsSignatures(); k++)
            {
                *s++ = U_SPACE;
                for (char* signature = ThrowsSignature(k);
                     *signature; signature++)
                {
                    *s++ = (*signature == U_SLASH ? (wchar_t) U_DOT
                            : *signature);
                }
                *s++ = U_COMMA;
            }
            s--; // remove the last ','
        }
        else if (NumThrows() > 0)
        {
            *s++ = U_SPACE;
            *s++ = U_t;
            *s++ = U_h;
            *s++ = U_r;
            *s++ = U_o;
            *s++ = U_w;
            *s++ = U_s;
            for (unsigned k = 0; k < NumThrows(); k++)
            {
                TypeSymbol* exception = Throws(k);

                PackageSymbol* package = exception -> ContainingPackage();
                wchar_t* package_name = package -> PackageName();
                *s++ = U_SPACE;
                if (package -> PackageNameLength() > 0 &&
                    wcscmp(package_name, StringConstant::US_DOT) != 0)
                {
                    while (*package_name)
                    {
                        *s++ = (*package_name == U_SLASH ? (wchar_t) U_DOT
                                : *package_name);
                        package_name++;
                    }
                    *s++ = U_DOT;
                }

                for (s2 = exception -> ExternalName(); *s2; s2++)
                    *s++ = *s2;
                *s++ = U_COMMA;
            }

            s--; // remove the last ','
        }

        *s++ = U_SEMICOLON;
        *s = U_NULL;

        assert((s - header) <= length);
    }
    return header;
}

void MethodSymbol::SetLocation()
{
    //AstMethodDeclaration or AstConstructorDeclaration
    if (! declaration)
        file_location = new FileLocation(containing_type -> file_symbol);
    else
    {
        AstMethodDeclaration* method_declaration =
            declaration -> MethodDeclarationCast();
        AstConstructorDeclaration* constructor_declaration =
            declaration -> ConstructorDeclarationCast();

        file_location =
            new FileLocation((containing_type -> semantic_environment ->
                              sem -> lex_stream),
                             (method_declaration
                              ? method_declaration -> LeftToken()
                              : constructor_declaration -> LeftToken()));
    }
}

MethodSymbol* SymbolTable::FindOverloadMethod(MethodSymbol* base_method,
                                              AstMethodDeclarator* method_declarator)
{
    for (MethodSymbol* method = base_method; method;
         method = method -> next_method)
    {
        assert(method -> IsTyped());

        if (method -> NumFormalParameters() ==
            method_declarator -> NumFormalParameters())
        {
            int i;
            for (i = method -> NumFormalParameters() - 1; i >= 0; i--)
            {
                AstFormalParameter* parameter =
                    method_declarator -> FormalParameter(i);
                if (method -> FormalParameter(i) -> Type() !=
                    parameter -> formal_declarator -> symbol -> Type())
                {
                    break;
                }
            }
            if (i < 0)
                return method;
        }
    }
    return NULL;
}


void TypeSymbol::ProcessTypeHeaders()
{
    semantic_environment -> sem -> ProcessTypeHeaders(declaration);
}

void TypeSymbol::ProcessMembers()
{
    semantic_environment -> sem -> ProcessMembers(declaration);
}

void TypeSymbol::CompleteSymbolTable()
{
    semantic_environment -> sem -> CompleteSymbolTable(declaration);
}


void TypeSymbol::ProcessExecutableBodies()
{
    semantic_environment -> sem -> ProcessExecutableBodies(declaration);
}


void TypeSymbol::RemoveCompilationReferences()
{
    if (semantic_environment)
    {
        semantic_environment = NULL;
        declaration = NULL;

        // Clear all AST references in the symbol table so the AST pool
        // can be safely deleted.
        if (table)
        {
            unsigned i;
            for (i = 0; i < table -> NumVariableSymbols(); i++)
                table -> VariableSym(i) -> declarator = NULL;
            for (i = 0; i < table -> NumMethodSymbols(); i++)
                table -> MethodSym(i) -> declaration = NULL;
            for (i = 0; i < table -> NumTypeSymbols(); i++)
                table -> TypeSym(i) -> declaration = NULL;
            for (i = 0; i < table -> NumAnonymousSymbols(); i++)
                table -> AnonymousSym(i) -> declaration = NULL;
        }
    }
}


TypeSymbol* TypeSymbol::GetArrayType(Semantic* sem, unsigned dims)
{
    if (dims == num_dimensions)
        return this;
    if (num_dimensions)
        return base_type -> GetArrayType(sem, dims);
    if (! dims || Bad())
        return this;
    if (dims < NumArrays())
        return Array(dims);

    if (NumArrays() == 0)
        AddArrayType(this);
    TypeSymbol* previous_array_type = Array(array -> Length() - 1);
    wchar_t* name = new wchar_t[ExternalNameLength() + (dims * 2) + 1];
    wcscpy(name, previous_array_type -> ExternalName());

    for (unsigned num = array -> Length(),
             len = previous_array_type -> ExternalNameLength() + 2;
         num <= dims;
         num++, len = len + 2)
    {
        wcscat(name, StringConstant::US_LB_RB);
        NameSymbol* name_sym = sem -> control.FindOrInsertName(name, len);
        TypeSymbol* type = new TypeSymbol(name_sym);

        type -> MarkHeaderProcessed();
        type -> MarkConstructorMembersProcessed();
        type -> MarkMethodMembersProcessed();
        type -> MarkFieldMembersProcessed();
        type -> MarkLocalClassProcessingCompleted();
        type -> MarkSourceNoLongerPending();
        type -> outermost_type = type;

        //
        // An array type has the same accessibility as its component.
        //
        if (ACC_PUBLIC())
          type -> SetACC_PUBLIC();
        else if (ACC_PROTECTED())
          type -> SetACC_PROTECTED();
        else if (ACC_PRIVATE())
          type -> SetACC_PRIVATE();
        type -> SetACC_FINAL();

        type -> super = sem -> control.Object();
        //
        // All arrays implement the interfaces java.io.Serializable and
        // java.io.Cloneable
        //
        type -> AddInterface(sem -> control.Serializable());
        type -> AddInterface(sem -> control.Cloneable());
        type -> base_type = this;
        type -> num_dimensions = num;
        type -> SetOwner(ContainingPackage());
        // only 2 elements will be added to this table
        type -> table = new SymbolTable(2);
        type -> SetSignature(sem -> control);

        MethodSymbol* method =
            type -> InsertMethodSymbol(sem -> control.clone_name_symbol);
        method -> SetType(sem -> control.Object());
        method -> SetContainingType(type);
        method -> SetFlags(ACCESS_PUBLIC | ACCESS_FINAL);
        // the associated symbol table will remain empty
        method -> SetBlockSymbol(new BlockSymbol(1));
        method -> SetSignature(sem -> control);

        VariableSymbol* symbol =
            type -> InsertVariableSymbol(sem -> control.length_name_symbol);
        symbol -> SetFlags(ACCESS_PUBLIC | ACCESS_FINAL);
        symbol -> SetOwner(type);
        symbol -> SetType(sem -> control.int_type);
        symbol -> MarkComplete();
        symbol -> MarkInitialized();

        type -> CompressSpace(); // space optimization
        AddArrayType(type);
    }

    delete [] name;
    return Array(dims);
}

void TypeSymbol::SetLocation()
{
    if (! declaration)
        file_location = new FileLocation(file_symbol);
    else
    {
        file_location =
            new FileLocation(semantic_environment -> sem -> lex_stream,
                             declaration -> identifier_token);
    }
}

void TypeSymbol::SetSignature(Control& control)
{
    if (num_dimensions > 0)
    {
        char* type_signature;
        TypeSymbol* subtype = ArraySubtype();
        // +1 for '['
        int signature_len = strlen(subtype -> SignatureString()) + 1;
        type_signature = new char[signature_len + 1];
        type_signature[0] = U_LEFT_BRACKET;
        strcpy(type_signature + 1, subtype -> SignatureString());
        signature = control.Utf8_pool.FindOrInsert(type_signature,
                                                   signature_len);
        fully_qualified_name = signature;
        delete [] type_signature;
    }
    else
    {
        const wchar_t* package_name = ContainingPackageName();
        const wchar_t* type_name = ExternalName();

        // +1 for 'L' +1 for '/' +1 for ';' +1 for '\0'
        int len = ContainingPackage() -> PackageNameLength() +
                  ExternalNameLength() + 4;
        wchar_t* type_signature = new wchar_t[len];
        type_signature[0] = U_L;
        type_signature[1] = U_NU;
        if (ContainingPackage() -> PackageNameLength() > 0 &&
            wcscmp(package_name, StringConstant::US_DOT) != 0)
        {
            wcscat(type_signature, package_name);
            wcscat(type_signature, StringConstant::US_SL);
        }
        wcscat(type_signature, type_name);
        // +1 to skip the initial L'L'
        fully_qualified_name =
            control.ConvertUnicodeToUtf8(type_signature + 1);

        wcscat(type_signature, StringConstant::US_SC);
        signature = control.ConvertUnicodeToUtf8(type_signature);

        delete [] type_signature;

        if (! (Anonymous() || IsLocal()))
            control.type_table.InsertType(this);
    }
}


//
// Generate the generic signature for a class (Signature attribute)
// Format: TypeParameters? SuperclassSignature SuperinterfaceSignature*
//
// This is needed for:
// 1. Generic types (those with type parameters)
// 2. Types extending parameterized types (e.g., anonymous classes
//    created with "new TypeToken<String>() {}")
//
void TypeSymbol::SetGenericSignature(Control& control)
{
    // Check if we need a signature attribute
    bool needs_signature = IsGeneric() || HasParameterizedSuper();

    if (! needs_signature)
        return;

    // Estimate size: type parameters + super + interfaces
    unsigned estimated_size = 1000;  // Conservative estimate
    char* buffer = new char[estimated_size];
    unsigned length = 0;

    // Add type parameters if present: <T:Ljava/lang/Object;>
    if (NumTypeParameters() > 0)
    {
        buffer[length++] = '<';
        for (unsigned i = 0; i < NumTypeParameters(); i++)
        {
            TypeParameterSymbol* type_param = TypeParameter(i);
            type_param -> GenerateSignature(control);
            const char* param_sig = type_param -> signature_string;
            unsigned param_len = type_param -> signature_length;
            for (unsigned j = 0; j < param_len; j++)
                buffer[length++] = param_sig[j];
        }
        buffer[length++] = '>';
    }

    // Add superclass signature
    // If we have a parameterized superclass, use its full signature
    if (parameterized_super)
    {
        parameterized_super -> GenerateSignature(buffer, length);
    }
    else if (super)
    {
        const char* super_sig = super -> SignatureString();
        unsigned super_len = strlen(super_sig);
        for (unsigned i = 0; i < super_len; i++)
            buffer[length++] = super_sig[i];
    }
    else
    {
        // If no super (e.g., Object or interface), use java/lang/Object
        const char* object_sig = "Ljava/lang/Object;";
        unsigned object_len = strlen(object_sig);
        for (unsigned i = 0; i < object_len; i++)
            buffer[length++] = object_sig[i];
    }

    // Add interface signatures
    for (unsigned i = 0; i < NumInterfaces(); i++)
    {
        // If we have a parameterized interface, use its full signature
        ParameterizedType* param_interf = ParameterizedInterface(i);
        if (param_interf)
        {
            param_interf -> GenerateSignature(buffer, length);
        }
        else
        {
            TypeSymbol* interface = Interface(i);
            const char* interface_sig = interface -> SignatureString();
            unsigned interface_len = strlen(interface_sig);
            for (unsigned j = 0; j < interface_len; j++)
                buffer[length++] = interface_sig[j];
        }
    }

    // Null terminate and store
    buffer[length] = '\0';
    generic_signature = control.Utf8_pool.FindOrInsert(buffer, length);
    delete [] buffer;
}


unsigned SymbolTable::primes[] = {DEFAULT_HASH_SIZE, 101, 401, MAX_HASH_SIZE};

void SymbolTable::Rehash()
{
    hash_size = primes[++prime_index];

    delete [] base;
    base = (Symbol**) memset(new Symbol*[hash_size], 0,
                             hash_size * sizeof(Symbol*));

    unsigned k;
    for (k = 0; k < NumTypeSymbols(); k++)
    {
        TypeSymbol* symbol = TypeSym(k);
        int i = symbol -> name_symbol -> index % hash_size;
        symbol -> next = base[i];
        base[i] = symbol;
    }

    for (k = 0; k < NumMethodSymbols(); k++)
    {
        MethodSymbol* symbol = MethodSym(k);
        if (symbol -> next != symbol) // not an overload
        {
            int i = symbol -> name_symbol -> index % hash_size;
            symbol -> next = base[i];
            base[i] = symbol;
        }
    }

    for (k = 0; k < NumVariableSymbols(); k++)
    {
        VariableSymbol* symbol = VariableSym(k);
        int i = symbol -> name_symbol -> index % hash_size;
        symbol -> next = base[i];
        base[i] = symbol;
    }

    for (k = 0; k < NumOtherSymbols(); k++)
    {
        Symbol* symbol = OtherSym(k);

        if (! symbol -> BlockCast())
        {
            int i = symbol -> Identity() -> index % hash_size;
            symbol -> next = base[i];
            base[i] = symbol;
        }
    }
}


SymbolTable::SymbolTable(unsigned hash_size_)
    : type_symbol_pool(NULL)
    , anonymous_symbol_pool(NULL)
    , method_symbol_pool(NULL)
    , variable_symbol_pool(NULL)
    , other_symbol_pool(NULL)
{
    hash_size = (hash_size_ <= 0 ? 1 : hash_size_);

    prime_index = -1;
    do
    {
        if (hash_size < primes[prime_index + 1])
            break;
        prime_index++;
    } while (primes[prime_index] < MAX_HASH_SIZE);

    base = (Symbol**) memset(new Symbol*[hash_size], 0,
                             hash_size * sizeof(Symbol*));
}

SymbolTable::~SymbolTable()
{
    unsigned i;
    for (i = 0; i < NumAnonymousSymbols(); i++)
        delete AnonymousSym(i);
    delete anonymous_symbol_pool;
    for (i = 0; i < NumTypeSymbols(); i++)
        delete TypeSym(i);
    delete type_symbol_pool;
    for (i = 0; i < NumMethodSymbols(); i++)
        delete MethodSym(i);
    delete method_symbol_pool;
    for (i = 0; i < NumVariableSymbols(); i++)
        delete VariableSym(i);
    delete variable_symbol_pool;
    for (i = 0; i < NumOtherSymbols(); i++)
        delete OtherSym(i);
    delete other_symbol_pool;
    delete [] base;
}


PackageSymbol::~PackageSymbol()
{
    delete [] package_name;
    delete table;
}


void PackageSymbol::SetPackageName()
{
    package_name_length = (owner ? owner -> PackageNameLength() + 1 : 0) +
        NameLength(); // +1 for '/'
    package_name = new wchar_t[package_name_length + 1]; // +1 for '\0'

    if (owner)
    {
        wcscpy(package_name, owner -> PackageName());
        wcscat(package_name, StringConstant::US_SL);
    }
    else package_name[0] = U_NULL;
    wcscat(package_name, Name());

    assert(wcslen(package_name) == package_name_length);
}


TypeSymbol::TypeSymbol(const NameSymbol* name_symbol_)
  : semantic_environment(NULL),
    declaration(NULL),
    file_symbol(NULL),
    file_location(NULL),
    name_symbol(name_symbol_),
    owner(NULL),
    outermost_type(NULL),
    super(NULL),
    base_type(NULL),
    index(TypeCycleChecker::OMEGA),
    unit_index(TypeCycleChecker::OMEGA),
    incremental_index(TypeCycleChecker::OMEGA),
    local(NULL),
    non_local(NULL),
    supertypes_closure(NULL),
    subtypes(NULL),
    subtypes_closure(NULL),
    innertypes_closure(NULL),
    dependents(new SymbolSet()),
    parents(new SymbolSet()),
    static_parents(new SymbolSet()),
    dependents_closure(NULL),
    parents_closure(NULL),
    signature(NULL),
    fully_qualified_name(NULL),
    expanded_type_table(NULL),
    expanded_field_table(NULL),
    expanded_method_table(NULL),
    num_dimensions(0),
    instance_initializer_method(NULL),
    static_initializer_method(NULL),
    external_name_symbol(NULL),
    table(NULL),
    local_shadow_map(NULL),
    status(0),
    package(NULL),
    class_name(NULL),
    class_literal_method(NULL),
    class_literal_name(NULL),
    assert_variable(NULL),
    local_constructor_call_environments(NULL),
    private_access_methods(NULL),
    private_access_constructors(NULL),
    read_methods(NULL),
    write_methods(NULL),
    placeholder_type(NULL),
    constructor_parameters(NULL),
    enclosing_instance(NULL),
    class_literals(NULL),
    nested_type_signatures(NULL),
    nested_types(NULL),
    interfaces(NULL),
    parameterized_interfaces(NULL),
    anonymous_types(NULL),
    array(NULL),
    type_parameters(NULL),
    parameterized_type(NULL),
    parameterized_super(NULL),
    generic_signature(NULL),
    is_generic(false)
{
    Symbol::_kind = TYPE;
}

unsigned TypeSymbol::NumLocalTypes()
{
    return local ? local -> Size() : 0;
}

//
// Get the erased type for this TypeSymbol
//
TypeSymbol* TypeSymbol::Erasure(Control& control)
{
    return parameterized_type ? parameterized_type -> Erasure(control) : this;
}


TypeSymbol::~TypeSymbol()
{
    unsigned i;

    // Clean up read_methods - it's a nested Map, so we need to delete inner maps first
    if (read_methods)
    {
        read_methods -> DeleteValues();
        delete read_methods;
    }

    // Clean up write_methods - also a nested Map
    if (write_methods)
    {
        write_methods -> DeleteValues();
        delete write_methods;
    }
    delete semantic_environment;
    delete local;
    delete non_local;
    delete supertypes_closure;
    delete subtypes;
    delete subtypes_closure;
    delete innertypes_closure;
    delete dependents;
    delete parents;
    delete static_parents;
    delete table;
    delete local_shadow_map;
    delete expanded_type_table;
    delete expanded_field_table;
    delete expanded_method_table;
    delete file_location;
    delete [] class_name;
    for (i = 1; i < NumArrays(); i++)
        delete Array(i);
    for (i = 0; i < NumNestedTypeSignatures(); i++)
        delete [] NestedTypeSignature(i);
    delete nested_type_signatures;

    delete local_constructor_call_environments;
    delete private_access_methods;
    delete private_access_constructors;
    delete constructor_parameters;
    delete class_literals;
    delete nested_types;
    delete interfaces;
    delete parameterized_interfaces;
    delete anonymous_types;
    delete array;

    // Cleanup generics support fields
    if (type_parameters)
    {
        for (i = 0; i < type_parameters -> Length(); i++)
            delete (*type_parameters)[i];
        delete type_parameters;
    }
    delete parameterized_type;
}


void TypeSymbol::UnlinkFromParents()
{
    if (super)
    {
        super -> subtypes -> RemoveElement(this);
        super -> dependents -> RemoveElement(this);
    }
    if (interfaces)
    {
        for (unsigned i = 0; i < NumInterfaces(); ++i)
        {
            TypeSymbol* interf = Interface(i);
            interf -> subtypes -> RemoveElement(this);
            interf -> dependents -> RemoveElement(this);
        }
    }
}


MethodSymbol::~MethodSymbol()
{
    for (unsigned i = 0; i < NumThrowsSignatures(); i++)
        delete [] ThrowsSignature(i);
    delete throws_signatures;
    delete formal_parameters;
    delete throws;

    delete block_symbol; // overload(s)
    delete [] header;
    delete file_location;

    // Cleanup generics support fields
    if (type_parameters)
    {
        for (unsigned i = 0; i < type_parameters -> Length(); i++)
            delete (*type_parameters)[i];
        delete type_parameters;
    }
    delete bridges_generated;
    delete param_type_param_indices;
}


BlockSymbol::BlockSymbol(unsigned hash_size)
    : max_variable_index(-1)
    , helper_variable_index(-1)
    , table(hash_size > 0 ? new SymbolTable(hash_size) : (SymbolTable*) NULL)
{
    Symbol::_kind = BLOCK;
}

BlockSymbol::~BlockSymbol()
{
    delete table;
}

void BlockSymbol::UnlinkTypesRecursively()
{
    if (!table)
        return;

    // Unlink anonymous types in this block
    for (unsigned i = 0; i < table->NumAnonymousSymbols(); i++)
        table->AnonymousSym(i)->UnlinkFromParents();

    // Unlink regular types in this block
    for (unsigned i = 0; i < table->NumTypeSymbols(); i++)
        table->TypeSym(i)->UnlinkFromParents();

    // Recursively process nested blocks (stored in OtherSym pool)
    for (unsigned i = 0; i < table->NumOtherSymbols(); i++)
    {
        BlockSymbol* nested = table->OtherSym(i)->BlockCast();
        if (nested)
            nested->UnlinkTypesRecursively();
    }
}

PathSymbol::PathSymbol(const NameSymbol* name_symbol_)
    : name_symbol(name_symbol_)
    , zipfile(NULL)
{
    Symbol::_kind = PATH;
}

PathSymbol::~PathSymbol()
{
    if (zipfile)
        delete zipfile;
}

DirectorySymbol::DirectorySymbol(const NameSymbol* name_symbol_,
                                 Symbol* owner_, bool source_dir_only_)
    : owner(owner_)
    , name_symbol(name_symbol_)
    , table(NULL)
    , entries(NULL)
    , directory_name(NULL)
    , source_dir_only(source_dir_only_)
{
    Symbol::_kind = _DIRECTORY;
}




DirectorySymbol::~DirectorySymbol()
{
    delete [] directory_name;
    delete entries;
    delete table;
}

void DirectorySymbol::SetDirectoryName()
{
    PathSymbol* path_symbol = owner -> PathCast();
    if (path_symbol)
    {
        if (strcmp(path_symbol -> Utf8Name(), ".") == 0)
        {
            directory_name_length = Utf8NameLength();
            directory_name = new char[directory_name_length + 1]; // +1: '\0'

            strcpy(directory_name, Utf8Name());
        }
        else
        {
            directory_name_length = path_symbol -> Utf8NameLength();
            directory_name = new char[directory_name_length + 1]; // +1: '\0'

            strcpy(directory_name, path_symbol -> Utf8Name());
        }
    }
    else
    {
        DirectorySymbol* owner_directory = owner -> DirectoryCast();
        if (Name()[NameLength() - 1] == U_SLASH ||
            strcmp(owner_directory -> DirectoryName(), ".") == 0)
        {
            // An absolute file name, or is the owner "." ?
            directory_name_length = Utf8NameLength();
            directory_name = new char[directory_name_length + 1]; // +1: '\0'
            strcpy(directory_name, Utf8Name());
        }
        else
        {
            int owner_length = owner_directory -> DirectoryNameLength();
            char* owner_name = owner_directory -> DirectoryName();
            directory_name_length = owner_length + Utf8NameLength() +
                (owner_name[owner_length - 1] != U_SLASH ? 1 : 0); // +1: '/'

            directory_name = new char[directory_name_length + 1]; // +1: '\0'

            strcpy(directory_name, owner_directory -> DirectoryName());
            if (owner_name[owner_length - 1] != U_SLASH)
                strcat(directory_name, StringConstant::U8S_SL);
            strcat(directory_name, Utf8Name());
        }
    }

    assert(strlen(directory_name) == directory_name_length);
}


void DirectorySymbol::ResetDirectory()
{
    //
    // TODO: the stat function does not work for directories in that the
    // "modified" time stamp associated with a directory is not updated when
    // a file contained in the directory is changed.
    // For now, we always reread the directory.
    //
    //    struct stat status;
    //    if ((SystemStat(DirectoryName(), &status) == 0) &&
    //        status.st_mtime > mtime)
    //    {
    //        mtime = status.st_mtime;
    //
    //        delete entries;
    //        entries = NULL;
    //    }
    //
    //    ReadDirectory();
    //

    delete entries;
    entries = NULL;
    ReadDirectory();
}


void DirectorySymbol::ReadDirectory()
{
    assert(! IsZip());

    if (! entries)
    {
        entries = new DirectoryTable();

//FIXME: these need to go into platform.cpp
#ifdef UNIX_FILE_SYSTEM
        DIR* directory = opendir(DirectoryName());
        if (directory)
        {
            for (dirent* entry = readdir(directory); entry;
                 entry = readdir(directory))
            {
                unsigned length = strlen(entry -> d_name);

                //
                // Check if the file is a java source, a java class file or a
                // subdirectory. Since packages cannot start with '.', we skip
                // all files that start with a dot. That includes this
                // directory "." and its parent ".."
                //
                // Don't add the class file if the source_dir_only flag is set.
                if ((length > FileSymbol::java_suffix_length &&
                     FileSymbol::IsJavaSuffix(&entry -> d_name[length - FileSymbol::java_suffix_length])) ||
                    (! source_dir_only && length > FileSymbol::class_suffix_length &&
                     FileSymbol::IsClassSuffix(&entry -> d_name[length - FileSymbol::class_suffix_length])) ||
                    (Case::Index(entry -> d_name, U_DOT) < 0 &&
                     SystemIsDirectory(entry -> d_name)))
                {
                    int len = DirectoryNameLength() + strlen(entry -> d_name);
                    char* filename = new char[len + 2]; // +2 for '/', NUL
                    sprintf(filename, "%s/%s", DirectoryName(),
                            entry -> d_name);
                    struct stat status;
                    if(JopaAPI::getInstance() -> stat(filename, &status) == 0)
                        entries -> InsertEntry(this, entry -> d_name, length);
                    delete [] filename;
                }
            }
            closedir(directory);
        }

#endif
    }
}


DirectorySymbol* FileSymbol::OutputDirectory()
{
    return output_directory ? output_directory
        : output_directory = Control::GetOutputDirectory(this);
}


FileSymbol::~FileSymbol()
{
    delete [] file_name;
    delete lex_stream;
}


void FileSymbol::SetFileName()
{
    PathSymbol* path_symbol = PathSym();
    char* directory_name = directory_symbol -> DirectoryName();
    size_t directory_name_length = directory_symbol -> DirectoryNameLength();
    bool dot_directory = (strcmp(directory_name, ".") == 0);
    file_name_length = (dot_directory ? 0 : directory_name_length) +
        Utf8NameLength() +
        (path_symbol -> IsZip() ? 2 // For zip files, we need "()";
         : (dot_directory || // for regular directory, we need 1 '/'
            directory_name[directory_name_length - 1] == U_SLASH ? 0 : 1)) +
        (kind == JAVA ? java_suffix_length : class_suffix_length);

    file_name = new char[file_name_length + 1]; // +1 for '\0'
    if (dot_directory)
        file_name[0] = U_NULL;
    else
    {
        strcpy(file_name, directory_symbol -> DirectoryName());
        if (path_symbol -> IsZip())
            strcat(file_name, StringConstant::U8S_LP);
        else if (directory_name[directory_name_length - 1] != U_SLASH)
            strcat(file_name, StringConstant::U8S_SL);
    }
    strcat(file_name, Utf8Name());
    strcat(file_name, kind == JAVA ? FileSymbol::java_suffix
           : FileSymbol::class_suffix);
    if (path_symbol -> IsZip())
        strcat(file_name, StringConstant::U8S_RP);

    assert(strlen(file_name) == file_name_length);
}

#ifdef UNIX_FILE_SYSTEM
bool FileSymbol::IsClassSuffix(char* suffix)
{
    return strncmp(suffix, class_suffix, class_suffix_length) == 0;
}

bool FileSymbol::IsJavaSuffix(char* suffix)
{
    return strncmp(suffix, java_suffix, java_suffix_length) == 0;
}
#endif

void FileSymbol::SetFileNameLiteral(Control* control)
{
    if (! file_name_literal)
    {
        char* file_name = FileName();

        int i;
        for (i = FileNameLength() - 1; i >= 0; i--)
        {
            if (file_name[i] == U_SLASH)
                break;
        }

        int file_name_start = i + 1;
        int file_name_length = FileNameLength() - file_name_start;
        file_name_literal =
            control -> Utf8_pool.FindOrInsert(file_name + file_name_start,
                                              file_name_length);
    }
}


void FileSymbol::CleanUp()
{
    delete lex_stream;
    lex_stream = NULL;

    if (compilation_unit)
    {
        // NOTE: The ast_pool is handled separately:
        // - For successful compilations, it's deleted early in Control::CleanUp
        // - For files with errors, it's deleted in Control's destructor
        // Here we just clear the pointer since the pool is managed elsewhere.
        compilation_unit = NULL;
    }

    delete semantic;
    semantic = NULL;
}


FileLocation::FileLocation(LexStream* lex_stream, TokenIndex token_index)
{
    char* file_name = lex_stream -> FileName();
    unsigned length = lex_stream -> FileNameLength();
    location = new wchar_t[length + 13];
    for (unsigned i = 0; i < length; i++)
        location[i] = (wchar_t) file_name[i];
    location[length++] = U_COLON;

    IntToWstring line_no(lex_stream -> Line(token_index));

    for (int j = 0; j < line_no.Length(); j++)
        location[length++] = line_no.String()[j];
    location[length] = U_NULL;
}


void TypeSymbol::SetClassName()
{
    size_t length;

    if (semantic_environment -> sem -> control.option.directory)
    {
        DirectorySymbol* output_directory = file_symbol -> OutputDirectory();
        int directory_length = output_directory -> DirectoryNameLength();
        char* directory_name = output_directory -> DirectoryName();
        length = directory_length + ExternalUtf8NameLength() +
            FileSymbol::class_suffix_length + 1; // +1 for /
        class_name = new char[length + 1]; // +1 for '\0'

        strcpy(class_name, directory_name);

        if (directory_name[directory_length - 1] != U_SLASH)
            strcat(class_name, StringConstant::U8S_SL);
    }
    else
    {
        char* file_name =
            semantic_environment -> sem -> lex_stream -> FileName();
        int n;
        for (n = semantic_environment -> sem -> lex_stream ->
                 FileNameLength() - 1;
             n >= 0; n--)
        {
            if (file_name[n] == U_SLASH)
                break;
        }
        n++;

        length =
            n + ExternalUtf8NameLength() + FileSymbol::class_suffix_length;
        class_name = new char[length + 1]; // +1 for '\0'
        strncpy(class_name, file_name, n);
        class_name[n] = U_NULL;
    }

    strcat(class_name, ExternalUtf8Name());
    strcat(class_name, FileSymbol::class_suffix);

    assert(strlen(class_name) <= length);
}


void TypeSymbol::ProcessNestedTypeSignatures(Semantic* sem, TokenIndex tok)
{
    for (unsigned i = 0; i < NumNestedTypeSignatures(); i++)
    {
        NameSymbol* name_symbol = sem ->
            control.ConvertUtf8ToUnicode(NestedTypeSignature(i),
                                         strlen(NestedTypeSignature(i)));
        delete [] NestedTypeSignature(i);
        sem -> ProcessNestedType(this, name_symbol, tok);
    }

    delete nested_type_signatures;
    nested_type_signatures = NULL;
}


void MethodSymbol::ProcessMethodThrows(Semantic* sem, TokenIndex tok)
{
    if (throws_signatures)
    {
        assert(sem);

        //
        // Process throws clause
        //
        for (unsigned i = 0; i < NumThrowsSignatures(); i++)
        {
            TypeSymbol* type =
                sem -> ReadTypeFromSignature(containing_type,
                                             ThrowsSignature(i),
                                             strlen(ThrowsSignature(i)),
                                             tok);
            AddThrows(type);
            delete [] ThrowsSignature(i);
        }

        delete throws_signatures;
        throws_signatures = NULL;
    }
}


//
// In addition to (re)setting the signature, this updates the
// max_variable_index if needed.
//
void MethodSymbol::SetSignature(Control& control, TypeSymbol* placeholder)
{
    unsigned i;
    bool is_constructor = Identity() == control.init_name_symbol;
    int len = is_constructor ? 3 : 2 + strlen(Type() -> SignatureString());
    // +1 for '(' +1 for ')'; constructors have type 'V'

    TypeSymbol* this0_type = containing_type -> EnclosingType();
    int variable_index = ACC_STATIC() ? 0 : 1;

    if (is_constructor && this0_type)
    {
        len += strlen(this0_type -> SignatureString());
        variable_index++;
    }
    for (i = 0; i < NumFormalParameters(); i++)
    {
        TypeSymbol* formal_type = FormalParameter(i) -> Type();
        len += strlen(formal_type -> SignatureString());
        variable_index += (control.IsDoubleWordType(formal_type) ? 2 : 1);
    }
    if (is_constructor)
    {
        for (i = 0; i < containing_type -> NumConstructorParameters(); i++)
        {
            TypeSymbol* shadow_type =
                containing_type -> ConstructorParameter(i) -> Type();
            len += strlen(shadow_type -> SignatureString());
            variable_index += (control.IsDoubleWordType(shadow_type) ? 2 : 1);
        }
        if (placeholder)
        {
            len += strlen(placeholder -> SignatureString());
            variable_index++;
        }
    }
    if (block_symbol && variable_index > block_symbol -> max_variable_index)
        block_symbol -> max_variable_index = variable_index;

    char* method_signature = new char[len + 1]; // +1 for '\0'
    method_signature[0] = U_LEFT_PARENTHESIS;
    int s = 1;
    if (is_constructor && this0_type)
    {
        for (const char* str = this0_type -> SignatureString();
             *str; str++, s++)
        {
            method_signature[s] = *str;
        }
    }
    for (i = 0; i < NumFormalParameters(); i++)
    {
        TypeSymbol* formal_type = FormalParameter(i) -> Type();
        for (const char* str = formal_type -> SignatureString();
             *str; str++, s++)
        {
            method_signature[s] = *str;
        }
    }
    if (is_constructor)
    {
        for (i = 0; i < containing_type -> NumConstructorParameters(); i++)
        {
            TypeSymbol* shadow_type =
                containing_type -> ConstructorParameter(i) -> Type();
            for (const char* str = shadow_type -> SignatureString();
                 *str; str++, s++)
            {
                method_signature[s] = *str;
            }
        }
        if (placeholder)
            for (const char* str = placeholder -> SignatureString();
                 *str; str++, s++)
            {
                method_signature[s] = *str;
            }
    }
    method_signature[s++] = U_RIGHT_PARENTHESIS;
    if (is_constructor)
    {
        assert(Type() == containing_type);
        method_signature[s++] = U_V;
    }
    else
    {
        for (const char* str = Type() -> SignatureString(); *str; str++, s++)
            method_signature[s] = *str;
    }
    method_signature[s] = U_NULL;

    signature = control.Utf8_pool.FindOrInsert(method_signature, len);

    delete [] method_signature;
}


//
// Helper function to check if a name matches a method type parameter.
// Returns the type parameter index if found, -1 otherwise.
//
static int FindMethodTypeParameter(AstName* name, MethodSymbol* method, LexStream* lex_stream)
{
    if (! name || ! method || ! lex_stream)
        return -1;

    const NameSymbol* name_sym = lex_stream -> NameSymbol(name -> identifier_token);
    if (! name_sym)
        return -1;

    for (unsigned i = 0; i < method -> NumTypeParameters(); i++)
    {
        TypeParameterSymbol* type_param = method -> TypeParameter(i);
        if (type_param -> name_symbol == name_sym)
            return (int) i;
    }
    return -1;
}

//
// Helper function to generate the generic type signature from an AST type node.
// This handles type parameters, parameterized types, arrays, and wildcards.
//
static void GenerateTypeSignatureFromAst(AstType* type, MethodSymbol* method,
                                         LexStream* lex_stream,
                                         char* buffer, unsigned& length)
{
    if (! type)
        return;

    // Handle array types
    AstArrayType* array_type = type -> ArrayTypeCast();
    if (array_type)
    {
        // Add '[' for each dimension
        unsigned dims = array_type -> NumBrackets();
        for (unsigned d = 0; d < dims; d++)
            buffer[length++] = '[';
        // Recursively handle element type
        GenerateTypeSignatureFromAst(array_type -> type, method, lex_stream, buffer, length);
        return;
    }

    // Handle type names (including parameterized types and type parameter references)
    AstTypeName* type_name = type -> TypeNameCast();
    if (type_name)
    {
        // First check if this is a type parameter reference (simple name, no base, no type args)
        if (type_name -> name && ! type_name -> base_opt && ! type_name -> type_arguments_opt)
        {
            // Check if this name matches one of the method's type parameters by name
            int type_param_idx = FindMethodTypeParameter(type_name -> name, method, lex_stream);
            if (type_param_idx >= 0)
            {
                // This is a reference to a method type parameter
                TypeParameterSymbol* type_param = method -> TypeParameter(type_param_idx);
                buffer[length++] = 'T';
                const char* name = type_param -> Utf8Name();
                unsigned name_len = type_param -> Utf8NameLength();
                for (unsigned j = 0; j < name_len; j++)
                    buffer[length++] = name[j];
                buffer[length++] = ';';
                return;
            }
        }

        // Not a type parameter reference, use the resolved type
        TypeSymbol* resolved = type_name -> symbol;
        if (! resolved)
            return;

        // Check if this is a parameterized type with type arguments
        if (type_name -> type_arguments_opt && type_name -> type_arguments_opt -> NumTypeArguments() > 0)
        {
            // Generate parameterized type signature: LClassName<TypeArgs>;
            const char* full_sig = resolved -> SignatureString();
            unsigned sig_len = strlen(full_sig);

            // Copy everything except trailing ';'
            for (unsigned i = 0; i < sig_len - 1; i++)
                buffer[length++] = full_sig[i];

            // Add type arguments
            buffer[length++] = '<';
            for (unsigned i = 0; i < type_name -> type_arguments_opt -> NumTypeArguments(); i++)
            {
                AstType* arg = type_name -> type_arguments_opt -> TypeArgument(i);
                GenerateTypeSignatureFromAst(arg, method, lex_stream, buffer, length);
            }
            buffer[length++] = '>';
            buffer[length++] = ';';
        }
        else
        {
            // Simple type, just use its signature
            const char* sig = resolved -> SignatureString();
            unsigned sig_len = strlen(sig);
            for (unsigned i = 0; i < sig_len; i++)
                buffer[length++] = sig[i];
        }
        return;
    }

    // Handle wildcards
    AstWildcard* wildcard = type -> WildcardCast();
    if (wildcard)
    {
        if (wildcard -> extends_token_opt)
        {
            buffer[length++] = '+';
            GenerateTypeSignatureFromAst(wildcard -> bounds_opt, method, lex_stream, buffer, length);
        }
        else if (wildcard -> super_token_opt)
        {
            buffer[length++] = '-';
            GenerateTypeSignatureFromAst(wildcard -> bounds_opt, method, lex_stream, buffer, length);
        }
        else
        {
            buffer[length++] = '*';
        }
        return;
    }

    // Handle primitive types
    AstPrimitiveType* primitive = type -> PrimitiveTypeCast();
    if (primitive && primitive -> symbol)
    {
        const char* sig = primitive -> symbol -> SignatureString();
        unsigned sig_len = strlen(sig);
        for (unsigned i = 0; i < sig_len; i++)
            buffer[length++] = sig[i];
    }
}

//
// Generate the generic signature for a generic method (Signature attribute)
// Format: TypeParameters? (TypeSignature*) ReturnDescriptor ThrowsSignature*
//
void MethodSymbol::SetGenericSignature(Control& control)
{
    // Methods need signature if they have:
    // 1. Method type parameters (NumTypeParameters() > 0), OR
    // 2. Return type is a class type parameter (return_type_param_index >= 0)
    if (NumTypeParameters() == 0 && return_type_param_index < 0)
        return;

    // Estimate size: type parameters + parameters + return type
    unsigned estimated_size = 1000;
    char* buffer = new char[estimated_size];
    unsigned length = 0;

    // Get lex_stream from containing type's semantic environment
    LexStream* lex_stream = NULL;
    if (containing_type && containing_type -> semantic_environment &&
        containing_type -> semantic_environment -> sem)
    {
        lex_stream = containing_type -> semantic_environment -> sem -> lex_stream;
    }

    // Add type parameters if present: <T:Ljava/lang/Object;>
    if (NumTypeParameters() > 0)
    {
        buffer[length++] = '<';
        for (unsigned i = 0; i < NumTypeParameters(); i++)
        {
            TypeParameterSymbol* type_param = TypeParameter(i);
            type_param -> GenerateSignature(control);
            const char* param_sig = type_param -> signature_string;
            unsigned param_len = type_param -> signature_length;
            for (unsigned j = 0; j < param_len; j++)
                buffer[length++] = param_sig[j];
        }
        buffer[length++] = '>';
    }

    // Add parameter types: (TypeSignature*)
    // We need to use the AST to preserve type argument information
    buffer[length++] = '(';

    // Try to get parameter types from AST declaration
    AstMethodDeclaration* method_decl = declaration ? declaration -> MethodDeclarationCast() : NULL;
    AstMethodDeclarator* declarator = method_decl ? method_decl -> method_declarator : NULL;

    for (unsigned i = 0; i < NumFormalParameters(); i++)
    {
        bool used_ast = false;

        // If we have AST info and this parameter uses a method type parameter,
        // generate the signature from AST to preserve type arguments
        if (declarator && lex_stream && i < declarator -> NumFormalParameters())
        {
            AstFormalParameter* formal = declarator -> FormalParameter(i);
            if (formal && formal -> type)
            {
                // Check if this parameter uses a method type parameter
                bool uses_method_type_param = false;
                if (param_type_param_indices && i < param_type_param_indices -> Length())
                {
                    if ((*(param_type_param_indices))[i] >= 0)
                        uses_method_type_param = true;
                }

                if (uses_method_type_param)
                {
                    GenerateTypeSignatureFromAst(formal -> type, this, lex_stream, buffer, length);
                    used_ast = true;
                }
            }
        }

        if (! used_ast)
        {
            // Fall back to erased type signature
            TypeSymbol* param_type = FormalParameter(i) -> Type();
            const char* param_sig = param_type -> SignatureString();
            unsigned param_len = strlen(param_sig);
            for (unsigned j = 0; j < param_len; j++)
                buffer[length++] = param_sig[j];
        }
    }
    buffer[length++] = ')';

    // Add return type
    bool is_constructor = Identity() == control.init_name_symbol;
    if (is_constructor)
    {
        buffer[length++] = 'V';
    }
    else if (method_return_type_param_index >= 0)
    {
        // Return type is a method type parameter - use TName; format
        TypeParameterSymbol* type_param = TypeParameter(method_return_type_param_index);
        buffer[length++] = 'T';
        const char* name = type_param -> Utf8Name();
        unsigned name_len = type_param -> Utf8NameLength();
        for (unsigned i = 0; i < name_len; i++)
            buffer[length++] = name[i];
        buffer[length++] = ';';
    }
    else if (return_type_param_index >= 0 && containing_type)
    {
        // Return type is a class type parameter - use TName; format
        // Get type parameter from containing type
        TypeParameterSymbol* type_param = containing_type -> TypeParameter(return_type_param_index);
        if (type_param)
        {
            buffer[length++] = 'T';
            const char* name = type_param -> Utf8Name();
            unsigned name_len = type_param -> Utf8NameLength();
            for (unsigned i = 0; i < name_len; i++)
                buffer[length++] = name[i];
            buffer[length++] = ';';
        }
        else
        {
            // Fallback to erased type
            TypeSymbol* return_type = Type();
            const char* return_sig = return_type -> SignatureString();
            unsigned return_len = strlen(return_sig);
            for (unsigned i = 0; i < return_len; i++)
                buffer[length++] = return_sig[i];
        }
    }
    else if (method_decl && method_decl -> type && lex_stream)
    {
        // Try to use AST for return type to preserve any parameterized type info
        GenerateTypeSignatureFromAst(method_decl -> type, this, lex_stream, buffer, length);
    }
    else
    {
        TypeSymbol* return_type = Type();
        const char* return_sig = return_type -> SignatureString();
        unsigned return_len = strlen(return_sig);
        for (unsigned i = 0; i < return_len; i++)
            buffer[length++] = return_sig[i];
    }

    // TODO: Add throws signatures

    // Null terminate and store
    buffer[length] = '\0';
    generic_signature = control.Utf8_pool.FindOrInsert(buffer, length);
    delete [] buffer;
}


void MethodSymbol::ProcessMethodSignature(Semantic* sem,
                                          TokenIndex token_location)
{
    if (! type_)
    {
        assert(sem);

        int num_parameters = 0;
        const char* signature = SignatureString();

        // Generic methods have type parameters before the method parameters
        // Signature format: <TypeParams>(Params)ReturnType
        // We need to skip past the type parameters to get to the method params
        if (*signature == U_LT)
        {
            // Skip over generic type parameters: <T:Ljava/lang/Object;U:...>
            int depth = 1;
            signature++;
            while (*signature && depth > 0)
            {
                if (*signature == U_LT)
                    depth++;
                else if (*signature == U_GT)
                    depth--;
                signature++;
            }
        }

        assert(*signature == U_LEFT_PARENTHESIS);
        signature++; // +1 to skip initial '('

        //
        // For the constructor of an inner type, skip the "this$0" argument.
        //
        if (containing_type -> EnclosingType() &&
            ! containing_type -> EnclosingType() -> ACC_PRIVATE() &&
            Identity() == sem -> control.init_name_symbol)
        {
            TypeSymbol* enclosing = sem -> ProcessSignature(containing_type,
                                                            signature,
                                                            token_location);
            assert(enclosing == containing_type -> EnclosingType());
            (void)enclosing;
        }

        while (*signature && *signature != U_RIGHT_PARENTHESIS)
        {
            //
            // Make up a name for each parameter.
            //
            NameSymbol* name_symbol =
                sem -> control.MakeParameter(++num_parameters);
            VariableSymbol* symbol = new VariableSymbol(name_symbol);
            symbol -> SetType(sem -> ProcessSignature(containing_type,
                                                      signature,
                                                      token_location));
            symbol -> MarkComplete();
            AddFormalParameter(symbol);
        }
        assert(*signature == U_RIGHT_PARENTHESIS);
        signature++; // skip ')'

        //
        // Now set the type of the method.
        //
        if (Identity() == sem -> control.init_name_symbol)
        {
            assert(*signature++ == U_V);
            SetType(containing_type);
        }
        else
        {
            SetType(sem -> ProcessSignature(containing_type, signature,
                                            token_location));
        }
        assert(! *signature);

        //
        // Create a symbol table for this method for consistency, and in
        // order to release the space used by the variable paramaters later.
        //
        BlockSymbol* block_symbol = new BlockSymbol(num_parameters);
        for (int k = 0; k < num_parameters; k++)
            block_symbol -> InsertVariableSymbol((*formal_parameters)[k]);
        block_symbol -> CompressSpace(); // space optimization
        SetBlockSymbol(block_symbol);
    }
}


void MethodSymbol::CleanUp()
{
    BlockSymbol* block = new BlockSymbol(NumFormalParameters());

    //
    // Make a copy of each parameter into the new pared-down symbol table and
    // fix the FormalParameter information to identify the new symbol.
    //
    for (unsigned k = 0; k < NumFormalParameters(); k++)
    {
        VariableSymbol* formal_parameter = (*formal_parameters)[k];
        VariableSymbol* symbol =
            block -> InsertVariableSymbol(formal_parameter -> Identity());
        symbol -> SetType(formal_parameter -> Type());
        symbol -> MarkComplete();
        (*formal_parameters)[k] = symbol;
    }

    //
    // Unlink any local/anonymous types from their parent types' subtypes sets
    // before destroying the block. This prevents dangling pointers.
    //
    if (block_symbol)
        block_symbol->UnlinkTypesRecursively();

    //
    // Destroy the old symbol and replace it by the new one.
    //
    delete block_symbol;
    block -> CompressSpace(); // space optimization
    SetBlockSymbol(block);

    declaration = NULL; // remove reference to Ast structure
}


int VariableSymbol::LocalVariableIndex(Semantic* sem)
{
    if (IsLocal(sem -> ThisMethod()))
    {
        assert(sem -> FinalFields());
        return local_variable_index + sem -> FinalFields() -> Length();
    }
    return local_variable_index;
}

void VariableSymbol::SetLocation()
{
    if (! declarator)
    {
        file_location = new FileLocation(ContainingType() -> file_symbol);
    }
    else
    {
        file_location =
            new FileLocation((ContainingType() -> semantic_environment ->
                              sem -> lex_stream),
                             declarator -> LeftToken());
    }
}

void VariableSymbol::ProcessVariableSignature(Semantic* sem,
                                              TokenIndex token_location)
{
    if (! type_)
    {
        assert(sem);
        const char* signature = signature_string;

        SetType(sem -> ProcessSignature((TypeSymbol*) owner, signature,
                                        token_location));
        assert(! *signature);
    }
}


bool TypeSymbol::IsNestedIn(TypeSymbol* type)
{
    for (SemanticEnvironment* env = semantic_environment;
         env; env = env -> previous)
    {
        if (env -> Type() == type)
            return true;
    }
    return false;
}


//
// Return the type of an enclosing instance, if this is an inner class
// which is not in a static context. For anonymous and local classes, the
// compiler necessarily built them from source, so enclosing_instance will
// be properly set. Non-static nested classes, however, could have been
// read from a .class file, hence the need for the second half of the ||.
//
TypeSymbol* TypeSymbol::EnclosingType()
{
    if (enclosing_instance || (IsInner() && ! Anonymous() && ! IsLocal()))
    {
        assert(ContainingType());
        return ContainingType();
    }
    return NULL;
}


//
// Check if this type has access to an enclosing instance of the named type.
// If exact is true, the enclosing instance must be the specified type,
// otherwise it is the innermost instance which is a subclass of type.
//
bool TypeSymbol::HasEnclosingInstance(TypeSymbol* type, bool exact)
{
    assert(semantic_environment);
    for (SemanticEnvironment* env = semantic_environment;
         env; env = env -> previous)
    {
        if (exact ? (env -> Type() == type)
            : (env -> Type() -> IsSubclass(type)))
        {
            //
            // We found the innermost candidate type. If we're in a static
            // region (e.g., during an explicit constructor invocation),
            // keep walking outward to see if an already-initialized
            // enclosing instance satisfies the request.
            //
            if (! env -> StaticRegion())
                return true;
            else
                continue;
        }
        if (env -> Type() -> ACC_STATIC()) // No more enclosing levels exist.
            return false;
    }
    return false; // The requested type does not enclose this type.
}


//
// Given two types T and T2 in different packages, the type T can access
// protected members of T2 iff T or any class in which T is lexically enclosed
// is a subclass of T2 or of some other type T3 that lexically encloses T2.
//
// Of course, T2 and all its enclosing classes, if any, must have been declared
// either public or protected, otherwise they could not be eligible as a
// superclass candidate. We do not need to check for that condition here.
//
bool TypeSymbol::HasProtectedAccessTo(TypeSymbol* target_type)
{
    assert(semantic_environment && ! target_type -> IsArray());

    // Loop through T and enclosing classes.
    for (SemanticEnvironment* env = semantic_environment;
         env; env = env -> previous)
    {
        TypeSymbol* main_type = env -> Type();
        // Loop through T2 and enclosing classes.
        for (TypeSymbol* type = target_type;
             type; type = type -> owner -> TypeCast())
        {
            if (main_type -> IsSubclass(type))
                return true;
        }
    }
    return false;
}


TypeSymbol* TypeSymbol::BoxedType(Control& control)
{
    if (! Primitive())
        return this;
    if (this == control.int_type)
        return control.Integer();
    if (this == control.boolean_type)
        return control.Boolean();
    if (this == control.byte_type)
        return control.Byte();
    if (this == control.short_type)
        return control.Short();
    if (this == control.char_type)
        return control.Character();
    if (this == control.long_type)
        return control.Long();
    if (this == control.float_type)
        return control.Float();
    if (this == control.double_type)
        return control.Double();
    assert(this == control.void_type);
    return control.Void();
}


TypeSymbol* TypeSymbol::UnboxedType(Control& control)
{
    if (ContainingPackage() != control.LangPackage())
        return this;
    if (this == control.Integer())
        return control.int_type;
    if (this == control.Boolean())
        return control.boolean_type;
    if (this == control.Byte())
        return control.byte_type;
    if (this == control.Short())
        return control.short_type;
    if (this == control.Character())
        return control.char_type;
    if (this == control.Long())
        return control.long_type;
    if (this == control.Float())
        return control.float_type;
    if (this == control.Double())
        return control.double_type;
    if (this == control.Void())
        return control.void_type;
    return this;
}


VariableSymbol* TypeSymbol::InsertThis0()
{
    assert(IsInner() && ContainingType());

    Control& control = semantic_environment -> sem -> control;

    // No local shadows and no this$0 yet.
    assert(NumConstructorParameters() == 0 && ! enclosing_instance);

    //
    // Create a this0 pointer for an inner class.
    //
    VariableSymbol* variable_symbol =
        InsertVariableSymbol(control.this_name_symbol);
    variable_symbol -> SetType(ContainingType());
    variable_symbol -> SetFlags(ACCESS_FINAL | ACCESS_SYNTHETIC);
    variable_symbol -> SetOwner(this);
    variable_symbol -> MarkComplete();
    variable_symbol -> MarkInitialized();

    enclosing_instance = variable_symbol;
    return variable_symbol;
}


TypeSymbol* TypeSymbol::FindOrInsertClassLiteralClass()
{
    //
    // Normally, the place-holder type for invoking private constructors can
    // be any type, because we just pass null along, avoiding static
    // initialization. But if we use the place-holder type to store the
    // class$() method, we must ensure it is a subclass of Object.
    //
    if (placeholder_type && (placeholder_type -> super !=
                             semantic_environment -> sem -> control.Object()))
        placeholder_type = NULL;
    return GetPlaceholderType();
}


MethodSymbol* TypeSymbol::FindOrInsertClassLiteralMethod(Control& control)
{
    assert(! ACC_INTERFACE());
    if (! class_literal_method)
    {
        //
        // Note that max_variable_index is initialized to 2 (instead of 1),
        // even though the class literal method is static. The reason is that
        // in generating code for this method, a try statement with a catch
        // will be used. Therefore, an extra "local" slot is required for the
        // local Exception parameter of the catch clause. We do not fill in
        // the body of this method here, because bytecode.cpp can do a much
        // more optimal job later. The method has the signature:
        //
        // /*synthetic*/ static Class class$(String name, boolean array);
        //
        BlockSymbol* block_symbol = new BlockSymbol(2);
        block_symbol -> max_variable_index = 2;

        class_literal_method = InsertMethodSymbol(control.class_name_symbol);
        class_literal_method -> SetType(control.Class());
        class_literal_method -> SetFlags(ACCESS_STATIC | ACCESS_SYNTHETIC);
        // No need to worry about strictfp, since this method avoids fp math
        class_literal_method -> SetContainingType(this);
        class_literal_method -> SetBlockSymbol(block_symbol);

        VariableSymbol* variable_symbol =
            block_symbol -> InsertVariableSymbol(control.MakeParameter(1));
        variable_symbol -> SetACC_SYNTHETIC();
        variable_symbol -> SetType(control.String());
        variable_symbol -> SetOwner(class_literal_method);
        variable_symbol -> SetLocalVariableIndex(block_symbol ->
                                                 max_variable_index++);
        variable_symbol -> MarkComplete();
        class_literal_method -> AddFormalParameter(variable_symbol);

        variable_symbol =
            block_symbol -> InsertVariableSymbol(control.MakeParameter(2));
        variable_symbol -> SetACC_SYNTHETIC();
        variable_symbol -> SetType(control.boolean_type);
        variable_symbol -> SetOwner(class_literal_method);
        variable_symbol -> SetLocalVariableIndex(block_symbol ->
                                                 max_variable_index++);
        variable_symbol -> MarkComplete();
        class_literal_method -> AddFormalParameter(variable_symbol);

        class_literal_method -> SetSignature(control);
        semantic_environment -> sem -> AddDependence(this, control.Class());
    }
    return class_literal_method;
}


Utf8LiteralValue* TypeSymbol::FindOrInsertClassLiteralName(Control& control)
{
    if (! class_literal_name)
    {
        int length = fully_qualified_name -> length;
        char* slashed_name = fully_qualified_name -> value;
        char* name = new char[length + 1];
        for (int i = 0; i < length; i++)
            name[i] = (slashed_name[i] == U_SLASH ? (wchar_t) U_DOT
                       : slashed_name[i]);
        name[length] = U_NULL;
        class_literal_name = control.Utf8_pool.FindOrInsert(name, length);
        delete [] name;
    }
    return class_literal_name;
}


VariableSymbol* TypeSymbol::FindOrInsertClassLiteral(TypeSymbol* type)
{
    assert(! type -> Primitive() && ! type -> Anonymous());
    assert(! Primitive() && ! IsArray());

    Semantic* sem = semantic_environment -> sem;
    Control& control = sem -> control;

    //
    // We must be careful that we do not initialize the class literal in
    // question, or any enclosing types. True inner classes can defer to their
    // enclosing class (since code in the inner class cannot be run without
    // the enclosing class being initialized), but static nested types get
    // their own class$ method and cache variables. Interfaces cannot have
    // non-public members, so if the innermost non-local type is an interface,
    // we use the placeholder class to hold the class$ magic.
    //
    TypeSymbol* owner = this;
    while (owner -> IsInner())
        owner = owner -> ContainingType();
    if (owner -> ACC_INTERFACE())
        owner = outermost_type -> FindOrInsertClassLiteralClass();
    owner -> FindOrInsertClassLiteralMethod(control);

    NameSymbol* name_symbol = NULL;
    const char* signature = type -> SignatureString();
    if (signature[0] == U_LEFT_BRACKET) // an array?
    {
        int array_length = control.array_name_symbol -> NameLength();
        int length = strlen(signature) + array_length;
        wchar_t* name = new wchar_t[length + 1]; // +1 for '\0';
        wcscpy(name, control.array_name_symbol -> Name());
        int i;
        int k;
        for (i = 0, k = array_length; signature[i] == U_LEFT_BRACKET; i++, k++)
            name[k] = U_DOLLAR;
        // Leave leading 'L', since there can be conflicts with primitive
        // array types otherwise
        for (wchar_t ch = signature[i++]; ch && ch != U_SEMICOLON;
             ch = signature[i++])
        {
            name[k++] = (ch == U_SLASH ? (wchar_t) U_DOLLAR : ch);
        }
        name[k] = U_NULL;
        name_symbol = control.FindOrInsertName(name, k);
        delete [] name;
    }
    else
    {
        assert(signature[0] == U_L); // a reference type
        int class_length = control.class_name_symbol -> NameLength();
        int length = strlen(signature) + class_length;

        wchar_t* name = new wchar_t[length + 1]; // +1 for '\0';
        wcscpy(name, control.class_name_symbol -> Name());
        int i = 1; // skip leading 'L'
        int k = class_length;
        name[k++] = U_DOLLAR;
        for (wchar_t ch = signature[i++]; ch && ch != U_SEMICOLON;
             ch = signature[i++])
        {
            name[k++] = (ch == U_SLASH ? (wchar_t) U_DOLLAR : ch);
        }
        name[k] = U_NULL;
        name_symbol = control.FindOrInsertName(name, k);
        delete [] name;
    }

    VariableSymbol* variable_symbol = owner -> FindVariableSymbol(name_symbol);
    if (! variable_symbol)
    {
        //
        // Generate a caching variable (no need to make it private, so that
        // nested classes of interfaces can share it easily).
        //
        // Foo.Bar.class is cached in:
        //     /*synthetic*/ static Class class$Foo$Bar;
        // int[][].class is cached in:
        //     /*synthetic*/ static Class array$$I;
        // Blah[].class is cached in:
        //     /*synthetic*/ static Class array$LBlah;
        //
        variable_symbol = owner -> InsertVariableSymbol(name_symbol);
        variable_symbol -> SetType(control.Class());
        variable_symbol -> SetFlags(ACCESS_STATIC | ACCESS_SYNTHETIC);
        variable_symbol -> SetOwner(owner);
        variable_symbol -> MarkComplete();

        owner -> AddClassLiteral(variable_symbol);
    }
    return variable_symbol;
}


VariableSymbol* TypeSymbol::FindOrInsertAssertVariable()
{
    if (! assert_variable)
    {
        assert(! (Primitive() || ACC_INTERFACE() || IsArray()));

        Semantic* sem = semantic_environment -> sem;
        Control& control = sem -> control;

        assert_variable = InsertVariableSymbol(control.assert_name_symbol);
        assert_variable -> SetType(control.boolean_type);
        assert_variable -> SetFlags(ACCESS_PRIVATE | ACCESS_STATIC |
                                    ACCESS_FINAL | ACCESS_SYNTHETIC);
        assert_variable -> SetOwner(this);
        assert_variable -> MarkComplete();
        assert_variable -> MarkInitialized();

        //
        // We'll create the field initializer later in bytecode.cpp, but we
        // create the static initializer that will contain the field
        // initializer now, if it was not already created.
        //
        sem -> GetStaticInitializerMethod();
    }
    return assert_variable;
}


VariableSymbol* TypeSymbol::FindOrInsertLocalShadow(VariableSymbol* local)
{
    assert(IsLocal() && local -> IsLocal());

    Control& control = semantic_environment -> sem -> control;
    VariableSymbol* variable = NULL;
    if (local_shadow_map)
        variable = (VariableSymbol*) local_shadow_map -> Image(local);

    //
    // For a local/anonymous class, if it does not yet have a shadow for a
    // local variable that it needs access to, create one.
    //
    // For example:
    // class Outer {
    //   static void foo(final int i) {
    //     class Local {
    //       Local(int k) { k = i; }
    //     }
    //     new Local(1);
    //   }
    // }
    //
    // expands to:
    // class Outer {
    //   static void foo(final int i) {
    //     new Outer$1Local(1, i);
    //   }
    // }
    // class Outer$1Local {
    //   /*synthetic*/ final int val$i;
    //   Outer$1Local(int k, int i) {
    //     val$i = i;
    //     super();
    //     k = val$i;
    //   }
    // }
    //
    // This method creates Outer$1Local.val$i in the above example.  Notice
    // that JVMS 4.9.4 permits initialization of synthetic fields BEFORE the
    // explicit constructor invocation, even though it would not normally be
    // valid Java; this is necessary for the case when the superconstructor
    // calls a polymorphic method which references i.
    //
    // Note that we must mangle the shadow with val$, because of this case:
    // void foo(final int i) {
    //   class Local { int j = i; }
    //   new Local() { int i; };
    // }
    //
    // In 1.5 and later, we use the prefix "-" instead of "val$".
    //
    if (! variable)
    {
        int length = control.val_name_symbol -> NameLength() +
            local -> NameLength();
        wchar_t* name = new wchar_t[length + 1]; // +1 for '\0';
        wcscpy(name, control.val_name_symbol -> Name());
        wcscat(name, local -> Name());
        NameSymbol* name_symbol = control.FindOrInsertName(name, length);

        variable = InsertVariableSymbol(name_symbol);
        variable -> SetType(local -> Type());
        // Also copy parameterized_type for generic type substitution
        variable -> parameterized_type = local -> parameterized_type;
        variable -> SetFlags(ACCESS_FINAL | ACCESS_SYNTHETIC);
        variable -> SetOwner(this);
        variable -> MarkComplete();
        variable -> MarkInitialized();

        if (ContainingType() == local -> ContainingType())
            variable -> accessed_local = local;
        else
        {
            assert(Anonymous() && ! EnclosingType());
            variable -> accessed_local = semantic_environment -> sem ->
                FindLocalVariable(local, ContainingType());
        }
        AddConstructorParameter(variable);

        delete [] name;

        if (! local_shadow_map)
            local_shadow_map = new SymbolMap();
        local_shadow_map -> Map(local, variable);
    }

#ifdef JOPA_DEBUG
    VariableSymbol* accessed;
    for (accessed = variable -> accessed_local;
         accessed && accessed != local;
         accessed = accessed -> accessed_local);
    assert(accessed);
#endif // JOPA_DEBUG
    return variable;
}


inline void TypeSymbol::MapSymbolToReadMethod(Symbol* symbol,
                                              TypeSymbol* base_type,
                                              MethodSymbol* method)
{
    if (! read_methods)
        // default size
        read_methods = new Map<Symbol, Map<TypeSymbol, MethodSymbol> >();

    Map<TypeSymbol, MethodSymbol>* map = read_methods -> Image(symbol);
    if (! map)
    {
        map = new Map<TypeSymbol, MethodSymbol>(1); // small size
        read_methods -> Add(symbol, map);
    }

    map -> Add(base_type, method);
}

inline MethodSymbol* TypeSymbol::ReadMethod(Symbol* symbol,
                                            TypeSymbol* base_type)
{
    if (read_methods)
    {
        Map<TypeSymbol, MethodSymbol>* map = read_methods -> Image(symbol);
        if (map)
            return map -> Image(base_type);
    }
    return NULL;
}

inline void TypeSymbol::MapSymbolToWriteMethod(VariableSymbol* symbol,
                                               TypeSymbol* base_type,
                                               MethodSymbol* method)
{
    if (! write_methods)
        write_methods = new Map<VariableSymbol,
            Map<TypeSymbol, MethodSymbol> >(); // default size

    Map<TypeSymbol, MethodSymbol>* map = write_methods -> Image(symbol);
    if (! map)
    {
        map = new Map<TypeSymbol, MethodSymbol>(1); // small size
        write_methods -> Add(symbol, map);
    }

    map -> Add(base_type, method);
}

inline MethodSymbol* TypeSymbol::WriteMethod(VariableSymbol* symbol,
                                             TypeSymbol* base_type)
{
    if (write_methods)
    {
        Map<TypeSymbol, MethodSymbol>* map = write_methods -> Image(symbol);
        if (map)
            return map -> Image(base_type);
    }
    return NULL;
}

MethodSymbol* TypeSymbol::GetReadAccessMethod(MethodSymbol* member,
                                              TypeSymbol* base_type)
{
    // accessing a method
    assert(member -> Identity() !=
           semantic_environment -> sem -> control.init_name_symbol);

    TypeSymbol* containing_type = member -> containing_type;
    if (! base_type)
        base_type = this;

    assert((member -> ACC_PRIVATE() && this == containing_type) ||
           (member -> ACC_PROTECTED() &&
            ! semantic_environment -> sem -> ProtectedAccessCheck(containing_type)) ||
           (base_type == super && ! member -> ACC_STATIC()));
    (void)containing_type;

    MethodSymbol* read_method = ReadMethod(member, base_type);

    if (! read_method)
    {
        //
        // BaseType is the qualifying type of we are accessing.  If the method
        // is private, BaseType should be this type, but for protected
        // variables, BaseType should be a superclass or subclass of this type
        // that is not in this package.
        //
        // To access
        // "static Type name(Type1 p1, Type2 p2, ...) throws Exception;",
        // expand to:
        //
        // /*synthetic*/ static Type access$<num>(Type1 p1, Type2 p2, ...)
        // throws Exception
        // {
        //     return BaseType.name(p1, p2, ...);
        // }
        //
        // If we are accessing
        // "void name(Type1 p1, Type2 p2, ...) throws Throwable;",
        // expand to:
        //
        // /*synthetic*/ static void access$<num>(BaseType $0, Type1 p1,
        //                                        Type2 p2, ...)
        // throws Throwable
        // {
        //     $0.name(p1, p2, ...);
        //     return;
        // }
        //
        // In 1.5 and later, we use the prefix "-" instead of "access$".
        //
        Semantic* sem = semantic_environment -> sem;
        assert(sem);

        Control& control = sem -> control;
        StoragePool* ast_pool = sem -> compilation_unit -> ast_pool;

        IntToWstring value(NumPrivateAccessMethods());

        int length = control.access_name_symbol -> NameLength() +
            value.Length();
        wchar_t* name = new wchar_t[length + 1]; // +1 for '\0';
        wcscpy(name, control.access_name_symbol -> Name());
        wcscat(name, value.String());

        //
        // Use the location of the class name for all elements of this method.
        //
        TokenIndex loc = declaration -> identifier_token;

        unsigned parameter_count = member -> NumFormalParameters();

        //
        // Add the method instead of inserting it, so it is not an overload
        // candidate.
        //
        read_method = new MethodSymbol(control.FindOrInsertName(name, length));
        Table() -> AddMethodSymbol(read_method);
        read_method -> SetType(member -> Type());
        read_method -> SetFlags(ACCESS_STATIC | ACCESS_SYNTHETIC);
        if (member -> ACC_STRICTFP())
            read_method -> SetACC_STRICTFP();
        if (member -> ACC_FINAL() || ACC_FINAL())
            read_method -> SetACC_FINAL();
        read_method -> SetContainingType(this);

        //
        // A read access method for a method has a formal parameter per
        // parameter of the member in question, plus one more if it is not
        // static.
        //
        BlockSymbol* block_symbol =
            new BlockSymbol(parameter_count +
                            (member -> ACC_STATIC() ? 0 : 1));
        block_symbol -> max_variable_index = 0;
        read_method -> SetBlockSymbol(block_symbol);
        for (unsigned j = 0; j < member -> NumThrows(); j++)
            read_method -> AddThrows(member -> Throws(j));

        AstExpression* base;
        if (! member -> ACC_STATIC() && base_type == super)
        {
            //
            // Special case - for Outer.super.m() where m() is an instance
            // method, we mark the field access as a super access, to
            // make sure we emit invokespecial instead of invokevirtual in
            // bytecode.cpp.  Notice that in this case,
            // ((Super) Outer.this).m() cannot generate an accessor method
            // (either m() is public or in the same package and thus already
            // accessible, or m is protected in a different package and
            // therefore inaccessible), so we don't have to worry about a
            // conflict in accessor methods for the same base type.
            //
            base = ast_pool -> GenSuperExpression(loc);
        }
        else base = ast_pool -> GenName(loc);

        AstArguments* args = ast_pool -> GenArguments(loc, loc);
        args -> AllocateArguments(parameter_count);

        AstMethodInvocation* method_invocation =
            ast_pool -> GenMethodInvocation(loc);
        method_invocation -> base_opt = base;
        method_invocation -> arguments = args;
        method_invocation -> symbol = member;

        AstMethodDeclarator* method_declarator =
            ast_pool -> GenMethodDeclarator();
        method_declarator -> identifier_token = loc;
        method_declarator -> left_parenthesis_token = loc;
        method_declarator -> right_parenthesis_token = loc;

        if (member -> ACC_STATIC())
        {
            method_declarator -> AllocateFormalParameters(parameter_count);
            base -> symbol = base_type;
        }
        else
        {
            method_declarator -> AllocateFormalParameters(parameter_count + 1);
            NameSymbol* instance_name = control.MakeParameter(1);

            VariableSymbol* instance =
                block_symbol -> InsertVariableSymbol(instance_name);
            instance -> SetACC_SYNTHETIC();
            instance -> SetType(base_type == super ? this : base_type);
            instance -> SetOwner(read_method);
            instance -> SetLocalVariableIndex(block_symbol ->
                                              max_variable_index++);
            instance -> MarkComplete();
            read_method -> AddFormalParameter(instance);
            base -> symbol = (base_type == super
                              ? (Symbol*) super : (Symbol*) instance);
        }

        for (unsigned i = 0; i < parameter_count; i++)
        {
            VariableSymbol* parm = block_symbol ->
                InsertVariableSymbol(member -> FormalParameter(i) -> Identity());
            parm -> SetACC_SYNTHETIC();
            parm -> SetType(member -> FormalParameter(i) -> Type());
            parm -> SetOwner(read_method);
            parm -> SetLocalVariableIndex(block_symbol ->
                                          max_variable_index++);
            parm -> MarkComplete();
            if (control.IsDoubleWordType(parm -> Type()))
                block_symbol -> max_variable_index++;
            read_method -> AddFormalParameter(parm);

            AstName* simple_name = ast_pool -> GenName(loc);
            simple_name -> symbol = parm;
            args -> AddArgument(simple_name);
        }
        read_method -> SetSignature(control);

        AstReturnStatement* return_statement =
            ast_pool -> GenReturnStatement();
        return_statement -> return_token = loc;
        return_statement -> semicolon_token = loc;
        return_statement -> is_reachable = true;

        AstMethodBody* block = ast_pool -> GenMethodBody();
        block -> left_brace_token = loc;
        block -> right_brace_token = loc;
        // the symbol table associated with this block will contain no element
        block -> block_symbol = new BlockSymbol(0);
        ast_pool -> RegisterBlockSymbol(block -> block_symbol);
        block -> is_reachable = true;

        if (member -> Type() == control.void_type)
        {
            AstExpressionStatement* expression_statement =
                ast_pool -> GenExpressionStatement();
            expression_statement -> expression = method_invocation;
            expression_statement -> semicolon_token_opt = loc;
            expression_statement -> is_reachable = true;
            expression_statement -> can_complete_normally = true;

            block -> AllocateStatements(2);
            block -> AddStatement(expression_statement);
        }
        else
        {
            return_statement -> expression_opt = method_invocation;
            block -> AllocateStatements(1);
        }
        block -> AddStatement(return_statement);

        AstMethodDeclaration* method_declaration =
            ast_pool -> GenMethodDeclaration();
        method_declaration -> method_symbol = read_method;
        method_declaration -> method_declarator = method_declarator;
        method_declaration -> method_body_opt = block;

        read_method -> declaration = method_declaration;
        read_method -> accessed_member = member;
        MapSymbolToReadMethod(member, base_type, read_method);
        AddPrivateAccessMethod(read_method);

        delete [] name;
    }
    return read_method;
}

MethodSymbol* TypeSymbol::GetReadAccessConstructor(MethodSymbol* ctor)
{
    //
    // Protected superconstructors are always accessible, and class instance
    // creation expressions can only invoke a protected constructor in the
    // current package, where an accessor is not needed. Also, anonymous
    // classes never have a private constructor.
    //
    assert((ctor -> Identity() ==
            semantic_environment -> sem -> control.init_name_symbol) &&
           ctor -> ACC_PRIVATE() && this == ctor -> containing_type &&
           ! Anonymous());

    MethodSymbol* read_method = ReadMethod(ctor, this);

    if (! read_method)
    {
        //
        // There are two cases for accessing a private constructor.  First, as
        // a superclass:
        //
        // class Outer {
        //     private Outer(Type1 $1, Type2 $2, ...) {}
        //     static class Inner extends Outer {
        //         Inner() { super(expr1, expr2, ...); }
        //     }
        // }
        //
        // We must create a synthetic place-holder class, and expand this to:
        // (TODO: can someone come up with a way to do this without a
        // placeholder class?)
        //
        // class Outer {
        //     private Outer(Type1 $1, Type2 $2, ...) {}
        //     /*synthetic*/ Outer(Outer$ $0, Type1 $1, Type2 $2, ...)
        //     {
        //         this($1, $2, ...);
        //     }
        // }
        // /*synthetic*/ class Outer$ {} // placeholder only
        // class Outer$Inner extends Outer {
        //     Outer$Inner() { super((Outer$) null, expr1, expr2, ...); }
        // }
        //
        // The other use is in class instance creation expressions (recall
        // that the default constructor for a private class is private):
        //
        // class Outer {
        //     private class Inner {}
        //     Inner i = new Inner();
        // }
        //
        // Here again, we create a place-holder class for now.  TODO:
        // alternatives have been proposed, such as using a static generator
        // method instead of an alternate constructor.
        //
        // class Outer {
        //     Outer$Inner i = new Outer$Inner(this, (Outer$) null);
        // }
        // /*synthetic*/ class Outer$ {} // placeholder only
        // class Outer$Inner {
        //     private final Outer this$0;
        //     private Outer$Inner(Outer $0) { super(); this$0 = $0; }
        //     /*synthetic*/ Outer$Inner(Outer $0, Outer$ $1) { this($0); }
        // }
        //
        Semantic* sem = semantic_environment -> sem;
        assert(sem);
        //
        // A clone situation exists only when trying to determine a final
        // value for a field. As obtaining a final value does not need an
        // access method, we delay creating the accessor until out of the
        // clone (otherwise, the placeholder type might be incorrect).
        //
        if (sem -> error && sem -> error -> InClone())
            return ctor;

        Control& control = sem -> control;
        StoragePool* ast_pool = sem -> compilation_unit -> ast_pool;

        // +3 to allow for dummy parameter, local variable shadows
        BlockSymbol* block_symbol =
            new BlockSymbol(ctor -> NumFormalParameters() + 3);

        //
        // Add the method instead of inserting it, so it is not an overload
        // candidate.
        //
        read_method = new MethodSymbol(control.init_name_symbol);
        Table() -> AddMethodSymbol(read_method);
        read_method -> SetType(this);
        read_method -> SetContainingType(this);
        read_method -> SetBlockSymbol(block_symbol);
        read_method -> SetACC_SYNTHETIC();
        if (ctor -> ACC_STRICTFP())
            read_method -> SetACC_STRICTFP();

        for (unsigned j = 0; j < ctor -> NumThrows(); j++)
            read_method -> AddThrows(ctor -> Throws(j));

        block_symbol -> max_variable_index = 1;
        read_method -> SetExternalIdentity(ctor -> Identity());

        Ast* declaration = ctor -> declaration;
        AstMethodDeclarator* declarator =
            ((AstConstructorDeclaration*) declaration) -> constructor_declarator;
        assert(declarator);
        TokenIndex loc = declarator -> identifier_token;

        AstMethodDeclarator* method_declarator =
            ast_pool -> GenMethodDeclarator();
        method_declarator -> identifier_token = loc;
        method_declarator -> left_parenthesis_token =
            declarator -> LeftToken();
        method_declarator -> right_parenthesis_token =
            declarator -> RightToken();

        AstArguments* args = ast_pool -> GenArguments(loc, loc);
        args -> AllocateArguments(ctor -> NumFormalParameters());

        AstThisCall* this_call = ast_pool -> GenThisCall();
        this_call -> this_token = loc;
        this_call -> arguments = args;
        this_call -> semicolon_token = loc;
        this_call -> symbol = ctor;

        VariableSymbol* this0_variable = NULL;
        if (EnclosingType())
        {
            this0_variable = block_symbol ->
                InsertVariableSymbol(control.this_name_symbol);
            this0_variable -> SetACC_SYNTHETIC();
            this0_variable -> SetType(ContainingType());
            this0_variable -> SetOwner(read_method);
            this0_variable -> SetLocalVariableIndex(block_symbol ->
                                                    max_variable_index++);
            this0_variable -> MarkComplete();
        }

        //
        // Since private_access_constructors will be compiled (see
        // body.cpp), we must create valid ast_simple_names for its
        // parameters.
        //
        VariableSymbol* parm;
        for (unsigned i = 0; i < ctor -> NumFormalParameters(); i++)
        {
            parm = block_symbol -> InsertVariableSymbol(ctor -> FormalParameter(i) -> Identity());
            parm -> SetACC_SYNTHETIC();
            parm -> SetType(ctor -> FormalParameter(i) -> Type());
            parm -> SetOwner(read_method);
            parm -> SetLocalVariableIndex(block_symbol ->
                                          max_variable_index++);
            parm -> MarkComplete();
            if (control.IsDoubleWordType(parm -> Type()))
                block_symbol -> max_variable_index++;
            read_method -> AddFormalParameter(parm);

            AstVariableDeclaratorId* variable_declarator_name =
                declarator -> FormalParameter(i) -> formal_declarator ->
                variable_declarator_name;
            AstName* simple_name = ast_pool ->
                GenName(variable_declarator_name -> identifier_token);
            simple_name -> symbol = parm;
            args -> AddArgument(simple_name);
        }

        //
        // Any local variable shadow parameters will be taken care of later,
        // possibly changing this signature.
        //
        read_method -> SetSignature(control,
                                    outermost_type -> GetPlaceholderType());

        AstReturnStatement* return_statement =
            ast_pool -> GenReturnStatement();
        return_statement -> return_token = loc;
        return_statement -> semicolon_token = loc;
        return_statement -> is_reachable = true;

        AstMethodBody* constructor_block = ast_pool -> GenMethodBody();
        // This symbol table will be empty.
        constructor_block -> block_symbol = new BlockSymbol(0);
        ast_pool -> RegisterBlockSymbol(constructor_block -> block_symbol);
        constructor_block -> block_symbol -> max_variable_index =
            block_symbol -> max_variable_index;
        constructor_block -> left_brace_token = loc;
        constructor_block -> right_brace_token = loc;
        constructor_block -> AllocateStatements(1);
        constructor_block -> AddStatement(return_statement);
        constructor_block -> explicit_constructor_opt = this_call;

        AstConstructorDeclaration* constructor_declaration =
            ast_pool -> GenConstructorDeclaration();
        constructor_declaration -> constructor_declarator = method_declarator;
        constructor_declaration -> constructor_body = constructor_block;

        constructor_declaration -> constructor_symbol = read_method;
        read_method -> declaration = constructor_declaration;

        AddPrivateAccessConstructor(read_method);

        read_method -> accessed_member = ctor;
        MapSymbolToReadMethod(ctor, this, read_method);
    }
    return read_method;
}


MethodSymbol* TypeSymbol::GetReadAccessMethod(VariableSymbol* member,
                                              TypeSymbol* base_type)
{
    TypeSymbol* containing_type = member -> owner -> TypeCast();
    if (! base_type)
        base_type = this;

    assert((member -> ACC_PRIVATE() && this == containing_type) ||
           (member -> ACC_PROTECTED() &&
            (! semantic_environment -> sem -> ProtectedAccessCheck(containing_type) ||
             (base_type == super && ! member -> ACC_STATIC()))));
    (void)containing_type;

    MethodSymbol* read_method = ReadMethod(member, base_type);

    if (! read_method)
    {
        //
        // BaseType is the qualifying type of we are accessing.  If the
        // variable is private, BaseType should be this type, but for
        // protected variables, BaseType should be a superclass or subclass
        // of this type that is not in this package.
        //
        // If we are accessing "static Type name;", expand to:
        //
        // /*synthetic*/ static Type access$<num>()
        // {
        //     return BaseType.name;
        // }
        //
        // If we are accessing "Type name;", expand to:
        //
        // /*synthetic*/ static Type access$<num>(BaseType $1)
        // {
        //     return $1.name;
        // }
        //
        // In 1.5 and later, we use the prefix "-" instead of "access$".
        //
        Semantic* sem = semantic_environment -> sem;
        assert(sem);

        Control& control = sem -> control;
        StoragePool* ast_pool = sem -> compilation_unit -> ast_pool;

        IntToWstring value(NumPrivateAccessMethods());

        int length = control.access_name_symbol -> NameLength() +
            value.Length();
        wchar_t* name = new wchar_t[length + 1]; // +1 for '\0';
        wcscpy(name, control.access_name_symbol -> Name());
        wcscat(name, value.String());

        //
        // Use the location of the class name for all elements of this method.
        //
        TokenIndex loc = declaration -> identifier_token;

        //
        // Add the method instead of inserting it, so it is not an overload
        // candidate.
        //
        read_method = new MethodSymbol(control.FindOrInsertName(name, length));
        Table() -> AddMethodSymbol(read_method);
        read_method -> SetType(member -> Type());
        read_method -> SetFlags(ACCESS_STATIC | ACCESS_SYNTHETIC);
        if (ACC_STRICTFP())
            read_method -> SetACC_STRICTFP();
        if (ACC_FINAL())
            read_method -> SetACC_FINAL();
        read_method -> SetContainingType(this);

        //
        // A read access method for a field has 1 formal parameter if the
        // member in question is not static
        //
        BlockSymbol* block_symbol =
            new BlockSymbol(member -> ACC_STATIC() ? 0 : 1);
        block_symbol -> max_variable_index = 0;
        read_method -> SetBlockSymbol(block_symbol);

        AstExpression* base;
        if (! member -> ACC_STATIC() && base_type == super)
        {
            //
            // Special case - for Outer.super.i where i is an instance field,
            // we mark the field access as a super access, to make sure we use
            // the correct qualifying instance.  Notice that in this case,
            // ((Super) Outer.this).i cannot generate an accessor method
            // (either i is public or in the same package and thus already
            // accessible, or i is protected in a different package and
            // therefore inaccessible), so we don't have to worry about a
            // conflict in accessor methods for the same base type.
            //
            base = ast_pool -> GenSuperExpression(loc);
        }
        else base = ast_pool -> GenName(loc);

        AstFieldAccess* field_access = ast_pool -> GenFieldAccess();
        field_access -> base = base;
        field_access -> identifier_token = loc;
        field_access -> symbol = member;

        AstMethodDeclarator* method_declarator =
            ast_pool -> GenMethodDeclarator();
        method_declarator -> identifier_token = loc;
        method_declarator -> left_parenthesis_token = loc;
        method_declarator -> right_parenthesis_token = loc;

        if (member -> ACC_STATIC())
        {
            base -> symbol = base_type;
        }
        else
        {
            method_declarator -> AllocateFormalParameters(1);

            NameSymbol* instance_name = control.MakeParameter(1);

            VariableSymbol* instance =
                block_symbol -> InsertVariableSymbol(instance_name);
            instance -> SetACC_SYNTHETIC();
            instance -> SetType(base_type == super ? this : base_type);
            instance -> SetOwner(read_method);
            instance -> SetLocalVariableIndex(block_symbol ->
                                              max_variable_index++);
            instance -> MarkComplete();
            read_method -> AddFormalParameter(instance);
            base -> symbol = (base_type == super
                              ? (Symbol*) super : (Symbol*) instance);
        }

        // A read access method has no throws clause !
        read_method -> SetSignature(control);

        AstReturnStatement* return_statement =
            ast_pool -> GenReturnStatement();
        return_statement -> return_token = loc;
        return_statement -> expression_opt = field_access;
        return_statement -> semicolon_token = loc;
        return_statement -> is_reachable = true;

        AstMethodBody* block = ast_pool -> GenMethodBody();
        block -> left_brace_token = loc;
        block -> right_brace_token = loc;
        block -> block_symbol = new BlockSymbol(0);
        ast_pool -> RegisterBlockSymbol(block -> block_symbol);
        block -> is_reachable = true;
        block -> AllocateStatements(1);
        block -> AddStatement(return_statement);

        AstMethodDeclaration* method_declaration =
            ast_pool -> GenMethodDeclaration();
        method_declaration -> method_symbol = read_method;
        method_declaration -> method_declarator = method_declarator;
        method_declaration -> method_body_opt = block;

        read_method -> declaration = method_declaration;
        read_method -> accessed_member = member;
        MapSymbolToReadMethod(member, base_type, read_method);
        AddPrivateAccessMethod(read_method);

        delete [] name;
    }
    return read_method;
}


MethodSymbol* TypeSymbol::GetWriteAccessMethod(VariableSymbol* member,
                                               TypeSymbol* base_type)
{
    TypeSymbol* containing_type = member -> owner -> TypeCast();
    if (! base_type)
        base_type = this;

    assert((member -> ACC_PRIVATE() && this == containing_type) ||
           (member -> ACC_PROTECTED() &&
            (! semantic_environment -> sem -> ProtectedAccessCheck(containing_type) ||
             (base_type == super && ! member -> ACC_STATIC()))));
    (void)containing_type;

    MethodSymbol* write_method = WriteMethod(member, base_type);

    if (! write_method)
    {
        //
        // BaseType is the qualifying type of we are accessing.  If the
        // variable is private, BaseType should be this type, but for
        // protected variables, BaseType should be a superclass or subclass
        // of this type that is not in this package.
        //
        // If we are accessing "static Type name;", expand to:
        //
        // /*synthetic*/ static void access$<num>(Type name)
        // {
        //     BaseType.name = name;
        //     return;
        // }
        //
        // If we are accessing "Type name;", expand to:
        //
        // /*synthetic*/ static void access$<num>(BaseType $1, Type name)
        // {
        //     $1.name = name;
        //     return;
        // }
        //
        // In 1.5 and later, we use the prefix "-" instead of "access$".
        //
        Semantic* sem = semantic_environment -> sem;
        assert(sem);

        Control& control = sem -> control;
        StoragePool* ast_pool = sem -> compilation_unit -> ast_pool;

        IntToWstring value(NumPrivateAccessMethods());

        int length = control.access_name_symbol -> NameLength() +
            value.Length();
        wchar_t* name = new wchar_t[length + 1]; // +1 for '\0';
        wcscpy(name, control.access_name_symbol -> Name());
        wcscat(name, value.String());

        //
        // Use the location of the class name for all elements of this method.
        //
        TokenIndex loc = declaration -> identifier_token;

        //
        // Add the method instead of inserting it, so it is not an overload
        // candidate.
        //
        write_method =
            new MethodSymbol(control.FindOrInsertName(name, length));
        Table() -> AddMethodSymbol(write_method);
        write_method -> SetType(sem -> control.void_type);
        write_method -> SetFlags(ACCESS_STATIC | ACCESS_SYNTHETIC);
        if (ACC_STRICTFP())
            write_method -> SetACC_STRICTFP();
        if (ACC_FINAL())
            write_method -> SetACC_FINAL();
        write_method -> SetContainingType(this);

        BlockSymbol* block_symbol =
            new BlockSymbol(member -> ACC_STATIC() ? 1 : 2);
        block_symbol -> max_variable_index = 0;
        write_method -> SetBlockSymbol(block_symbol);

        AstExpression* base;
        if (! member -> ACC_STATIC() && base_type == super)
        {
            //
            // Special case - for Outer.super.i where i is an instance field,
            // we mark the field access as a super access, to make sure we use
            // the correct qualifying instance.  Notice that in this case,
            // ((Super) Outer.this).i cannot generate an accessor method
            // (either i is public or in the same package and thus already
            // accessible, or i is protected in a different package and
            // therefore inaccessible), so we don't have to worry about a
            // conflict in accessor methods for the same base type.
            //
            base = ast_pool -> GenSuperExpression(loc);
        }
        else base = ast_pool -> GenName(loc);

        AstFieldAccess* left_hand_side = ast_pool -> GenFieldAccess();
        left_hand_side -> base = base;
        left_hand_side -> identifier_token = loc;
        left_hand_side -> symbol = member;

        AstMethodDeclarator* method_declarator =
            ast_pool -> GenMethodDeclarator();
        method_declarator -> identifier_token = loc;
        method_declarator -> left_parenthesis_token = loc;
        method_declarator -> right_parenthesis_token = loc;

        if (member -> ACC_STATIC())
        {
            method_declarator -> AllocateFormalParameters(1);
            base -> symbol = base_type;
        }
        else
        {
            method_declarator -> AllocateFormalParameters(2);

            NameSymbol* instance_name = control.MakeParameter(1);

            VariableSymbol* instance =
                block_symbol -> InsertVariableSymbol(instance_name);
            instance -> SetACC_SYNTHETIC();
            instance -> SetType(base_type == super ? this : base_type);
            instance -> SetOwner(write_method);
            instance -> SetLocalVariableIndex(block_symbol ->
                                              max_variable_index++);
            instance -> MarkComplete();
            write_method -> AddFormalParameter(instance);
            base -> symbol = (base_type == super
                              ? (Symbol*) super : (Symbol*) instance);
        }

        VariableSymbol* symbol =
            block_symbol -> InsertVariableSymbol(member -> Identity());
        symbol -> SetACC_SYNTHETIC();
        symbol -> SetType(member -> Type());
        symbol -> SetOwner(write_method);
        symbol -> SetLocalVariableIndex(block_symbol -> max_variable_index++);
        symbol -> MarkComplete();

        if (control.IsDoubleWordType(member -> Type()))
            block_symbol -> max_variable_index++;
        write_method -> AddFormalParameter(symbol);
        // A write access method has no throws clause !
        write_method -> SetSignature(control);

        AstName* simple_name = ast_pool -> GenName(loc);
        simple_name -> symbol = symbol;

        AstAssignmentExpression* assignment_expression = ast_pool ->
            GenAssignmentExpression(AstAssignmentExpression::SIMPLE_EQUAL,
                                    loc);
        assignment_expression -> left_hand_side = left_hand_side;
        assignment_expression -> expression = simple_name;

        AstExpressionStatement* expression_statement =
            ast_pool -> GenExpressionStatement();
        expression_statement -> expression = assignment_expression;
        expression_statement -> semicolon_token_opt = loc;
        expression_statement -> is_reachable = true;
        expression_statement -> can_complete_normally = true;

        AstReturnStatement* return_statement =
            ast_pool -> GenReturnStatement();
        return_statement -> return_token = loc;
        return_statement -> semicolon_token = loc;
        return_statement -> is_reachable = true;

        AstMethodBody* block = ast_pool -> GenMethodBody();
        block -> left_brace_token = loc;
        block -> right_brace_token = loc;
        block -> block_symbol = new BlockSymbol(0);
        ast_pool -> RegisterBlockSymbol(block -> block_symbol);
        block -> is_reachable = true;
        block -> AllocateStatements(2);
        block -> AddStatement(expression_statement);
        block -> AddStatement(return_statement);

        AstMethodDeclaration* method_declaration =
            ast_pool -> GenMethodDeclaration();
        method_declaration -> method_symbol = write_method;
        method_declaration -> method_declarator = method_declarator;
        method_declaration -> method_body_opt = block;

        write_method -> declaration = method_declaration;
        write_method -> accessed_member = member;
        MapSymbolToWriteMethod(member, base_type, write_method);
        AddPrivateAccessMethod(write_method);

        delete [] name;
    }
    return write_method;
}


MethodSymbol* TypeSymbol::GetWriteAccessFromReadAccess(MethodSymbol* read_method)
{
    assert(read_method && read_method -> ACC_SYNTHETIC() &&
           read_method -> containing_type == this);
    VariableSymbol* variable =
        DYNAMIC_CAST<VariableSymbol*> (read_method -> accessed_member);
    AstMethodDeclaration* method_declaration =
        DYNAMIC_CAST<AstMethodDeclaration*> (read_method -> declaration);
    AstMethodBody* block = method_declaration -> method_body_opt;
    AstReturnStatement* return_statement =
        DYNAMIC_CAST<AstReturnStatement*> (block -> Statement(0));
    AstFieldAccess* field_access =
        DYNAMIC_CAST<AstFieldAccess*> (return_statement -> expression_opt);
    return GetWriteAccessMethod(variable, field_access -> base -> Type());
}


//
// Create a new placeholder type in order to create a unique parameter in
// accessor constructors. The first anonymous type created in an outer class
// can be used as the placeholder.
//
TypeSymbol* TypeSymbol::GetPlaceholderType()
{
    assert(outermost_type == this);
    if (! placeholder_type)
    {
        //
        // Use the location of the class name for all elements of the
        // placeholder.
        //
        Semantic* sem = semantic_environment -> sem;
        sem -> state_stack.Push(semantic_environment);
        TokenIndex loc = declaration -> identifier_token;
        Control& control = sem -> control;
        StoragePool* ast_pool = sem -> compilation_unit -> ast_pool;

        AstClassBody* class_body = ast_pool -> GenClassBody();
        class_body -> left_brace_token = loc;
        class_body -> right_brace_token = loc;
        AstName* ast_type = ast_pool -> GenName(loc);

        AstClassCreationExpression* class_creation =
            ast_pool -> GenClassCreationExpression();
        class_creation -> new_token = loc;
        class_creation -> class_type = ast_pool -> GenTypeName(ast_type);
        class_creation -> arguments = ast_pool -> GenArguments(loc, loc);
        class_creation -> class_body_opt = class_body;

        sem -> GetAnonymousType(class_creation, control.Object());
        sem -> state_stack.Pop();
        assert(placeholder_type);
        placeholder_type -> SetACC_SYNTHETIC();
    }
    return placeholder_type;
}


} // Close namespace Jopa block
