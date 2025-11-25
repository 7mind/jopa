#include "platform.h"
#include "semantic.h"
#include "control.h"
#include "depend.h"
#include "table.h"
#include "tuple.h"
#include "spell.h"
#include "option.h"
#include "stream.h"
#include "typeparam.h"
#include "paramtype.h"


namespace Jopa { // Open namespace Jopa block
//
// If this compilation unit contains a package declaration, make sure
// the package is not also associated with a type. We used to also
// require that the package exist at compile time, but this was
// changed so that we are compatible with other Java compilers.
//
inline void Semantic::CheckPackage()
{
    if (! compilation_unit -> package_declaration_opt)
        return;
    AstPackageDeclaration* package_decl =
        compilation_unit -> package_declaration_opt;
    //
    // Make sure that the package or any of its parents does not match the
    // name of a type.
    //
    for (PackageSymbol* subpackage = this_package,
             *package = subpackage -> owner;
         package; subpackage = package, package = package -> owner)
    {
        FileSymbol* file_symbol =
            Control::GetFile(control, package, subpackage -> Identity());
        if (file_symbol)
        {
            char* file_name = file_symbol -> FileName();
            int length = file_symbol -> FileNameLength();
            wchar_t* error_name = new wchar_t[length + 1];
            for (int i = 0; i < length; i++)
                error_name[i] = file_name[i];
            error_name[length] = U_NULL;

            ReportSemError(SemanticError::PACKAGE_TYPE_CONFLICT,
                           compilation_unit -> package_declaration_opt -> name,
                           package -> PackageName(), subpackage -> Name(),
                           error_name);
            delete [] error_name;
        }
    }
    // TODO: Warn about package annotations outside of package-info.java.
    ProcessPackageModifiers(package_decl);
}


//
// Pass 1: Introduce the main package, the current package and all types
// specified into their proper scope
//
void Semantic::ProcessTypeNames()
{
    import_on_demand_packages.Next() = control.LangPackage();
    compilation_unit = source_file_symbol -> compilation_unit;

    //
    // If we are supposed to be verbose, report empty declarations...
    //
    if (control.option.pedantic)
    {
        if (compilation_unit -> EmptyCompilationUnitCast())
            ReportSemError(SemanticError::NO_TYPES, compilation_unit);
        for (unsigned i = 0;
             i < compilation_unit -> NumTypeDeclarations(); i++)
        {
            AstDeclaredType* type_declaration =
                compilation_unit -> TypeDeclaration(i);
            if (type_declaration -> EmptyDeclarationCast())
            {
                ReportSemError(SemanticError::EMPTY_DECLARATION,
                               type_declaration);
            }
        }
    }

    //
    // If we have a bad compilation unit insert its types as "bad types"
    //
    if (compilation_unit -> BadCompilationUnitCast())
    {
        for (unsigned i = 0; i < lex_stream -> NumTypes(); i++)
        {
            TokenIndex identifier_token =
                lex_stream -> Next(lex_stream -> Type(i));
            NameSymbol* name_symbol =
                lex_stream -> NameSymbol(identifier_token);
            if (name_symbol)
            {
                TypeSymbol* type = this_package -> FindTypeSymbol(name_symbol);
                assert(type);

                type -> MarkBad();
                type -> MarkSourceNoLongerPending();
                type -> supertypes_closure = new SymbolSet;
                type -> subtypes = new SymbolSet;
                type -> semantic_environment =
                    new SemanticEnvironment(this, type, NULL);
                if (type != control.Object())
                    type -> super = control.no_type;
                if (! type -> FindMethodSymbol(control.init_name_symbol))
                    AddDefaultConstructor(type);
                source_file_symbol -> types.Next() = type;
            }
        }
        return;
    }

    //
    // Use this tuple to compute the list of valid types encountered in this
    // compilation unit.
    //
    Tuple<TypeSymbol*> type_list;

    //
    // Process each type in this compilation unit, in turn
    //
    for (unsigned k = 0; k < compilation_unit -> NumTypeDeclarations(); k++)
    {
        TokenIndex identifier_token = BAD_TOKEN;
        TypeSymbol* type = NULL;
        AstDeclaredType* declaration = compilation_unit -> TypeDeclaration(k);
        if (! declaration -> EmptyDeclarationCast())
        {
            identifier_token = declaration -> class_body -> identifier_token;
            NameSymbol* name_symbol =
                lex_stream -> NameSymbol(identifier_token);
            //
            // Warn against unconventional names.
            //
            if (name_symbol -> IsBadStyleForClass())
            {
                ReportSemError(SemanticError::UNCONVENTIONAL_CLASS_NAME,
                               identifier_token, name_symbol -> Name());
            }
            type = this_package -> FindTypeSymbol(name_symbol);
            // type was already created in Control::ProcessPackageDeclaration
            assert(type);
            if (! type -> SourcePending())
            {
                ReportSemError(SemanticError::DUPLICATE_TYPE_DECLARATION,
                               identifier_token, name_symbol -> Name(),
                               type -> FileLoc());
                type = NULL;
            }
            else
            {
                if (type -> ContainingPackage() == control.UnnamedPackage())
                {
                    TypeSymbol* old_type = (TypeSymbol*) control.
                        unnamed_package_types.Image(name_symbol);
                    if (old_type != type)
                    {
                        ReportSemError(SemanticError::DUPLICATE_TYPE_DECLARATION,
                                       identifier_token, name_symbol -> Name(),
                                       old_type -> FileLoc());
                    }
                }
                // Save valid type for later processing. See below.
                type_list.Next() = type;
                type -> MarkSourceNoLongerPending();
                type -> semantic_environment =
                    new SemanticEnvironment(this, type, NULL);
                type -> declaration = declaration -> class_body;
                type -> SetFlags(ProcessTopLevelTypeModifiers(declaration));
                //
                // Add 3 extra elements for padding. May need a default
                // constructor and other support elements.
                //
                type -> SetSymbolTable(declaration -> class_body ->
                                       NumClassBodyDeclarations() + 3);
                type -> SetLocation();

                if (lex_stream -> IsDeprecated(declaration -> LeftToken()))
                    type -> MarkDeprecated();
                source_file_symbol -> types.Next() = type;
                declaration -> class_body -> semantic_environment =
                    type -> semantic_environment;
                CheckNestedMembers(type, declaration -> class_body);
            }
        }

        //
        // If we successfully processed this type, check that
        //     . its name does not conflict with a subpackage
        //     . if it is contained in a file with a different name
        //       than its own name that there does not also exist a
        //       (java or class) file with its name.
        //
        if (type)
        {
            NameSymbol* name_symbol =
                lex_stream -> NameSymbol(identifier_token);
            for (unsigned i = 0; i < this_package -> directory.Length(); i++)
            {
                //
                // The unnamed package cannot contain subpackages, as
                // subpackages require a named top-level package. In other
                // words, java.lang is not a subpackage of the unnamed one.
                //
                if ((this_package -> directory[i] ->
                     FindDirectorySymbol(name_symbol)) &&
                    this_package != control.UnnamedPackage())
                {
                    char* file_name = type -> file_symbol -> FileName();
                    int length = type -> file_symbol -> FileNameLength();
                    wchar_t* error_name = new wchar_t[length + 1];
                    for (int j = 0; j < length; j++)
                        error_name[j] = file_name[j];
                    error_name[length] = U_NULL;

                    ReportSemError(SemanticError::PACKAGE_TYPE_CONFLICT,
                                   identifier_token,
                                   this_package -> PackageName(),
                                   name_symbol -> Name(), error_name);
                    delete [] error_name;
                }
            }

            if (type -> Identity() != source_file_symbol -> Identity())
            {
                PackageSymbol* package = this_package;
                FileSymbol* file_symbol =
                    Control::GetJavaFile(package, type -> Identity());

                if (file_symbol)
                {
                    ReportSemError(SemanticError::TYPE_IN_MULTIPLE_FILES,
                                   identifier_token,
                                   this_package -> PackageName(),
                                   source_file_symbol -> Name(),
                                   package -> PackageName(), type -> Name());
                }
            }
        }
    }

    CheckPackage();
    ProcessImports();

    //
    // Make sure that compilation unit contains exactly one public type, and
    // that it matches the file name.
    //
    TypeSymbol* public_type = NULL;
    for (unsigned i = 0; i < type_list.Length(); i++)
    {
        TypeSymbol* type = type_list[i];
        if (type && type -> ACC_PUBLIC())
        {
            if (! public_type)
            {
                public_type = type;
                if (source_file_symbol -> Identity() !=
                    public_type -> Identity())
                {
                    ReportSemError(SemanticError::MISMATCHED_TYPE_AND_FILE_NAMES,
                                   type -> declaration -> identifier_token,
                                   public_type -> Name());
                }
            }
            else
            {
                ReportSemError(SemanticError::MULTIPLE_PUBLIC_TYPES,
                               type -> declaration -> identifier_token,
                               type -> Name(),
                               public_type -> Name());
            }
        }
    }
}


void Semantic::CheckNestedMembers(TypeSymbol* containing_type,
                                  AstClassBody* class_body)
{
    unsigned i;
    for (i = 0; i < class_body -> NumNestedClasses(); i++)
    {
        AstClassDeclaration* decl = class_body -> NestedClass(i);
        ProcessNestedTypeName(containing_type, decl);
    }
    for (i = 0; i < class_body -> NumNestedEnums(); i++)
    {
        AstEnumDeclaration* decl = class_body -> NestedEnum(i);
        ProcessNestedTypeName(containing_type, decl);
    }
    for (i = 0; i < class_body -> NumNestedInterfaces(); i++)
    {
        AstInterfaceDeclaration* decl = class_body -> NestedInterface(i);
        ProcessNestedTypeName(containing_type, decl);
    }
    for (i = 0; i < class_body -> NumNestedAnnotations(); i++)
    {
        AstAnnotationDeclaration* decl = class_body -> NestedAnnotation(i);
        ProcessNestedTypeName(containing_type, decl);
    }
    for (i = 0; i < class_body -> NumEmptyDeclarations(); i++)
    {
        if (control.option.pedantic)
            ReportSemError(SemanticError::EMPTY_DECLARATION,
                           class_body -> EmptyDeclaration(i));
    }
}


//
// Given a type shadow symbol, returns the first accessible type, and reports
// an error for any other accessible types.
//
inline TypeSymbol* Semantic::FindTypeInShadow(TypeShadowSymbol* type_shadow_symbol,
                                              TokenIndex identifier_token)
{
    TypeSymbol* type = type_shadow_symbol -> type_symbol;
    unsigned i = 0;
    if (! TypeAccessCheck(type))
    {
        if (type_shadow_symbol -> NumConflicts())
        {
            //
            // The conflicts are necessarily accessible, because they are
            // public types inherited from interfaces.
            //
            type = type_shadow_symbol -> Conflict(0);
            i = 1;
        }
        else type = NULL;
    }
    for ( ; i < type_shadow_symbol -> NumConflicts(); i++)
    {
        ReportSemError(SemanticError::AMBIGUOUS_TYPE,
                       identifier_token, type -> Name(),
                       type -> owner -> TypeCast() -> ContainingPackageName(),
                       type -> owner -> TypeCast() -> ExternalName(),
                       type_shadow_symbol -> Conflict(i) -> owner -> TypeCast() -> ContainingPackageName(),
                       type_shadow_symbol -> Conflict(i) -> owner -> TypeCast() -> ExternalName());
    }
    return type;
}


void Semantic::CheckNestedTypeDuplication(SemanticEnvironment* env,
                                          TokenIndex identifier_token)
{
    NameSymbol* name_symbol = lex_stream -> NameSymbol(identifier_token);

    //
    // First check to see if we have a duplication at the same level...
    //
    TypeSymbol* old_type = NULL;
    if (env -> symbol_table.Size())
    {
        for (int i = env -> symbol_table.Size(); --i >= 0; )
        {
            old_type = env -> symbol_table[i] -> FindTypeSymbol(name_symbol);
            if (old_type)
                break;
        }
    }
    else old_type = env -> Type() -> FindTypeSymbol(name_symbol);
    if (old_type)
    {
        ReportSemError(SemanticError::DUPLICATE_TYPE_DECLARATION,
                       identifier_token, name_symbol -> Name(),
                       old_type -> FileLoc());
    }
    else
    {
        //
        // ... Then check the enclosing environments...
        //
        for (; env; env = env -> previous)
        {
            if (env -> Type() -> Identity() == name_symbol)
            {
                ReportSemError(SemanticError::DUPLICATE_INNER_TYPE_NAME,
                               identifier_token, name_symbol -> Name(),
                               env -> Type() -> FileLoc());
                break;
            }
        }
    }
}


TypeSymbol* Semantic::ProcessNestedTypeName(TypeSymbol* containing_type,
                                            AstDeclaredType* declaration)
{
    AstClassBody* class_body = declaration -> class_body;
    NameSymbol* name_symbol =
        lex_stream -> NameSymbol(class_body -> identifier_token);
    TypeSymbol* outermost_type = containing_type -> outermost_type;

    CheckNestedTypeDuplication(containing_type -> semantic_environment,
                               class_body -> identifier_token);
    int length = containing_type -> ExternalNameLength() + 1 +
        name_symbol -> NameLength(); // +1 for $,... +1 for $
    wchar_t* external_name = new wchar_t[length + 1]; // +1 for '\0';
    wcscpy(external_name, containing_type -> ExternalName());
    wcscat(external_name, StringConstant::US_DS);
    wcscat(external_name, name_symbol -> Name());

    TypeSymbol* inner_type =
        containing_type -> InsertNestedTypeSymbol(name_symbol);
    inner_type -> outermost_type = outermost_type;
    inner_type -> supertypes_closure = new SymbolSet;
    inner_type -> subtypes = new SymbolSet;
    inner_type -> SetExternalIdentity(control.FindOrInsertName(external_name,
                                                               length));
    inner_type -> semantic_environment =
        new SemanticEnvironment(this, inner_type,
                                containing_type -> semantic_environment);
    inner_type -> declaration = declaration -> class_body;
    inner_type -> file_symbol = source_file_symbol;
    inner_type -> SetFlags(ProcessNestedTypeModifiers(containing_type,
                                                      declaration));
    inner_type -> SetOwner(containing_type);
    //
    // Add 3 extra elements for padding. May need a default constructor and
    // other support elements.
    //
    inner_type -> SetSymbolTable(class_body -> NumClassBodyDeclarations() + 3);
    inner_type -> SetLocation();
    inner_type -> SetSignature(control);

    delete [] external_name;

    if (lex_stream -> IsDeprecated(declaration -> LeftToken()))
        inner_type -> MarkDeprecated();

    //
    // If not a top-level type, then add pointer to enclosing type.
    //
    if (! inner_type -> ACC_STATIC())
        inner_type -> InsertThis0();
    else if (containing_type -> IsInner())
    {
        ReportSemError(SemanticError::STATIC_TYPE_IN_INNER_CLASS,
                       class_body -> identifier_token, name_symbol -> Name(),
                       containing_type -> Name(),
                       containing_type -> FileLoc());
        // Change its status so we can continue compiling...
        inner_type -> ResetACC_STATIC();
    }

    if (inner_type -> IsLocal())
    {
        if (! outermost_type -> local)
            outermost_type -> local = new SymbolSet;
        outermost_type -> local -> AddElement(inner_type);
    }
    else
    {
        if (! outermost_type -> non_local)
            outermost_type -> non_local = new SymbolSet;
        outermost_type -> non_local -> AddElement(inner_type);
    }

    //
    // Warn against unconventional names.
    //
    if (name_symbol -> IsBadStyleForClass())
    {
        ReportSemError(SemanticError::UNCONVENTIONAL_CLASS_NAME,
                       class_body -> identifier_token, name_symbol -> Name());
    }

    declaration -> class_body -> semantic_environment =
        inner_type -> semantic_environment;
    CheckNestedMembers(inner_type, class_body);
    return inner_type;
}


//
// Pass 1.2: Process all import statements
//
void Semantic::ProcessImports()
{
    for (unsigned i = 0; i < compilation_unit -> NumImportDeclarations(); i++)
    {
        AstImportDeclaration* import_declaration =
            compilation_unit -> ImportDeclaration(i);
        if (import_declaration -> static_token_opt)
        {
            if (control.option.source < JopaOption::SDK1_5)
            {
                ReportSemError(SemanticError::STATIC_IMPORT_UNSUPPORTED,
                               import_declaration -> static_token_opt);
            }
            else
            {
                // Java 5: Process static imports
                if (import_declaration -> star_token_opt)
                    ProcessStaticImportOnDemandDeclaration(import_declaration);
                else
                    ProcessSingleStaticImportDeclaration(import_declaration);
            }
        }
        else
        {
            // Regular type imports
            if (import_declaration -> star_token_opt)
                ProcessTypeImportOnDemandDeclaration(import_declaration);
            else
                ProcessSingleTypeImportDeclaration(import_declaration);
        }
    }
}


//
// Pass 2: Process "extends" and "implements" clauses associated with the
// types.
//
void Semantic::ProcessTypeHeader(AstClassDeclaration* declaration)
{
    TypeSymbol* type =
        declaration -> class_body -> semantic_environment -> Type();
    assert(! type -> HeaderProcessed() || type -> Bad());
    type -> MarkHeaderProcessed();

    // Set processing_type so ThisType() works during header processing
    processing_type = type;

    if (type -> Bad())
    {
        processing_type = NULL;
        return;
    }

    //
    // Special case certain classes in java.lang.  We can't use the
    // control.Classname() accessor method here, because that causes problems
    // with recursion or non-existant classes.
    //
    if (this_package == control.LangPackage() && ! type -> IsNested())
    {
        // java.lang.Object is the only class with no supertype.
        if (type -> Identity() == control.Object_name_symbol)
        {
            if (declaration -> super_opt || declaration -> NumInterfaces())
            {
                ReportSemError(SemanticError::OBJECT_WITH_SUPER_TYPE,
                               declaration -> LeftToken(),
                               declaration -> class_body -> left_brace_token - 1);
            }
            if (declaration -> type_parameters_opt)
            {
                ReportSemError(SemanticError::TYPE_MAY_NOT_HAVE_PARAMETERS,
                               declaration -> LeftToken(),
                               declaration -> class_body -> left_brace_token - 1,
                               type -> ContainingPackageName(),
                               type -> ExternalName());
            }
            type -> MarkHeaderProcessed();
            return;
        }
        //
        // java.lang.Enum didn't exist before 1.5.
        //
        else if (type -> Identity() == control.Enum_name_symbol)
            type -> MarkEnum();
    }

    if (declaration -> type_parameters_opt)
        ProcessTypeParameters(type, declaration -> type_parameters_opt);

    //
    // Process the supertypes.
    //
    if (declaration -> super_opt)
    {
        ProcessType(declaration -> super_opt);
        TypeSymbol* super_type = declaration -> super_opt -> symbol;
        assert(! super_type -> SourcePending());
        if (! super_type -> HeaderProcessed())
            super_type -> ProcessTypeHeaders();
        if (control.option.deprecation && state_stack.Size() == 0 &&
            super_type -> IsDeprecated() && ! type -> IsDeprecated())
        {
            ReportSemError(SemanticError::DEPRECATED_TYPE,
                           declaration -> super_opt,
                           super_type -> ContainingPackageName(),
                           super_type -> ExternalName());
        }
        if (super_type -> IsEnum())
        {
            ReportSemError(SemanticError::SUPER_IS_ENUM,
                           declaration -> super_opt,
                           super_type -> ContainingPackageName(),
                           super_type -> ExternalName());
        }
        else if (super_type -> ACC_INTERFACE())
        {
            ReportSemError(SemanticError::NOT_A_CLASS,
                           declaration -> super_opt,
                           super_type -> ContainingPackageName(),
                           super_type -> ExternalName());
        }
        else if (super_type -> ACC_FINAL())
        {
            ReportSemError(SemanticError::SUPER_IS_FINAL,
                           declaration -> super_opt,
                           super_type -> ContainingPackageName(),
                           super_type -> ExternalName());
        }
        else if (super_type -> Bad())
            ; // ignore
        else
        {
            super_type -> subtypes -> AddElement(type);
            type -> super = super_type;
            while (super_type)
            {
                type -> supertypes_closure -> AddElement(super_type);
                type -> supertypes_closure -> Union(*super_type ->
                                                    supertypes_closure);
                if (super_type -> owner -> TypeCast())
                    super_type = super_type -> ContainingType();
                else super_type = NULL;
            }
        }
    }
    if (! type -> super)
    {
        type -> super = control.Object();
        type -> supertypes_closure -> AddElement(control.Object());
        control.Object() -> subtypes -> AddElement(type);
    }
    AddDependence(type, type -> super);

    for (unsigned i = 0; i < declaration -> NumInterfaces(); i++)
        ProcessSuperinterface(type, declaration -> Interface(i));

    // if there is a cycle, break it and issue an error message
    if (type -> supertypes_closure -> IsElement(type))
    {
        type -> super = control.Object();
        type -> ResetInterfaces();
        type -> MarkCircular();
        ReportSemError(SemanticError::CIRCULAR_CLASS,
                       declaration -> class_body -> identifier_token,
                       declaration -> class_body -> left_brace_token - 1,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }
    else if (declaration -> type_parameters_opt &&
             type -> IsSubclass(control.Throwable()))
    {
        ReportSemError(SemanticError::TYPE_MAY_NOT_HAVE_PARAMETERS,
                       declaration -> LeftToken(),
                       declaration -> class_body -> left_brace_token - 1,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }

    // Clear processing_type after header processing
    processing_type = NULL;
}


void Semantic::ProcessTypeHeader(AstEnumDeclaration* declaration)
{
    TypeSymbol* type =
        declaration -> class_body -> semantic_environment -> Type();
    assert(! type -> HeaderProcessed() || type -> Bad());
    type -> MarkHeaderProcessed();

    // Set processing_type so ThisType() works during header processing
    processing_type = type;

    if (control.option.source < JopaOption::SDK1_5)
    {
        ReportSemError(SemanticError::ENUM_TYPE_UNSUPPORTED,
                       declaration -> enum_token);
        type -> super = control.Object();
        type -> MarkBad();
    }
    if (type -> Bad())
    {
        processing_type = NULL;
        return;
    }

    //
    // Process the supertypes.
    //
    type -> super = control.Enum();
    type -> supertypes_closure -> AddElement(control.Enum());
    type -> MarkEnum();
    type -> SetACC_ENUM(); // JVM spec requires ACC_ENUM flag for enum types
    control.Enum() -> subtypes -> AddElement(type);
    AddDependence(type, type -> super);
    for (unsigned i = 0; i < declaration -> NumInterfaces(); i++)
        ProcessSuperinterface(type, declaration -> Interface(i));
    // there will not be a cycle
    assert(! type -> supertypes_closure -> IsElement(type));

    // Clear processing_type after header processing
    processing_type = NULL;
}


void Semantic::ProcessTypeHeader(AstInterfaceDeclaration* declaration)
{
    TypeSymbol* type =
        declaration -> class_body -> semantic_environment -> Type();
    assert(! type -> HeaderProcessed() || type -> Bad());
    type -> MarkHeaderProcessed();

    // Set processing_type so ThisType() works during header processing
    processing_type = type;

    if (declaration -> type_parameters_opt)
        ProcessTypeParameters(type, declaration -> type_parameters_opt);

    //
    // Although interfaces do not have a superclass in source code, in
    // bytecode they are treated as subclasses of Object.
    //
    type -> super = control.Object();
    AddDependence(type, control.Object());
    for (unsigned k = 0; k < declaration -> NumInterfaces(); k++)
        ProcessSuperinterface(type, declaration -> Interface(k));

    if (type -> supertypes_closure -> IsElement(type))
    {
        //
        // Remove all the interfaces if a loop is detected.
        //
        type -> ResetInterfaces();
        type -> MarkCircular();
        ReportSemError(SemanticError::CIRCULAR_INTERFACE,
                       declaration -> class_body -> identifier_token,
                       declaration -> class_body -> left_brace_token - 1,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }

    // Clear processing_type after header processing
    processing_type = NULL;
}


void Semantic::ProcessTypeHeader(AstAnnotationDeclaration* declaration)
{
    TypeSymbol* type =
        declaration -> class_body -> semantic_environment -> Type();
    assert(! type -> HeaderProcessed() || type -> Bad());
    type -> MarkHeaderProcessed();

    // Set processing_type so ThisType() works during header processing
    processing_type = type;

    if (control.option.source < JopaOption::SDK1_5)
    {
        ReportSemError(SemanticError::ANNOTATION_TYPE_UNSUPPORTED,
                       declaration -> interface_token - 1,
                       declaration -> interface_token);
        type -> MarkBad();
    }
    //
    // All annotations are treated as subclasses of Object and Annotation.
    //
    type -> super = control.Object();
    AddDependence(type, control.Object());
    type -> AddInterface(control.Annotation());
    AddDependence(type, control.Annotation());

    // Clear processing_type after header processing
    processing_type = NULL;
}


void Semantic::ProcessSuperinterface(TypeSymbol* base_type, AstTypeName* name)
{
    ProcessType(name);
    TypeSymbol* interf = name -> symbol;

    assert(! interf -> SourcePending());
    if (! interf -> HeaderProcessed())
        interf -> ProcessTypeHeaders();
    if (control.option.deprecation && state_stack.Size() == 0 &&
        interf -> IsDeprecated() && ! base_type -> IsDeprecated())
    {
        ReportSemError(SemanticError::DEPRECATED_TYPE, name,
                       interf -> ContainingPackageName(),
                       interf -> ExternalName());
    }
    if (! interf -> ACC_INTERFACE())
    {
        if (! interf -> Bad())
        {
            ReportSemError(SemanticError::NOT_AN_INTERFACE, name,
                           interf -> ContainingPackageName(),
                           interf -> ExternalName());
        }
        name -> symbol = NULL;
    }
    else
    {
        for (unsigned k = 0; k < base_type -> NumInterfaces(); k++)
        {
            if (base_type -> Interface(k) == interf)
            {
                ReportSemError(SemanticError::DUPLICATE_INTERFACE, name,
                               interf -> ContainingPackageName(),
                               interf -> ExternalName(),
                               base_type -> ExternalName());
                name -> symbol = NULL;
                return;
            }
        }
        name -> symbol = interf; // save type name in ast.
        base_type -> AddInterface(interf);
        interf -> subtypes -> AddElement(base_type);
        AddDependence(base_type, interf);
        while (interf)
        {
            base_type -> supertypes_closure -> AddElement(interf);
            base_type -> supertypes_closure -> Union(*interf ->
                                                     supertypes_closure);
            if (interf -> owner -> TypeCast())
                interf = interf -> ContainingType();
            else interf = NULL;
        }
    }
}


//
// Processes the type parameters of a class or interface.
//
void Semantic::ProcessTypeParameters(TypeSymbol* type,
                                     AstTypeParameters* parameters)
{
    if (! parameters)
        return;

    // Process each type parameter declaration
    for (unsigned i = 0; i < parameters -> NumTypeParameters(); i++)
    {
        AstTypeParameter* param_ast = parameters -> TypeParameter(i);

        // Get the name of the type parameter
        const NameSymbol* name_symbol =
            lex_stream -> NameSymbol(param_ast -> identifier_token);

        // Check for duplicate type parameter names
        for (unsigned j = 0; j < i; j++)
        {
            AstTypeParameter* prev_param = parameters -> TypeParameter(j);
            const NameSymbol* prev_name =
                lex_stream -> NameSymbol(prev_param -> identifier_token);

            if (name_symbol == prev_name)
            {
                // Duplicate type parameter - report error and skip
                ReportSemError(SemanticError::DUPLICATE_TYPE_PARAMETER,
                              param_ast -> identifier_token,
                              name_symbol -> Name());
                param_ast -> symbol = NULL;
                continue;
            }
        }

        // Create the TypeParameterSymbol
        TypeParameterSymbol* type_param =
            new TypeParameterSymbol(name_symbol, type, i);

        // Store it in the AST node
        param_ast -> symbol = type_param;

        // Add to the containing type's type parameters
        type -> AddTypeParameter(type_param);

        // Process bounds (extends clauses)
        // NOTE: We defer bound processing because FindType requires a semantic
        // environment stack which isn't set up during header processing.
        // Bounds will be validated later during semantic analysis.
        // For now, we'll try a simple name lookup.
        for (unsigned j = 0; j < param_ast -> NumBounds(); j++)
        {
            AstTypeName* bound_ast = param_ast -> Bound(j);
            NameSymbol* bound_name = lex_stream -> NameSymbol(bound_ast -> name -> identifier_token);

            // Simple lookup in current package or java.lang
            TypeSymbol* bound_type = NULL;

            // Try to find in the current package
            PackageSymbol* package = type -> ContainingPackage();
            if (package)
            {
                bound_type = package -> FindTypeSymbol(bound_name);
            }

            // If not found, try java.lang package
            if (!bound_type)
            {
                PackageSymbol* java_lang = control.ProcessPackage(StringConstant::US_java_SL_lang);
                if (java_lang)
                {
                    bound_type = java_lang -> FindTypeSymbol(bound_name);

                    // If still not found, try to read the type from source
                    if (!bound_type)
                    {
                        FileSymbol* file_symbol = Control::GetJavaFile(java_lang, bound_name);
                        if (file_symbol)
                        {
                            ReadType(file_symbol, java_lang, bound_name, bound_ast -> name -> identifier_token);
                            bound_type = java_lang -> FindTypeSymbol(bound_name);
                        }
                    }
                }
            }

            if (bound_type)
            {
                // TODO: Add proper validation for final classes in bounds
                // TODO: Validate that only first bound can be a class
                type_param -> AddBound(bound_type);
            }
            // If bound_type is still NULL, we'll report an error later
        }
    }
}


//
// Process type parameters for a generic method
//
void Semantic::ProcessMethodTypeParameters(MethodSymbol* method,
                                           AstTypeParameters* parameters)
{
    if (! parameters)
        return;

    // Process each type parameter declaration
    for (unsigned i = 0; i < parameters -> NumTypeParameters(); i++)
    {
        AstTypeParameter* param_ast = parameters -> TypeParameter(i);

        // Get the name of the type parameter
        const NameSymbol* name_symbol =
            lex_stream -> NameSymbol(param_ast -> identifier_token);

        // Check for duplicate type parameter names
        for (unsigned j = 0; j < i; j++)
        {
            AstTypeParameter* prev_param = parameters -> TypeParameter(j);
            const NameSymbol* prev_name =
                lex_stream -> NameSymbol(prev_param -> identifier_token);

            if (name_symbol == prev_name)
            {
                // Duplicate type parameter - report error and skip
                ReportSemError(SemanticError::DUPLICATE_TYPE_PARAMETER,
                              param_ast -> identifier_token,
                              name_symbol -> Name());
                param_ast -> symbol = NULL;
                continue;
            }
        }

        // Create the TypeParameterSymbol with method as owner
        TypeParameterSymbol* type_param =
            new TypeParameterSymbol(name_symbol, method, i);

        // Store it in the AST node
        param_ast -> symbol = type_param;

        // Add to the containing method's type parameters
        method -> AddTypeParameter(type_param);

        // Process bounds (extends clauses)
        // NOTE: Using simple name lookup to avoid semantic environment stack issues
        for (unsigned j = 0; j < param_ast -> NumBounds(); j++)
        {
            AstTypeName* bound_ast = param_ast -> Bound(j);
            NameSymbol* bound_name = lex_stream -> NameSymbol(bound_ast -> name -> identifier_token);

            // Simple lookup in containing type's package or java.lang
            TypeSymbol* bound_type = NULL;

            // Try to find in the containing type's package
            TypeSymbol* containing_type = method -> containing_type;
            if (containing_type)
            {
                PackageSymbol* package = containing_type -> ContainingPackage();
                if (package)
                {
                    bound_type = package -> FindTypeSymbol(bound_name);
                }
            }

            // If not found, try java.lang package
            if (!bound_type)
            {
                PackageSymbol* java_lang = control.ProcessPackage(StringConstant::US_java_SL_lang);
                if (java_lang)
                {
                    bound_type = java_lang -> FindTypeSymbol(bound_name);
                }
            }

            if (bound_type)
            {
                type_param -> AddBound(bound_type);
            }
        }
    }
}


//
// Process type arguments for a parameterized type (e.g., List<String>)
//
// Process type arguments and optionally create a ParameterizedType
// Returns the ParameterizedType if one was created, NULL otherwise
//
ParameterizedType* Semantic::ProcessTypeArguments(TypeSymbol* base_type,
                                                   AstTypeArguments* type_arguments)
{
    if (! type_arguments)
        return NULL;

    // Check for Java 7+ diamond operator <>
    // Diamond is detected when:
    // 1. We're in Java 7+ mode
    // 2. There's exactly 1 type argument
    // 3. That type argument is a wildcard
    // 4. The wildcard's question_token is a synthetic diamond token
    if (control.option.source >= JopaOption::SDK1_7 &&
        type_arguments -> NumTypeArguments() == 1)
    {
        AstType* first_arg = type_arguments -> TypeArgument(0);
        AstWildcard* wildcard = first_arg -> WildcardCast();
        if (wildcard && lex_stream -> IsDiamondToken(wildcard -> question_token))
        {
            // Diamond operator - type arguments are inferred from context
            // For type erasure, just return NULL (treat as raw type)
            // The type system will use the base type's erasure
            return NULL;
        }
    }

    // Ensure the type header is processed before checking if it's generic
    // This is necessary when the base type is defined in the same compilation unit
    // but hasn't been processed yet
    if (! base_type -> HeaderProcessed() && base_type -> declaration)
    {
        base_type -> ProcessTypeHeaders();
    }

    // Check if the base type is actually a generic type
    if (! base_type -> IsGeneric())
    {
        ReportSemError(SemanticError::TYPE_MAY_NOT_HAVE_PARAMETERS,
                      type_arguments,
                      base_type -> ContainingPackageName(),
                      base_type -> ExternalName());
        return NULL;
    }

    // Validate arity (number of type arguments matches number of type parameters)
    unsigned num_args = type_arguments -> NumTypeArguments();
    unsigned num_params = base_type -> NumTypeParameters();

    if (num_args != num_params)
    {
        ReportSemError(SemanticError::MISMATCHED_TYPE_PARAMETER_COUNT,
                      type_arguments,
                      base_type -> ContainingPackageName(),
                      base_type -> ExternalName());
        return NULL;
    }

    // Process each type argument
    for (unsigned i = 0; i < num_args; i++)
    {
        AstType* type_arg_ast = type_arguments -> TypeArgument(i);

        // Process the type argument (recursively handles nested type arguments)
        ProcessType(type_arg_ast);

        TypeSymbol* arg_type = type_arg_ast -> symbol;

        if (! arg_type || arg_type -> Bad())
            continue;

        // Validate that type argument satisfies bounds of type parameter
        TypeParameterSymbol* type_param = base_type -> TypeParameter(i);

        // Wildcards have special handling - check their bounds separately
        AstWildcard* wildcard = type_arg_ast -> WildcardCast();
        if (wildcard)
        {
            // For unbounded wildcards (?), no validation needed
            // For bounded wildcards (? extends T, ? super T), bounds are validated when parsing
            continue;
        }

        // Check each bound of the type parameter
        for (unsigned j = 0; j < type_param -> NumBounds(); j++)
        {
            TypeSymbol* bound = type_param -> Bound(j);

            // Check if arg_type is a subtype of the bound
            if (! arg_type -> IsSubtype(bound))
            {
                // Type argument doesn't satisfy bound
                ReportSemError(SemanticError::TYPE_ARGUMENT_FAILS_BOUNDS,
                              type_arg_ast,
                              arg_type -> ContainingPackageName(),
                              arg_type -> ExternalName(),
                              type_param -> Name(),
                              bound -> ContainingPackageName(),
                              bound -> ExternalName());
                break; // Only report first bound violation
            }
        }

        // If no explicit bounds, default bound is Object - all reference types satisfy this
        // Primitive types cannot be type arguments (enforced elsewhere)
    }

    // At this point, all type arguments are valid
    // Create a ParameterizedType to track the type arguments
    Tuple<Type*>* type_arg_tuple = new Tuple<Type*>(num_args);
    for (unsigned i = 0; i < num_args; i++)
    {
        AstType* type_arg_ast = type_arguments -> TypeArgument(i);
        // For now, we just store the TypeSymbol as a Type*
        // In full implementation, we'd convert wildcards, nested parameterized types, etc.
        TypeSymbol* arg_sym = type_arg_ast -> symbol;
        if (! arg_sym || arg_sym -> Bad())
            arg_sym = control.no_type; // Use placeholder for bad types
        type_arg_tuple -> Next() = new Type(arg_sym);
    }

    ParameterizedType* param_type = new ParameterizedType(base_type, type_arg_tuple);
    return param_type;
}


//
// Generate bridge methods for generic method overrides
//
// Bridge methods are needed when a method in a subclass overrides a method
// from a superclass, and the erased signatures differ. This happens with
// covariant return types in generics.
//
// Example:
//   class Box<T> { T get() {...} }
//   class StringBox extends Box<String> { String get() {...} }
//
// After erasure, Box.get() returns Object, but StringBox.get() returns String.
// We generate a bridge: Object get() { return (Object)this.get(); }
//
void Semantic::GenerateBridgeMethods(TypeSymbol* type)
{
    if (! type -> expanded_method_table)
        return;

    // Iterate through all methods in this type
    for (unsigned i = 0; i < type -> NumMethodSymbols(); i++)
    {
        MethodSymbol* method = type -> MethodSym(i);

        // Skip constructors, static methods, and private methods
        if (method -> Identity() == control.init_name_symbol ||
            method -> ACC_STATIC() ||
            method -> ACC_PRIVATE())
        {
            continue;
        }

        // Look for overridden methods in superclasses
        if (! type -> super)
            continue;

        // Search for method with same name in superclass
        MethodShadowSymbol* shadow = type -> expanded_method_table ->
            FindOverloadMethodShadow(method, this, type -> declaration -> identifier_token);

        if (! shadow)
            continue;

        // Check all overridden methods for signature mismatches
        for (unsigned j = 0; j < shadow -> NumConflicts(); j++)
        {
            MethodSymbol* super_method = shadow -> Conflict(j);

            // Skip if same method or not from superclass
            if (super_method == method ||
                ! method -> containing_type -> IsSubclass(super_method -> containing_type))
            {
                continue;
            }

            // Check if erased signatures differ (covariant return type case)
            if (method -> Type() != super_method -> Type())
            {
                // Need bridge method: super_method signature -> method
                // Create bridge method with super_method's signature
                MethodSymbol* bridge = new MethodSymbol(method -> Identity());
                bridge -> SetContainingType(type);
                bridge -> SetType(super_method -> Type());
                bridge -> SetFlags(method -> Flags() | AccessFlags::ACCESS_SYNTHETIC | AccessFlags::ACCESS_BRIDGE);
                bridge -> SetBridgeTarget(method);
                bridge -> SetBlockSymbol(new BlockSymbol(1)); // Minimal block for bytecode generation

                // Copy parameter types from super_method
                for (unsigned k = 0; k < super_method -> NumFormalParameters(); k++)
                {
                    VariableSymbol* param = super_method -> FormalParameter(k);
                    VariableSymbol* new_param = new VariableSymbol(param -> Identity());
                    new_param -> SetType(param -> Type());
                    new_param -> SetFlags(param -> Flags());
                    new_param -> SetOwner(bridge);
                    bridge -> AddFormalParameter(new_param);
                }

                // Set signature (erased signature from super_method)
                bridge -> SetSignature(control);

                // Add bridge to method table
                type -> InsertMethodSymbol(bridge);
                method -> AddGeneratedBridge(bridge);
            }
        }
    }
}


//
// Process the type headers of the owner of body.  Anonymous types have no
// owner, so anon_type must be non-null only in that case.
//
TypeSymbol* Semantic::ProcessTypeHeaders(AstClassBody* body,
                                         TypeSymbol* anon_type)
{
    assert(! body -> owner ^ ! anon_type);
    SemanticEnvironment* sem = anon_type ? anon_type -> semantic_environment
        : body -> semantic_environment;
    TypeSymbol* type = anon_type ? anon_type : sem -> Type();
    if (type -> HeaderProcessed())
        return type; // Possible if a subclass was declared in the same file.
    if (anon_type)
        anon_type -> MarkHeaderProcessed();
    else if (body -> owner -> ClassDeclarationCast())
        ProcessTypeHeader((AstClassDeclaration*) body -> owner);
    else if (body -> owner -> EnumDeclarationCast())
        ProcessTypeHeader((AstEnumDeclaration*) body -> owner);
    else if (body -> owner -> InterfaceDeclarationCast())
        ProcessTypeHeader((AstInterfaceDeclaration*) body -> owner);
    else
    {
        assert(body -> owner -> AnnotationDeclarationCast());
        ProcessTypeHeader((AstAnnotationDeclaration*) body -> owner);
    }
    state_stack.Push(sem);
    unsigned i;
    for (i = 0; i < body -> NumNestedClasses(); i++)
    {
        AstClassDeclaration* nested_class = body -> NestedClass(i);
        type -> AddNestedType(ProcessTypeHeaders(nested_class -> class_body));
    }
    for (i = 0; i < body -> NumNestedEnums(); i++)
    {
        AstEnumDeclaration* nested_enum = body -> NestedEnum(i);
        type -> AddNestedType(ProcessTypeHeaders(nested_enum -> class_body));
    }
    for (i = 0; i < body -> NumNestedInterfaces(); i++)
    {
        AstInterfaceDeclaration* nested = body -> NestedInterface(i);
        type -> AddNestedType(ProcessTypeHeaders(nested -> class_body));
    }
    for (i = 0; i < body -> NumNestedAnnotations(); i++)
    {
        AstAnnotationDeclaration* nested = body -> NestedAnnotation(i);
        type -> AddNestedType(ProcessTypeHeaders(nested -> class_body));
    }
    state_stack.Pop();
    return type;
}


void Semantic::ReportTypeInaccessible(TokenIndex left_tok,
                                      TokenIndex right_tok,
                                      TypeSymbol* type)
{
    ReportSemError(SemanticError::TYPE_NOT_ACCESSIBLE, left_tok, right_tok,
                   type -> ContainingPackageName(),
                   type -> ExternalName(), type -> AccessString());
}


//
// Finds an accessible member type named identifier_token within type, or
// returns NULL. Issues an error if there are multiple ambiguous types. The
// caller is responsible for searching for inaccessible member types.
//
TypeSymbol* Semantic::FindNestedType(TypeSymbol* type,
                                     TokenIndex identifier_token)
{
    if (type == control.null_type || type -> Bad() || type -> Primitive())
    {
        return NULL;
    }

    NameSymbol* name_symbol = lex_stream -> NameSymbol(identifier_token);

    if (! type -> expanded_type_table)
        ComputeTypesClosure(type, identifier_token);
    TypeShadowSymbol* type_shadow_symbol =
        type -> expanded_type_table -> FindTypeShadowSymbol(name_symbol);

    return (type_shadow_symbol
            ? FindTypeInShadow(type_shadow_symbol, identifier_token)
            : (TypeSymbol*) NULL);
}


//
// Finds a nested type named name within the enclosing type, and establishes
// a dependence relation. This also searches for inaccessible types, and
// reports an error before returning the inaccessible type. For any other
// error, the return is control.no_type.
//
TypeSymbol* Semantic::MustFindNestedType(TypeSymbol* type, AstName* name)
{
    if (type -> Bad())
        return control.no_type;
    if (name -> base_opt && ! name -> base_opt -> symbol)
        type = MustFindNestedType(type, name -> base_opt);
    TypeSymbol* inner_type = FindNestedType(type, name -> identifier_token);
    if (! inner_type)
    {
        //
        // Before failing completely, check whether or not the user is trying
        // to access an inaccessible nested type.
        //
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(name -> identifier_token);
        for (TypeSymbol* super_type = type -> super;
             super_type && ! super_type -> Bad();
             super_type = super_type -> super)
        {
            assert(super_type -> expanded_type_table);

            TypeShadowSymbol* type_shadow_symbol = super_type ->
                expanded_type_table -> FindTypeShadowSymbol(name_symbol);
            if (type_shadow_symbol)
            {
                inner_type = FindTypeInShadow(type_shadow_symbol,
                                              name -> identifier_token);
                break;
            }
        }

        if (inner_type)
            ReportTypeInaccessible(name, inner_type);
        else inner_type = GetBadNestedType(type, name -> identifier_token);
    }
    return inner_type -> Bad() ? control.no_type : inner_type;
}


//
// Pass 3: Process all method and constructor declarations within the
// compilation unit so that any field initialization enclosed in the
// compilation unit can invoke any constructor or method within the unit.
//
inline void Semantic::ProcessConstructorMembers(AstClassBody* class_body)
{
    TypeSymbol* this_type = ThisType();
    assert(this_type -> HeaderProcessed());
    if (class_body -> NumConstructors())
    {
        for (unsigned i = 0; i < class_body -> NumConstructors(); i++)
            ProcessConstructorDeclaration(class_body -> Constructor(i));
    }
    else if (! this_type -> Anonymous() && ! this_type -> ACC_INTERFACE())
        AddDefaultConstructor(this_type);
    this_type -> MarkConstructorMembersProcessed();
}


inline void Semantic::ProcessMethodMembers(AstClassBody* class_body)
{
    assert(ThisType() -> HeaderProcessed());
    for (unsigned i = 0; i < class_body -> NumMethods(); i++)
        ProcessMethodDeclaration(class_body -> Method(i));
    ThisType() -> MarkMethodMembersProcessed();
}


inline void Semantic::ProcessFieldMembers(AstClassBody* class_body)
{
    assert(ThisType() -> HeaderProcessed());
    unsigned i;
    for (i = 0; i < class_body -> NumInstanceVariables(); i++)
        ProcessFieldDeclaration(class_body -> InstanceVariable(i));
    for (i = 0; i < class_body -> NumClassVariables(); i++)
        ProcessFieldDeclaration(class_body -> ClassVariable(i));
    ThisType() -> MarkFieldMembersProcessed();
}


//
// Process enum constant declarations for enum types
//
inline void Semantic::ProcessEnumConstantMembers(AstClassBody* class_body)
{
    // Check if this class body belongs to an enum
    if (! class_body -> owner)
        return;

    AstEnumDeclaration* enum_decl = class_body -> owner -> EnumDeclarationCast();
    if (! enum_decl)
        return;

    TypeSymbol* enum_type = ThisType();
    assert(enum_type -> HeaderProcessed());
    assert(enum_type -> IsEnum());

    // Process each enum constant
    for (unsigned i = 0; i < enum_decl -> NumEnumConstants(); i++)
    {
        AstEnumConstant* enum_constant = enum_decl -> EnumConstant(i);

        // Set the ordinal (sequential position)
        enum_constant -> ordinal = i;

        // Get the constant's name
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(enum_constant -> identifier_token);

        // Check for duplicate constant names
        if (enum_type -> FindVariableSymbol(name_symbol))
        {
            ReportSemError(SemanticError::DUPLICATE_FIELD,
                           enum_constant -> identifier_token,
                           name_symbol -> Name(),
                           enum_type -> Name(),
                           enum_type -> FindVariableSymbol(name_symbol) -> FileLoc());
            continue;
        }

        // Create a static final field for this enum constant
        VariableSymbol* field = enum_type -> InsertVariableSymbol(name_symbol);
        field -> SetType(enum_type); // Type is the enum type itself
        field -> SetFlags(ProcessEnumConstantModifiers(enum_constant));
        field -> SetOwner(enum_type);
        field -> SetLocation();

        // Link the AST node to the symbol
        enum_constant -> field_symbol = field;

        // Mark as deprecated if needed
        if (lex_stream -> IsDeprecated(enum_constant -> LeftToken()))
            field -> MarkDeprecated();
    }
}


void Semantic::ProcessClassBodyForEffectiveJavaChecks(AstClassBody* class_body)
{
    TypeSymbol* this_type = ThisType();
    assert(this_type -> HeaderProcessed());

    //
    // Find out about this class' constructors:
    // Does it have any non-default constructors?
    // Does it have a private constructor?
    //
    bool has_private_constructor = false;
    bool has_non_default_constructor = false;
    for (unsigned i = 0; i < class_body -> NumConstructors(); ++i)
    {
        AstConstructorDeclaration* constructor_declaration =
            class_body -> Constructor(i);
        if (! constructor_declaration -> IsValid())
            continue;
        MethodSymbol* constructor = constructor_declaration ->
            constructor_symbol;
        if (constructor -> ACC_PRIVATE())
        {
            has_private_constructor = true;
        }

        if (class_body -> default_constructor == NULL ||
            constructor != class_body -> default_constructor -> constructor_symbol)
        {
            has_non_default_constructor = true;
        }
    }

    //
    // Find out about equals and hashCode, and count how many instance methods
    // we have (the NumMethods member function returns the sum of both the
    // class and instance methods).
    //
    bool has_correct_equals_method = false;
    AstMethodDeclaration* equals_method = NULL;
    AstMethodDeclaration* hashCode_method = NULL;
    int instance_method_count = 0;

    for (unsigned i = 0; i < class_body -> NumMethods(); ++i)
    {
        AstMethodDeclaration* method_declaration = class_body -> Method(i);
        if (! method_declaration -> IsValid())
            continue;
        MethodSymbol* method = method_declaration -> method_symbol;
        if (! method -> ACC_STATIC())
        {
            ++instance_method_count;
        }
        if (! this_type -> ACC_INTERFACE() && ! method -> ACC_ABSTRACT())
        {
            //
            // Is it "boolean equals(T other)" for some type T?
            //
            if (method -> name_symbol == control.equals_name_symbol &&
                method -> Type() == control.boolean_type &&
                method -> NumFormalParameters() == 1 &&
                ! has_correct_equals_method)
            {
                equals_method = method_declaration;
                has_correct_equals_method = (method -> FormalParameter(0) ->
                                             Type() == control.Object());
            }

            //
            // Is it "int hashCode()"?
            //
            if (method -> name_symbol == control.hashCode_name_symbol &&
                method -> Type() == control.int_type &&
                method -> NumFormalParameters() == 0)
            {
                hashCode_method = method_declaration;
            }
        }
    }

    //
    // Warn about problems with equals and hashCode.
    //
    if (equals_method != NULL && ! has_correct_equals_method)
    {
        ReportSemError(SemanticError::EJ_AVOID_OVERLOADING_EQUALS,
                       equals_method -> method_declarator -> identifier_token,
                       this_type -> Name());
    }
    if (equals_method != NULL && hashCode_method == NULL)
    {
        ReportSemError(SemanticError::EJ_EQUALS_WITHOUT_HASH_CODE,
                       equals_method -> method_declarator -> identifier_token,
                       this_type -> Name());
    }
    if (equals_method == NULL && hashCode_method != NULL)
    {
        ReportSemError(SemanticError::EJ_HASH_CODE_WITHOUT_EQUALS,
                       hashCode_method -> method_declarator -> identifier_token,
                       this_type -> Name());
    }

    //
    // Warn against utility classes that don't have a private constructor.
    // Empty classes don't count. They're not very useful, but they do
    // exist. The jacks test suite uses one to check the compiler's
    // working, and Sun use them in the Swing Windows plaf, for example.
    //
    bool is_non_empty_class = (class_body -> NumClassVariables() > 0 ||
                               class_body -> NumInstanceVariables() > 0 ||
                               class_body -> NumMethods() > 0);
    bool has_instance_members = (class_body -> NumInstanceVariables() > 0 ||
                                 instance_method_count > 0);
    if (is_non_empty_class &&
        ! has_instance_members &&
        ! this_type -> ACC_INTERFACE() &&
        ! this_type -> ACC_ABSTRACT() &&
        ! this_type -> Anonymous() &&
        ! has_non_default_constructor &&
        ! has_private_constructor &&
        this_type -> super == control.Object())
    {
        ReportSemError(SemanticError::EJ_MISSING_PRIVATE_CONSTRUCTOR,
                       class_body -> identifier_token,
                       this_type -> Name());
    }

    //
    // Warn against interfaces that don't define any behavior.
    //
    if (this_type -> ACC_INTERFACE() &&
        this_type -> super == control.Object() &&
        class_body -> NumMethods() == 0)
    {
        //
        // Tag interfaces such as java.io.Serializable are okay, so we need
        // to check that there is actually something in this interface
        // before we complain about it.
        //
        int field_count = class_body -> NumClassVariables() +
                          class_body -> NumInstanceVariables();
        if (field_count != 0)
        {
            ReportSemError(SemanticError::EJ_INTERFACE_DOES_NOT_DEFINE_TYPE,
                           class_body -> identifier_token,
                           this_type -> Name());
        }
    }

    CheckForSerializationMistakes(class_body);
}


void Semantic::CheckForSerializationMistakes(AstClassBody* class_body)
{
    TypeSymbol* this_type = ThisType();

    if (! this_type -> Implements(control.Serializable()))
        return;

    if (this_type -> IsInner())
    {
        // FIXME: If the class implements the readObject and writeObject
        // methods, it should be okay. But would anyone really do that?
        ReportSemError(SemanticError::EJ_SERIALIZABLE_INNER_CLASS,
                       class_body -> identifier_token);
    }

    //
    // Warn against Serializable classes without an explicit serialVersionUID.
    //
    bool found_serialVersionUID = false;
    for (unsigned i = 0; i < class_body -> NumClassVariables(); ++i)
    {
        AstFieldDeclaration* fd = class_body -> ClassVariable(i);
        for (unsigned j = 0; j < fd -> NumVariableDeclarators(); ++j)
        {
            AstVariableDeclarator* vd = fd -> VariableDeclarator(j);
            NameSymbol* name_symbol = lex_stream -> NameSymbol(vd ->
                variable_declarator_name -> identifier_token);
            if (name_symbol == control.serialVersionUID_name_symbol)
            {
                found_serialVersionUID = true;
            }
        }
    }
    if (! found_serialVersionUID)
    {
        ReportSemError(SemanticError::MISSING_SERIAL_VERSION_UID,
                       class_body -> identifier_token);
    }
}


void Semantic::ProcessMembers(AstClassBody* class_body)
{
    state_stack.Push(class_body -> semantic_environment);
    TypeSymbol* this_type = ThisType();
    unsigned i;

    assert(! this_type -> ConstructorMembersProcessed() || this_type -> Bad());
    assert(! this_type -> MethodMembersProcessed() || this_type -> Bad());
    assert(! this_type -> FieldMembersProcessed() || this_type -> Bad());

    ProcessConstructorMembers(class_body);
    ProcessMethodMembers(class_body);
    ProcessFieldMembers(class_body);
    ProcessEnumConstantMembers(class_body); // Process enum constants if this is an enum

    // Add synthetic methods for enums (values(), valueOf())
    if (this_type -> IsEnum())
        AddEnumSyntheticMethods(this_type);

    ProcessClassBodyForEffectiveJavaChecks(class_body);

    delete this_type -> innertypes_closure; // save some space !!!
    this_type -> innertypes_closure = NULL;

    if (this_type -> IsInner())
    {
        for (i = 0; i < class_body -> NumStaticInitializers(); i++)
        {
            ReportSemError(SemanticError::STATIC_INITIALIZER_IN_INNER_CLASS,
                           class_body -> StaticInitializer(i),
                           this_type -> Name(), this_type -> FileLoc());
        }
    }

    for (i = 0; i < this_type -> NumNestedTypes(); i++)
    {
        TypeSymbol* inner_type = this_type -> NestedType(i);
        ProcessMembers(inner_type -> declaration);
    }
    state_stack.Pop();
}


//
// Pass 4: Process the field declarations at the top level of the types
//
void Semantic::CompleteSymbolTable(AstClassBody* class_body)
{
    if (compilation_unit -> BadCompilationUnitCast())
        return;

    state_stack.Push(class_body -> semantic_environment);
    TypeSymbol* this_type = ThisType();
    TokenIndex identifier = class_body -> identifier_token;

    assert(this_type -> ConstructorMembersProcessed());
    assert(this_type -> MethodMembersProcessed());
    assert(this_type -> FieldMembersProcessed());

    if (! this_type -> expanded_method_table)
        ComputeMethodsClosure(this_type, identifier);

    if (this_type -> super && ! this_type -> Bad())
    {
        if (! this_type -> ACC_ABSTRACT())
        {
            //
            // We already checked that this class does not declare abstract
            // methods. Now see that there are no unimplemented abstract
            // methods in any of the superclasses or superinterfaces. Exploit
            // the fact that the method table is built with the first element
            // being from a superclass; all conflicts are inherited from
            // interfaces and are necessarily abstract.
            //
            ExpandedMethodTable* expanded_table =
                this_type -> expanded_method_table;
            for (unsigned i = 0;
                 i < expanded_table -> symbol_pool.Length(); i++)
            {
                MethodSymbol* method =
                    expanded_table -> symbol_pool[i] -> method_symbol;

                if (method -> ACC_ABSTRACT())
                {
                    TypeSymbol* containing_type = method -> containing_type;
                    if (containing_type != this_type)
                    {
                        if (! method -> IsTyped())
                            method -> ProcessMethodSignature(this, identifier);

                        ReportSemError(SemanticError::NON_ABSTRACT_TYPE_INHERITS_ABSTRACT_METHOD,
                                       identifier, method -> Header(),
                                       containing_type -> ContainingPackageName(),
                                       containing_type -> ExternalName(),
                                       this_type -> ContainingPackageName(),
                                       this_type -> ExternalName());
                    }
                }
            }
        }

        //
        // If any superclass of this_type is abstract and is contained in a
        // different package, check to see if its members include abstract
        // methods with default access. If so, this class must be abstract,
        // as it cannot override them. And if this class has a protected or
        // public method with a conflicting signature, then it is cannot be
        // implemented. However, this can be tricky: suppose abstract p1.A
        // declares abstract foo(), abstract p2.B extends p1.A, abstract p1.C
        // extends p2.B and implements foo(). Then, p2.B does not inherit foo()
        // and thus neither does p1.C, but p1.C DOES override foo() with a
        // valid implementation. And thus, p2.D extends p1.C may be concrete.
        //
        PackageSymbol* package = this_type -> ContainingPackage();
        for (TypeSymbol* super_type = this_type -> super;
             super_type && super_type -> ACC_ABSTRACT();
             super_type = super_type -> super)
        {
            if (super_type -> ContainingPackage() == package)
                continue;

            package = super_type -> ContainingPackage();
            ExpandedMethodTable* super_expanded_table =
                super_type -> expanded_method_table;
            for (unsigned i = 0;
                 i < super_expanded_table -> symbol_pool.Length(); i++)
            {
                MethodSymbol* method =
                    super_expanded_table -> symbol_pool[i] -> method_symbol;

                //
                // Remember that abstract methods cannot be private, and that
                // non-default methods were inherited.
                //
                if (! method -> ACC_ABSTRACT() || method -> ACC_PUBLIC() ||
                    method -> ACC_PROTECTED())
                {
                    continue;
                }
                TypeSymbol* containing_type = method -> containing_type;
                if (! method -> IsTyped())
                    method -> ProcessMethodSignature(this, identifier);

                //
                // Search all intermediate superclasses in the same package
                // as the current super_class for an override of the abstract
                // method in question. Also report any protected or public
                // methods outside super's package that cause this class
                // to be uninstantiable.
                //
                TypeSymbol* intermediate;
                MethodSymbol* method_clash = NULL;
                for (intermediate = this_type;
                     intermediate != super_type;
                     intermediate = intermediate -> super)
                {
                    MethodShadowSymbol* shadow = intermediate ->
                        expanded_method_table ->
                        FindOverloadMethodShadow(method, this, identifier);
                    if (! shadow)
                        continue;
                    if (intermediate -> ContainingPackage() != package)
                    {
                        if ((shadow -> method_symbol -> ACC_PUBLIC() ||
                             shadow -> method_symbol -> ACC_PROTECTED()) &&
                            (shadow -> method_symbol -> Type() !=
                             method -> Type()))
                        {
                            //
                            // No need to repeat the warning for subclasses of
                            // where the problem originally occurred.
                            //
                            if (method_clash == shadow -> method_symbol)
                                method_clash = NULL;
                            else method_clash = shadow -> method_symbol;
                        }
                    }
                    else if (shadow -> method_symbol -> containing_type ==
                             intermediate)
                    {
                        break;
                    }
                }

                if (intermediate == super_type &&
                    ! this_type -> ACC_ABSTRACT())
                {
                    ReportSemError((this_type -> Anonymous()
                                    ? SemanticError::ANONYMOUS_TYPE_CANNOT_OVERRIDE_DEFAULT_ABSTRACT_METHOD
                                    : SemanticError::NON_ABSTRACT_TYPE_CANNOT_OVERRIDE_DEFAULT_ABSTRACT_METHOD),
                                   identifier, method -> Header(),
                                   containing_type -> ContainingPackageName(),
                                   containing_type -> ExternalName(),
                                   this_type -> ContainingPackageName(),
                                   this_type -> ExternalName());
                }
                if (method_clash)
                {
                    TypeSymbol* base_type = method_clash -> containing_type;
                    ReportSemError(SemanticError::UNIMPLEMENTABLE_CLASS,
                                   identifier,
                                   this_type -> ContainingPackageName(),
                                   this_type -> ExternalName(),
                                   method_clash -> Header(),
                                   base_type -> ContainingPackageName(),
                                   base_type -> ExternalName(),
                                   method -> Header(),
                                   containing_type -> ContainingPackageName(),
                                   containing_type -> ExternalName());
                }
            }
        }
    }

    //
    // Compute the set of final variables declared by the user in this type.
    // Then process all variable initializers and initializer blocks.
    //
    DefiniteSetup();
    ProcessStaticInitializers(class_body);
    ProcessInstanceInitializers(class_body);

    //
    // Reset the this_variable and this_method may have been set in
    // ProcessStaticInitializers and/or ProcessInstanceInitializers.
    // Indicate that there is no method being currently compiled
    // in this environment.
    //
    ThisVariable() = NULL;
    ThisMethod() = NULL;

    //
    // Generate bridge methods for generic covariant overrides
    //
    GenerateBridgeMethods(this_type);

    //
    // Recursively process all inner types
    //
    for (unsigned l = 0; l < this_type -> NumNestedTypes(); l++)
    {
        TypeSymbol* inner_type = this_type -> NestedType(l);
        CompleteSymbolTable(inner_type -> declaration);
    }
    state_stack.Pop();
}


//
// Pass 5: Free up unneeded space.
//
void Semantic::CleanUp()
{
    if (control.option.nocleanup)
        return;

    for (unsigned i = 0; i < compilation_unit -> NumTypeDeclarations(); i++)
    {
        AstDeclaredType* type_declaration =
            compilation_unit -> TypeDeclaration(i);
        if (type_declaration -> class_body &&
            type_declaration -> class_body -> semantic_environment)
        {
            CleanUpType(type_declaration -> class_body ->
                        semantic_environment -> Type());
        }
    }
}


void Semantic::CleanUpType(TypeSymbol* type)
{
    type -> DeleteAnonymousTypes();
    for (unsigned i = 0; i < type -> NumNestedTypes(); i++)
        CleanUpType(type -> NestedType(i));

    type -> CompressSpace(); // space optimization

    for (unsigned j = 0; j < type -> NumMethodSymbols(); j++)
        type -> MethodSym(j) -> CleanUp();

    delete type -> local;

    type -> local = NULL;

    delete type -> non_local;
    type -> non_local = NULL;

    delete type -> semantic_environment;
    type -> semantic_environment = NULL;

    type -> declaration = NULL;
}


TypeSymbol* Semantic::ReadType(FileSymbol* file_symbol, PackageSymbol* package,
                               NameSymbol* name_symbol, TokenIndex tok)
{
    TypeSymbol* type;

    if (file_symbol && file_symbol -> IsJava())
    {
        if (! file_symbol -> semantic)
            control.ProcessHeaders(file_symbol);
        type = package -> FindTypeSymbol(name_symbol);
        if (! type)
        {
            type = package -> InsertOuterTypeSymbol(name_symbol);
            type -> MarkBad();
            type -> outermost_type = type;
            type -> supertypes_closure = new SymbolSet;
            type -> subtypes = new SymbolSet;
            type -> semantic_environment =
                new SemanticEnvironment(this, type, NULL);
            if (type != control.Object())
                type -> super = (type == control.Throwable()
                                 ? control.Object() : control.Throwable());
            type -> SetOwner(package);
            type -> SetSignature(control);
            AddDefaultConstructor(type);
            type -> file_symbol = file_symbol;
            file_symbol -> types.Next() = type;

            ReportSemError(SemanticError::TYPE_NOT_FOUND, tok,
                           type -> ContainingPackageName(),
                           type -> ExternalName());
        }
        else if (file_symbol -> semantic -> NumErrors())
        {
            ReportSemError(SemanticError::INVALID_TYPE_FOUND, tok,
                           name_symbol -> Name());
        }
    }
    else if (file_symbol)
    {
        // Read class file.
        type = package -> InsertOuterTypeSymbol(name_symbol);
        type -> outermost_type = type;
        type -> supertypes_closure = new SymbolSet;
        type -> subtypes = new SymbolSet;
        type -> SetOwner(package);
        type -> SetSignature(control);
        type -> file_symbol = file_symbol;
        type -> SetLocation();

        file_symbol -> package = package;
        file_symbol -> types.Next() = type;

        ReadClassFile(type, tok);
        assert (! type -> IsNested());
        control.input_class_file_set.AddElement(file_symbol);
    }
    else
    {
        //
        // No file found. See if a package by the same name exists, otherwise
        // create a placeholder type to avoid errors when the type name is
        // subsequently used.
        //
        PackageSymbol* subpackage = package -> FindPackageSymbol(name_symbol);
        if (! subpackage)
            subpackage = package -> InsertPackageSymbol(name_symbol);
        control.FindPathsToDirectory(subpackage);
        if (subpackage -> directory.Length())
        {
            if (package -> directory.Length())
                ReportSemError(SemanticError::PACKAGE_NOT_TYPE, tok,
                               subpackage -> PackageName());
            type = control.no_type;
        }
        else
        {
            type = package -> InsertOuterTypeSymbol(name_symbol);
            type -> outermost_type = type;
            type -> SetOwner(package);
            type -> SetSignature(control);
            control.ProcessBadType(type);
            type -> MarkBad();
            ReportSemError(SemanticError::TYPE_NOT_FOUND, tok,
                           type -> ContainingPackageName(),
                           type -> ExternalName());
        }
    }
    return type;
}


TypeSymbol* Semantic::GetBadNestedType(TypeSymbol* type,
                                       TokenIndex identifier_token)
{
    NameSymbol* name_symbol = lex_stream -> NameSymbol(identifier_token);

    TypeSymbol* outermost_type = type -> outermost_type;
    if (! outermost_type -> non_local)
        outermost_type -> non_local = new SymbolSet;
    if (! outermost_type -> local)
        outermost_type -> local = new SymbolSet;

    int length = type -> ExternalNameLength() + 1 +
        name_symbol -> NameLength(); // +1 for $,... +1 for $
    wchar_t* external_name = new wchar_t[length + 1]; // +1 for '\0';
    wcscpy(external_name, type -> ExternalName());
    wcscat(external_name, StringConstant::US_DS);
    wcscat(external_name, name_symbol -> Name());

    TypeSymbol* inner_type = type -> InsertNestedTypeSymbol(name_symbol);
    inner_type -> MarkBad();
    inner_type -> outermost_type = type -> outermost_type;
    inner_type -> supertypes_closure = new SymbolSet;
    inner_type -> subtypes = new SymbolSet;
    inner_type -> SetExternalIdentity(control.FindOrInsertName(external_name,
                                                               length));
    inner_type -> super = control.Object();
    inner_type -> SetOwner(type);
    if (! type -> Bad())
        ReportSemError(SemanticError::TYPE_NOT_FOUND, identifier_token,
                       inner_type -> ContainingPackageName(),
                       inner_type -> ExternalName());

    delete [] external_name;

    return inner_type;
}


void Semantic::ProcessImportQualifiedName(AstName* name)
{
    if (name -> base_opt)
    {
        ProcessImportQualifiedName(name -> base_opt);
        Symbol* symbol = name -> base_opt -> symbol;

        TypeSymbol* type = symbol -> TypeCast();
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(name -> identifier_token);
        if (type) // The base name is a type
        {
            if (type -> Bad()) // Avoid chain-reaction errors.
            {
                name -> symbol = control.no_type;
                return;
            }
            if (! type -> expanded_type_table)
                ComputeTypesClosure(type, name -> identifier_token);
            TypeSymbol* inner_type = NULL;
            TypeShadowSymbol* type_shadow_symbol = type ->
                expanded_type_table -> FindTypeShadowSymbol(name_symbol);
            //
            // Only canonical names may be used in import statements, so we
            // don't worry about ambiguous names (which are necessarily
            // inherited and hence non-canonical). But we do need an extra
            // filter on the containing type being correct.
            //
            if (type_shadow_symbol)
                inner_type = type_shadow_symbol -> type_symbol;
            if (! inner_type)
                inner_type = control.no_type;
            else if (type != inner_type -> owner)
            {
                ReportSemError(SemanticError::IMPORT_NOT_CANONICAL,
                               name, name_symbol -> Name(),
                               inner_type -> ContainingPackageName(),
                               inner_type -> ExternalName());
            }
            else if (inner_type -> ACC_PRIVATE() ||
                     (! inner_type -> ACC_PUBLIC() &&
                      inner_type -> ContainingPackage() != this_package))
            {
                ReportTypeInaccessible(name, inner_type);
            }
            name -> symbol = inner_type;
        }
        else
        {
            PackageSymbol* package = symbol -> PackageCast();
            type = package -> FindTypeSymbol(name_symbol);
            if (! type)
            {
                FileSymbol* file_symbol =
                    Control::GetFile(control, package, name_symbol);
                if (file_symbol)
                    type = ReadType(file_symbol, package, name_symbol,
                                    name -> identifier_token);
            }
            else if (type -> SourcePending())
                control.ProcessHeaders(type -> file_symbol);

            //
            // If the field_access was resolved to a type, save it later use.
            // Otherwise, assume the field_access is a package name.
            //
            if (type)
            {
                if (! type -> ACC_PUBLIC() &&
                    type -> ContainingPackage() != this_package)
                {
                    ReportTypeInaccessible(name, type);
                }
                name -> symbol = type;
            }
            else
            {
                PackageSymbol* subpackage =
                    package -> FindPackageSymbol(name_symbol);
                if (! subpackage)
                    subpackage = package -> InsertPackageSymbol(name_symbol);
                control.FindPathsToDirectory(subpackage);
                name -> symbol = subpackage;
            }
        }
    }
    else // unqualified name
    {
        //
        // JLS 6.3 The leading simple name of a type import must be a package
        // name, as class names are not in scope. JLS 7.5: Nested classes of
        // all sorts (top-level or inner) can be imported by either kind of
        // import statement. Class names in import statements must be the
        // canonical version.
        //
        TypeSymbol* type = FindSimpleNameType(control.UnnamedPackage(),
                                              name -> identifier_token);

        //
        // If the name is a type, detect the error. Otherwise, assume
        // it is a package, and legal.
        //
        if (type)
        {
            ReportSemError(SemanticError::IMPORT_FROM_UNNAMED_PACKAGE,
                           name -> identifier_token,
                           lex_stream -> NameString(name -> identifier_token));
            name -> symbol = control.no_type;
        }
        else
        {
            NameSymbol* name_symbol =
                lex_stream -> NameSymbol(name -> identifier_token);
            PackageSymbol* package =
                control.external_table.FindPackageSymbol(name_symbol);
            if (! package)
                package = control.external_table.
                    InsertPackageSymbol(name_symbol, NULL);
            control.FindPathsToDirectory(package);
            name -> symbol = package;
        }
    }
}


//
// Processes a package-or-type name. If an accessible type exists, it is
// chosen. Next, if a package exists, it is chosen. Then, an error is issued,
// but a check for an inaccessible type is made before inventing a package.
// The result is stored in name->symbol.
//
void Semantic::ProcessPackageOrType(AstName* name)
{
    if (name -> base_opt)
    {
        ProcessPackageOrType(name -> base_opt);
        Symbol* symbol = name -> base_opt -> symbol;

        TypeSymbol* type = symbol -> TypeCast();
        if (type) // The base name is a type
            name -> symbol = MustFindNestedType(type, name);
        else
        {
            // Base name is package. Search for type, then subpackage.
            PackageSymbol* package = symbol -> PackageCast();
            NameSymbol* name_symbol =
                lex_stream -> NameSymbol(name -> identifier_token);
            type = package -> FindTypeSymbol(name_symbol);
            if (! type)
            {
                FileSymbol* file_symbol =
                    Control::GetFile(control, package, name_symbol);
                if (file_symbol)
                    type = ReadType(file_symbol, package, name_symbol,
                                    name -> identifier_token);
            }
            else if (type -> SourcePending())
                control.ProcessHeaders(type -> file_symbol);

            //
            // If the field access was resolved into a type, then save it.
            // Otherwise, assume it is a package
            //
            if (type)
                // save the resolved type of this expression for later use...
                name -> symbol = type;
            else
            {
                NameSymbol* name_symbol =
                    lex_stream -> NameSymbol(name -> identifier_token);
                PackageSymbol* subpackage =
                    package -> FindPackageSymbol(name_symbol);
                if (! subpackage)
                    subpackage = package -> InsertPackageSymbol(name_symbol);
                control.FindPathsToDirectory(subpackage);
                name -> symbol = subpackage;
                if (subpackage -> directory.Length() == 0)
                {
                    ReportSemError(SemanticError::PACKAGE_NOT_FOUND,
                                   name -> identifier_token,
                                   subpackage -> PackageName());
                }
            }
        }
    }
    else // unqualified name
    {
        TypeSymbol* type = FindType(name -> identifier_token);
        if (type)
            name -> symbol = type;
        else
        {
            NameSymbol* name_symbol =
                lex_stream -> NameSymbol(name -> identifier_token);
            PackageSymbol* package =
                control.external_table.FindPackageSymbol(name_symbol);
            if (! package)
                package = control.external_table.
                    InsertPackageSymbol(name_symbol, NULL);
            control.FindPathsToDirectory(package);
            if (package -> directory.Length() == 0)
            {
                //
                // If there is no package, see if the user is trying to access
                // an inaccessible nested type before giving up.
                //
                if (state_stack.Size())
                {
                    NameSymbol* name_symbol = lex_stream ->
                        NameSymbol(name -> identifier_token);
                    for (TypeSymbol* super_type = ThisType() -> super;
                         super_type; super_type = super_type -> super)
                    {
                        assert(super_type -> expanded_type_table);

                        TypeShadowSymbol* type_shadow_symbol =
                            super_type -> expanded_type_table ->
                            FindTypeShadowSymbol(name_symbol);
                        if (type_shadow_symbol)
                        {
                            type = type_shadow_symbol -> type_symbol;
                            break;
                        }
                    }
                }
                if (type)
                {
                    ReportTypeInaccessible(name, type);
                    name -> symbol = type;
                }
                else
                {
                    ReportSemError(SemanticError::PACKAGE_NOT_FOUND,
                                   name -> identifier_token,
                                   package -> PackageName());
                    name -> symbol = package;
                }
            }
            else name -> symbol = package;
        }
    }
}


void Semantic::ProcessTypeImportOnDemandDeclaration(AstImportDeclaration* import_declaration)
{
    ProcessImportQualifiedName(import_declaration -> name);
    Symbol* symbol = import_declaration -> name -> symbol;

    PackageSymbol* package = symbol -> PackageCast();
    if (package && package -> directory.Length() == 0)
    {
        ReportSemError(SemanticError::PACKAGE_NOT_FOUND,
                       import_declaration -> name,
                       package -> PackageName());
    }

    //
    // Two or more type-import-on-demand may name the same package; the effect
    // is as if there were only one such declaration. Likewise, importing the
    // current package or java.lang.* is ok, although useless.
    // TODO: In pedantic mode, warn about duplicate imports of the same
    // package, of the current package, or of java.lang.*.
    //
    if (symbol == this_package)
        return;
    for (unsigned i = 0; i < import_on_demand_packages.Length(); i++)
    {
        if (symbol == import_on_demand_packages[i])
            return;
    }

    import_on_demand_packages.Next() = symbol;

    TypeSymbol* type = symbol -> TypeCast();
    if (control.option.deprecation && type &&
        type -> IsDeprecated() && type -> file_symbol != source_file_symbol)
    {
        ReportSemError(SemanticError::DEPRECATED_TYPE,
                       import_declaration -> name,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }
}


TypeSymbol* Semantic::FindSimpleNameType(PackageSymbol* package,
                                         TokenIndex identifier_token)
{
    NameSymbol* name_symbol = lex_stream -> NameSymbol(identifier_token);
    TypeSymbol* type = package -> FindTypeSymbol(name_symbol);
    if (type)
    {
        if (type -> SourcePending())
            control.ProcessHeaders(type -> file_symbol);
    }
    else
    {
        //
        // Check whether or not the type was declared in another compilation
        // unit in the main package.
        //
        FileSymbol* file_symbol = Control::GetFile(control, package,
                                                   name_symbol);
        if (file_symbol)
            type = ReadType(file_symbol, package, name_symbol,
                            identifier_token);
    }

    return type;
}

void Semantic::ProcessSingleTypeImportDeclaration(AstImportDeclaration* import_declaration)
{
    ProcessImportQualifiedName(import_declaration -> name);
    Symbol* symbol = import_declaration -> name -> symbol;
    PackageSymbol* package = symbol -> PackageCast();
    TypeSymbol* type = symbol -> TypeCast();

    //
    // Technically, the JLS grammar forbids "import foo;". However, our
    // grammar parses it, and will either find or create the package foo, so
    // we can give a better message than "expected '.'". If a non-type is
    // imported, we create a place-holder type so that the use of the
    // unqualified type name won't cause cascading errors elsewhere.
    //
    if (package)
    {
        ReportSemError(SemanticError::UNKNOWN_ON_DEMAND_IMPORT,
                       import_declaration -> name,
                       package -> PackageName());
        NameSymbol* name_symbol = lex_stream ->
            NameSymbol(import_declaration -> name -> RightToken());
        type = package -> InsertOuterTypeSymbol(name_symbol);
        type -> MarkBad();
        type -> super = control.no_type;
        type -> outermost_type = control.no_type;
    }

    //
    // If two single-type-import declarations in the same compilation unit
    // attempt to import types with the same simple name, then a compile-time
    // error occurs, unless the two types are the same type, in which case the
    // duplicate declaration is ignored.
    // TODO: Give pedantic warnings about duplicate type declarations.
    //
    for (unsigned i = 0; i < single_type_imports.Length(); i++)
    {
        if (type == single_type_imports[i])
            return;
    }

    TypeSymbol* old_type = NULL;
    unsigned k;
    for (k = 0; k < compilation_unit -> NumTypeDeclarations(); k++)
    {
        AstDeclaredType* declaration = compilation_unit -> TypeDeclaration(k);
        if (declaration -> class_body &&
            declaration -> class_body -> semantic_environment)
        {
            old_type =
                declaration -> class_body -> semantic_environment -> Type();
            if (old_type -> Identity() == type -> Identity())
                break;
        }
    }

    if (k < compilation_unit -> NumTypeDeclarations())
    {
        AstName* name = import_declaration -> name;
        package = name -> base_opt
            ? name -> base_opt -> symbol -> PackageCast()
            : control.UnnamedPackage();

        //
        // It's ok to import a type that is being compiled...
        //
        if (type == old_type && package == this_package)
        {
            ReportSemError(SemanticError::UNNECESSARY_TYPE_IMPORT, name,
                           lex_stream -> NameString(name -> identifier_token),
                           old_type -> FileLoc());
        }
        else
        {
            ReportSemError(SemanticError::DUPLICATE_IMPORT_NAME, name,
                           lex_stream -> NameString(name -> identifier_token),
                           old_type -> FileLoc());
        }
    }
    else
    {
        unsigned i = 0;
        for (i = 0; i < compilation_unit -> NumImportDeclarations(); i++)
        {
            TypeSymbol* other_type =
                compilation_unit -> ImportDeclaration(i) -> name -> Type();
            if (compilation_unit -> ImportDeclaration(i) == import_declaration ||
                (other_type && other_type -> Identity() == type -> Identity()))
            {
                break;
            }
        }

        assert(i < compilation_unit -> NumImportDeclarations());

        if (compilation_unit -> ImportDeclaration(i) == import_declaration)
        {
            // No duplicate found
            import_declaration -> name -> symbol = type;
            single_type_imports.Next() = type;
        }
        else
        {
            AstName* name = compilation_unit -> ImportDeclaration(i) -> name;
            FileLocation file_location(lex_stream, name -> identifier_token);
            ReportSemError(SemanticError::DUPLICATE_IMPORT_NAME, name,
                           lex_stream -> NameString(name -> identifier_token),
                           file_location.location);
        }
    }

    if (control.option.deprecation && type -> IsDeprecated() &&
        type -> file_symbol != source_file_symbol)
    {
        ReportSemError(SemanticError::DEPRECATED_TYPE,
                       import_declaration -> name,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }
}


//
// Java 5: Process a single static import declaration
// Example: import static java.lang.Math.PI;
//
void Semantic::ProcessSingleStaticImportDeclaration(AstImportDeclaration* import_declaration)
{
    AstName* name = import_declaration -> name;

    // For "import static pkg.Type.member", we need to:
    // 1. Resolve pkg.Type (everything except the last identifier)
    // 2. Find the static member in that type

    // The name should be qualified (e.g., java.lang.Math.PI)
    if (!name -> base_opt)
    {
        ReportSemError(SemanticError::INVALID_STATIC_IMPORT,
                       name -> identifier_token,
                       lex_stream -> NameString(name -> identifier_token));
        return;
    }

    // Resolve the type part (everything except the last component)
    ProcessImportQualifiedName(name -> base_opt);

    // Check if resolution succeeded
    if (!name -> base_opt || !name -> base_opt -> symbol)
    {
        // Resolution failed, error already reported
        return;
    }

    Symbol* symbol = name -> base_opt -> symbol;

    // The symbol must be a type, not a package
    PackageSymbol* package = symbol -> PackageCast();
    TypeSymbol* type = symbol -> TypeCast();

    if (package || !type)
    {
        ReportSemError(SemanticError::INVALID_STATIC_IMPORT,
                       name -> identifier_token,
                       lex_stream -> NameString(name -> identifier_token));
        return;
    }

    if (type -> Bad())
    {
        // Avoid cascading errors
        return;
    }

    // Get the member name (last component)
    if (!name || name -> identifier_token == 0)
    {
        // Invalid token
        return;
    }

    NameSymbol* member_name = lex_stream -> NameSymbol(name -> identifier_token);
    if (!member_name)
    {
        // Failed to get member name
        return;
    }

    // Store the import information for lazy resolution during name lookup
    // We don't validate that the member exists here because:
    // 1. The type might not be fully processed yet (source files)
    // 2. We want to defer resolution until the member is actually used
    // 3. This avoids circular dependency issues during compilation

    // Check for duplicates
    for (unsigned i = 0; i < single_static_imports.Length(); i++)
    {
        StaticImportInfo* info = single_static_imports[i];
        if (!info)
            continue;
        if (info -> type == type && info -> member_name == member_name)
            return; // Duplicate, ignore
    }

    // Create and store the import info
    StaticImportInfo* info = new StaticImportInfo();
    if (!info)
    {
        // Out of memory?
        return;
    }

    info -> type = type;
    info -> member_name = member_name;
    single_static_imports.Next() = info;
}


//
// Java 5: Process a static import-on-demand declaration
// Example: import static java.lang.Math.*;
//
void Semantic::ProcessStaticImportOnDemandDeclaration(AstImportDeclaration* import_declaration)
{
    ProcessImportQualifiedName(import_declaration -> name);
    TypeSymbol* type = import_declaration -> name -> Type();

    if (!type)
    {
        // Error already reported
        return;
    }

    // Ensure at least headers are processed
    if (type -> SourcePending())
        control.ProcessHeaders(type -> file_symbol);

    // Check for duplicates
    for (unsigned i = 0; i < static_on_demand_imports.Length(); i++)
    {
        if (static_on_demand_imports[i] == type)
            return;
    }

    static_on_demand_imports.Next() = type;
}


void Semantic::ProcessFieldDeclaration(AstFieldDeclaration* field_declaration)
{
    TypeSymbol* this_type = ThisType();
    AccessFlags access_flags = this_type -> ACC_INTERFACE()
        ? ProcessInterfaceFieldModifiers(field_declaration)
        : ProcessFieldModifiers(field_declaration);

    //
    // JLS2 8.1.2 - Inner classes may not have static fields unless they are
    // final and initialized by a constant.  Hence, the type of the static
    // field may only be a primitive or String.  Here, we check that the
    // entire declaration is final, then that each variableDeclarator is
    // of the right type and is initialized.  Later, when processing the
    // initializer, we check that it is indeed a compile-time constant
    // (see init.cpp, Semantic::ProcessVariableInitializer)
    //
    bool must_be_constant = false;
    if (this_type -> IsInner() && access_flags.ACC_STATIC())
    {
        if (access_flags.ACC_FINAL())
            must_be_constant = true;
        else
        {
            assert(field_declaration -> modifiers_opt &&
                   field_declaration -> modifiers_opt -> static_token_opt);
            ReportSemError(SemanticError::STATIC_FIELD_IN_INNER_CLASS_NOT_FINAL,
                           field_declaration -> modifiers_opt -> static_token_opt,
                           this_type -> Name(),
                           this_type -> FileLoc());
        }
    }

    //
    // To avoid deprecated type warnings when processing a deprecated field
    // declaration, we must temporarily mark this type as deprecated, because
    // the field variable symbol(s) do not yet exist.
    //
    bool deprecated_declarations =
        lex_stream -> IsDeprecated(field_declaration -> LeftToken());
    bool deprecated_type = this_type -> IsDeprecated();
    if (deprecated_declarations)
        this_type -> MarkDeprecated();
    ProcessType(field_declaration -> type);
    TypeSymbol* field_type = field_declaration -> type -> symbol;
    if (! deprecated_type && deprecated_declarations)
        this_type -> ResetDeprecated();

    for (unsigned i = 0;
         i < field_declaration -> NumVariableDeclarators(); i++)
    {
        AstVariableDeclarator* variable_declarator =
            field_declaration -> VariableDeclarator(i);
        AstVariableDeclaratorId* name =
            variable_declarator -> variable_declarator_name;
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(name -> identifier_token);

        if (this_type -> FindVariableSymbol(name_symbol))
        {
            ReportSemError(SemanticError::DUPLICATE_FIELD,
                           name -> identifier_token, name_symbol -> Name(),
                           this_type -> Name(),
                           this_type -> FindVariableSymbol(name_symbol) -> FileLoc());
        }
        else
        {
            if (name_symbol != control.serialVersionUID_name_symbol)
            {
                WarnOfAccessibleFieldWithName(SemanticError::HIDDEN_FIELD,
                                              name, name_symbol, access_flags.ACC_STATIC());
            }

            VariableSymbol* variable =
                this_type -> InsertVariableSymbol(name_symbol);
            unsigned dims =
                field_type -> num_dimensions + name -> NumBrackets();
            variable -> SetType(field_type -> GetArrayType(this, dims));
            variable -> SetFlags(access_flags);
            variable -> SetOwner(this_type);
            variable -> declarator = variable_declarator;
            variable -> field_declaration = field_declaration;

            // Copy parameterized type information if present
            AstTypeName* type_name = field_declaration -> type -> TypeNameCast();
            if (type_name && type_name -> parameterized_type)
            {
                variable -> parameterized_type = type_name -> parameterized_type;
            }
            if (must_be_constant &&
                (dims || ! variable_declarator -> variable_initializer_opt ||
                 (! field_type -> Primitive() &&
                  field_type != control.String())))
            {
                ReportSemError(SemanticError::STATIC_FIELD_IN_INNER_CLASS_NOT_CONSTANT,
                               name -> identifier_token, name_symbol -> Name(),
                               this_type -> Name(), this_type -> FileLoc());
            }
            variable_declarator -> symbol = variable;
            variable -> SetLocation();
            if (deprecated_declarations)
                variable -> MarkDeprecated();
        }

        CheckFieldDeclaration(field_declaration, name, access_flags);
    }
}


void Semantic::CheckFieldDeclaration(AstFieldDeclaration* field_declaration,
                                     AstVariableDeclaratorId* name,
                                     const AccessFlags& access_flags)
{
    TypeSymbol* field_type = field_declaration -> type -> symbol;
    NameSymbol* name_symbol = lex_stream ->
        NameSymbol(name -> identifier_token);

    //
    // Warn against public static final array fields.
    //
    bool is_constant_field = (access_flags.ACC_FINAL() &&
                              access_flags.ACC_STATIC());
    if (access_flags.ACC_PUBLIC() &&
        is_constant_field &&
        field_type -> IsArray())
    {
        // FIXME: shouldn't warn if it's a zero-length array.
        ReportSemError(SemanticError::EJ_PUBLIC_STATIC_FINAL_ARRAY_FIELD,
                       name, name_symbol -> Name());
    }

    if (name_symbol == control.serialVersionUID_name_symbol)
    {
        //
        // Warn about serialVersionUID mistakes.
        //
        TypeSymbol* this_type = ThisType();
        bool is_serializable = this_type -> Implements(control.Serializable());
        if (! is_serializable)
        {
            ReportSemError(SemanticError::UNNEEDED_SERIAL_VERSION_UID,
                           name);
        }
        else if (field_type != control.long_type ||
                 is_constant_field == false ||
                 access_flags.ACC_PUBLIC() ||
                 access_flags.ACC_PROTECTED())
        {
            ReportSemError(SemanticError::BAD_SERIAL_VERSION_UID,
                           field_declaration);
        }
    }
    else if (name_symbol == control.serialPersistentFields_name_symbol)
    {
        //
        // FIXME: Warn about serialPersistentFields mistakes; has anyone
        // ever seen this used in the wild?
        //
    }
    else
    {
        //
        // Warn against unconventional field names. This doesn't apply to
        // the serialization fields above, because their names are mandated
        // names that break conventions, and aren't likely to be fixed.
        //
        CheckFieldName(name, name_symbol, is_constant_field);
    }
}


void Semantic::CheckFieldName(AstVariableDeclaratorId* name,
                              NameSymbol* name_symbol,
                              bool is_constant_field)
{
    if (is_constant_field &&
        name_symbol -> IsBadStyleForConstantField())
    {
        ReportSemError(SemanticError::UNCONVENTIONAL_CONSTANT_FIELD_NAME,
                       name -> identifier_token, name_symbol -> Name());
    }
    else if (! is_constant_field &&
             name_symbol -> IsBadStyleForField())
    {
        ReportSemError(SemanticError::UNCONVENTIONAL_FIELD_NAME,
                       name -> identifier_token, name_symbol -> Name());
    }
}


void Semantic::ProcessConstructorDeclaration(AstConstructorDeclaration* constructor_declaration)
{
    TypeSymbol* this_type = ThisType();

    AccessFlags access_flags =
        ProcessConstructorModifiers(constructor_declaration);
    if (this_type -> ACC_STRICTFP())
        access_flags.SetACC_STRICTFP();

    // Process type parameters for generic constructors
    // Generic constructors have their own type parameters separate from the class
    // Example: <T> MyClass(T arg) { ... }
    MethodSymbol* temp_constructor = NULL;
    if (constructor_declaration -> type_parameters_opt)
    {
        // Create temporary constructor to hold type parameters during processing
        temp_constructor = new MethodSymbol(control.init_name_symbol);
        temp_constructor -> SetContainingType(this_type);

        MethodSymbol* saved_method = ThisMethod();
        ThisMethod() = temp_constructor;

        ProcessMethodTypeParameters(temp_constructor,
                                    constructor_declaration -> type_parameters_opt);

        ThisMethod() = saved_method;
    }

    AstMethodDeclarator* constructor_declarator =
        constructor_declaration -> constructor_declarator;
    const wchar_t* constructor_name =
        lex_stream -> NameString(constructor_declarator -> identifier_token);

    //
    // A bad name indicates either a misspelling, or a method missing
    // a return type.  In an anonymous class, assume a missing return
    // type.  In all other classes, if the probability of misspelling
    // >= 50%, correct the name, otherwise treat it as a method with
    // bad return type.
    //
    bool treat_as_method = false;
    if (this_type -> Anonymous())
    {
        ReportSemError(SemanticError::CONSTRUCTOR_FOUND_IN_ANONYMOUS_CLASS,
                       constructor_declarator, constructor_name);
        treat_as_method = true;
    }
    else if (lex_stream -> NameSymbol(constructor_declarator -> identifier_token) != this_type -> Identity())
    {
        if (Spell::Index(constructor_name, this_type -> Name()) >= 5)
        {
            ReportSemError(SemanticError::MISSPELLED_CONSTRUCTOR_NAME,
                           constructor_declarator -> identifier_token,
                           constructor_name, this_type -> Name());
            constructor_name = this_type -> Name(); // correct the name
        }
        else
        {
            ReportSemError(SemanticError::MISMATCHED_CONSTRUCTOR_NAME,
                           constructor_declarator -> identifier_token,
                           constructor_name, this_type -> Name());
            treat_as_method = true;
        }
    }

    //
    // As the body of the constructor may not have been parsed yet, we estimate
    // a size for its symbol table based on the number of lines in the body + a
    // margin for one-liners.
    //
    BlockSymbol* block_symbol =
        new BlockSymbol(constructor_declarator -> NumFormalParameters() + 3);
    // All types need a spot for "this".
    block_symbol -> max_variable_index = 1;

    // Enum constructors have two implicit parameters: String name, int ordinal
    // These must be added before user-defined parameters
    // But NOT for java.lang.Enum itself - it declares them explicitly
    VariableSymbol* enum_name_param = NULL;
    VariableSymbol* enum_ordinal_param = NULL;
    if (this_type -> IsEnum() && this_type != control.Enum())
    {
        // Add name parameter (String)
        NameSymbol* name_sym = control.FindOrInsertName(L"name", 4);
        enum_name_param = block_symbol -> InsertVariableSymbol(name_sym);
        enum_name_param -> SetType(control.String());
        enum_name_param -> SetLocalVariableIndex(block_symbol -> max_variable_index++);
        enum_name_param -> MarkComplete();
        enum_name_param -> SetACC_SYNTHETIC();

        // Add ordinal parameter (int)
        NameSymbol* ordinal_sym = control.FindOrInsertName(L"ordinal", 7);
        enum_ordinal_param = block_symbol -> InsertVariableSymbol(ordinal_sym);
        enum_ordinal_param -> SetType(control.int_type);
        enum_ordinal_param -> SetLocalVariableIndex(block_symbol -> max_variable_index++);
        enum_ordinal_param -> MarkComplete();
        enum_ordinal_param -> SetACC_SYNTHETIC();
    }

    ProcessFormalParameters(block_symbol, constructor_declarator);

    //
    // Note that constructors are always named "<init>", but if this is a
    // method with missing return type, we use the method name.
    //
    NameSymbol* name_symbol = treat_as_method
        ? lex_stream -> NameSymbol(constructor_declarator -> identifier_token)
        : control.init_name_symbol;
    MethodSymbol* constructor = this_type -> FindMethodSymbol(name_symbol);
    if (constructor && this_type -> FindOverloadMethod(constructor,
                                                       constructor_declarator))
    {
        ReportSemError(SemanticError::DUPLICATE_CONSTRUCTOR,
                       constructor_declarator, this_type -> Name(),
                       constructor -> FileLoc());
        delete block_symbol;
        return;
    }

    constructor = this_type -> InsertMethodSymbol(name_symbol);
    TypeSymbol* ctor_type = this_type;
    if (treat_as_method)
        ctor_type = control.no_type;
    constructor -> SetType(ctor_type);
    constructor -> SetFlags(access_flags);
    constructor -> SetContainingType(this_type);
    constructor -> SetBlockSymbol(block_symbol);
    constructor -> declaration = constructor_declaration;
    constructor -> SetLocation();
    if (this_type -> EnclosingType())
    {
        VariableSymbol* this0_variable =
            block_symbol -> InsertVariableSymbol(control.this_name_symbol);
        this0_variable -> SetType(this_type -> ContainingType());
        this0_variable -> SetOwner(constructor);
        this0_variable -> SetLocalVariableIndex(block_symbol ->
                                                max_variable_index++);
        this0_variable -> MarkComplete();
        this0_variable -> SetACC_SYNTHETIC();
    }

    // Add enum implicit parameters to constructor first
    if (enum_name_param)
    {
        enum_name_param -> SetOwner(constructor);
        constructor -> AddFormalParameter(enum_name_param);
    }
    if (enum_ordinal_param)
    {
        enum_ordinal_param -> SetOwner(constructor);
        constructor -> AddFormalParameter(enum_ordinal_param);
    }

    for (unsigned i = 0;
         i < constructor_declarator -> NumFormalParameters(); i++)
    {
        AstVariableDeclarator* formal_declarator =
            constructor_declarator -> FormalParameter(i) -> formal_declarator;
        VariableSymbol* symbol = formal_declarator -> symbol;

        symbol -> SetOwner(constructor);
        symbol -> SetLocalVariableIndex(block_symbol -> max_variable_index++);
        symbol -> MarkComplete();
        if (control.IsDoubleWordType(symbol -> Type()))
            block_symbol -> max_variable_index++;
        symbol -> declarator = formal_declarator;
        symbol -> SetLocation();
        constructor -> AddFormalParameter(symbol);
    }

    // Transfer type parameters from temp constructor to actual constructor
    if (temp_constructor && temp_constructor -> NumTypeParameters() > 0)
    {
        for (unsigned i = 0; i < temp_constructor -> NumTypeParameters(); i++)
        {
            TypeParameterSymbol* type_param = temp_constructor -> TypeParameter(i);
            type_param -> owner = constructor;  // Update owner to actual constructor
            constructor -> AddTypeParameter(type_param);
        }
        delete temp_constructor;  // Clean up temporary
    }

    constructor -> SetSignature(control);

    for (unsigned k = 0; k < constructor_declaration -> NumThrows(); k++)
    {
        AstTypeName* throw_expr = constructor_declaration -> Throw(k);
        ProcessType(throw_expr);
        constructor -> AddThrows(throw_expr -> symbol);
    }

    // save for processing bodies later.
    constructor_declaration -> constructor_symbol = constructor;

    if (lex_stream -> IsDeprecated(constructor_declaration -> LeftToken()))
        constructor -> MarkDeprecated();
}


void Semantic::AddDefaultConstructor(TypeSymbol* type)
{
    assert(! type -> ACC_INTERFACE());
    MethodSymbol* constructor =
        type -> InsertMethodSymbol(control.init_name_symbol);

    BlockSymbol* block_symbol = new BlockSymbol(1);
    block_symbol -> max_variable_index = 1; // All types need a spot for "this"

    constructor -> SetType(type);
    constructor -> SetContainingType(type);
    constructor -> SetBlockSymbol(block_symbol);

    // Enum constructors must be private
    if (type -> IsEnum())
        constructor -> SetACC_PRIVATE();
    else if (type -> ACC_PUBLIC())
        constructor -> SetACC_PUBLIC();
    else if (type -> ACC_PROTECTED())
        constructor -> SetACC_PROTECTED();
    else if (type -> ACC_PRIVATE())
        constructor -> SetACC_PRIVATE();

    if (type -> ACC_STRICTFP())
        constructor -> SetACC_STRICTFP();

    // Enum constructors have two implicit parameters: String name, int ordinal
    // But NOT for java.lang.Enum itself - it declares them explicitly
    if (type -> IsEnum() && type != control.Enum())
    {
        // Add name parameter (String)
        NameSymbol* name_sym = control.FindOrInsertName(L"name", 4);
        VariableSymbol* name_param = block_symbol -> InsertVariableSymbol(name_sym);
        name_param -> SetType(control.String());
        name_param -> SetOwner(constructor);
        name_param -> SetLocalVariableIndex(block_symbol -> max_variable_index++);
        name_param -> MarkComplete();
        name_param -> SetACC_SYNTHETIC();
        constructor -> AddFormalParameter(name_param);

        // Add ordinal parameter (int)
        NameSymbol* ordinal_sym = control.FindOrInsertName(L"ordinal", 7);
        VariableSymbol* ordinal_param = block_symbol -> InsertVariableSymbol(ordinal_sym);
        ordinal_param -> SetType(control.int_type);
        ordinal_param -> SetOwner(constructor);
        ordinal_param -> SetLocalVariableIndex(block_symbol -> max_variable_index++);
        ordinal_param -> MarkComplete();
        ordinal_param -> SetACC_SYNTHETIC();
        constructor -> AddFormalParameter(ordinal_param);
    }

    if (type -> EnclosingType())
    {
        VariableSymbol* this0_variable =
            block_symbol -> InsertVariableSymbol(control.this_name_symbol);
        this0_variable -> SetType(type -> ContainingType());
        this0_variable -> SetOwner(constructor);
        this0_variable -> SetLocalVariableIndex(block_symbol ->
                                                max_variable_index++);
        this0_variable -> MarkComplete();
        this0_variable -> SetACC_SYNTHETIC();
    }
    constructor -> SetSignature(control);

    AstClassBody* class_body = type -> declaration;
    if (class_body)
    {
        TokenIndex left_loc = class_body -> identifier_token;
        TokenIndex right_loc = class_body -> left_brace_token - 1;

        AstMethodDeclarator* method_declarator =
            compilation_unit -> ast_pool -> GenMethodDeclarator();
        method_declarator -> identifier_token = left_loc;
        method_declarator -> left_parenthesis_token = left_loc;
        method_declarator -> right_parenthesis_token = right_loc;

        // Enum AST formal parameters are created implicitly - the signature
        // is set from the block symbol's variables above

        AstSuperCall* super_call = NULL;
        if (type != control.Object())
        {
            super_call = compilation_unit -> ast_pool -> GenSuperCall();
            super_call -> super_token = left_loc;
            super_call -> arguments = compilation_unit -> ast_pool ->
                GenArguments(left_loc, right_loc);

            // For enums, pass name and ordinal parameters to super (Enum) constructor
            if (type -> IsEnum())
            {
                super_call -> arguments -> AllocateArguments(2);

                // Get the parameter symbols from the constructor's block symbol
                // They were added earlier: name (index 1) and ordinal (index 2)
                // (index 0 is 'this')
                VariableSymbol* name_param = constructor -> block_symbol -> FindVariableSymbol(
                    control.FindOrInsertName(L"name", 4));
                VariableSymbol* ordinal_param = constructor -> block_symbol -> FindVariableSymbol(
                    control.FindOrInsertName(L"ordinal", 7));

                // Create name argument reference
                AstName* name_ref = compilation_unit -> ast_pool -> GenName(left_loc);
                if (name_param)
                    name_ref -> symbol = name_param;
                super_call -> arguments -> AddArgument(name_ref);

                // Create ordinal argument reference
                AstName* ordinal_ref = compilation_unit -> ast_pool -> GenName(left_loc);
                if (ordinal_param)
                    ordinal_ref -> symbol = ordinal_param;
                super_call -> arguments -> AddArgument(ordinal_ref);
            }

            super_call -> semicolon_token = right_loc;
        }

        AstReturnStatement* return_statement =
            compilation_unit -> ast_pool -> GenReturnStatement();
        return_statement -> return_token = left_loc;
        return_statement -> semicolon_token = left_loc;
        return_statement -> is_reachable = true;

        AstMethodBody* constructor_block =
            compilation_unit -> ast_pool -> GenMethodBody();
        // This symbol table will be empty.
        constructor_block -> block_symbol = new BlockSymbol(0);
        compilation_unit -> ast_pool -> RegisterBlockSymbol(constructor_block -> block_symbol);
        constructor_block -> left_brace_token  = left_loc;
        constructor_block -> right_brace_token = right_loc;
        constructor_block -> AllocateStatements(1);
        constructor_block -> AddStatement(return_statement);
        constructor_block -> explicit_constructor_opt = super_call;

        AstConstructorDeclaration* constructor_declaration =
            compilation_unit -> ast_pool -> GenConstructorDeclaration();
        constructor_declaration -> constructor_declarator = method_declarator;
        constructor_declaration -> constructor_body = constructor_block;

        constructor_declaration -> constructor_symbol = constructor;
        constructor -> declaration = constructor_declaration;
        class_body -> default_constructor = constructor_declaration;
    }
}


void Semantic::AddEnumSyntheticMethods(TypeSymbol* enum_type)
{
    assert(enum_type -> IsEnum());

    // Add synthetic values() method: public static EnumType[] values()
    NameSymbol* values_name = control.FindOrInsertName(L"values", 6);
    MethodSymbol* values_method = enum_type -> InsertMethodSymbol(values_name);

    // Create array type for return value: EnumType[]
    TypeSymbol* array_type = enum_type -> GetArrayType((Semantic*) this, 1);

    values_method -> SetType(array_type);
    values_method -> SetContainingType(enum_type);
    values_method -> SetACC_PUBLIC();
    values_method -> SetACC_STATIC();
    values_method -> SetACC_SYNTHETIC();
    values_method -> SetBlockSymbol(new BlockSymbol(0));
    values_method -> SetSignature(control);

    // Add synthetic valueOf(String) method: public static EnumType valueOf(String name)
    NameSymbol* valueOf_name = control.FindOrInsertName(L"valueOf", 7);
    MethodSymbol* valueOf_method = enum_type -> InsertMethodSymbol(valueOf_name);

    valueOf_method -> SetType(enum_type);
    valueOf_method -> SetContainingType(enum_type);
    valueOf_method -> SetACC_PUBLIC();
    valueOf_method -> SetACC_STATIC();
    valueOf_method -> SetACC_SYNTHETIC();

    // Create parameter: String name
    BlockSymbol* block_symbol = new BlockSymbol(1);
    NameSymbol* name_param_name = control.FindOrInsertName(L"name", 4);
    VariableSymbol* name_param = block_symbol -> InsertVariableSymbol(name_param_name);
    name_param -> SetType(control.String());
    name_param -> SetLocalVariableIndex(0);
    name_param -> MarkComplete();
    valueOf_method -> AddFormalParameter(name_param);

    valueOf_method -> SetBlockSymbol(block_symbol);
    valueOf_method -> SetSignature(control);
}


//
// This is called by AddInheritedMethods in two conditions: First, method
// belongs to base_type, so it must successfully override hidden_method,
// including a compatible throws clause. Second, method belongs to a supertype
// of base_type, in which case base_type is inheriting two declarations, one
// of which is necessarily abstract. If one is non-abstract, it must
// successfully override the abstract version, including the throws clause;
// but if both are abstract the throws clauses are inconsequential. It is
// possible that both method and hidden_method were declared in the same
// class, and inherited through two paths, in which case we do nothing.
// See JLS 8.4.6.4, and 9.4.1.
//
void Semantic::CheckMethodOverride(MethodSymbol* method,
                                   MethodSymbol* hidden_method,
                                   TypeSymbol* base_type)
{
    assert(! hidden_method -> ACC_PRIVATE());

    //
    // If we inherit the same method from multiple paths (including methods
    // of Object via interfaces), we already know the result.
    //
    if (hidden_method == method ||
        (method -> containing_type -> ACC_INTERFACE() &&
         method -> containing_type != base_type &&
         hidden_method -> containing_type == control.Object()))
    {
        return;
    }

    TokenIndex left_tok;
    TokenIndex right_tok;

    if (method -> containing_type == base_type && ThisType() == base_type)
    {
        AstMethodDeclaration* method_declaration =
            (AstMethodDeclaration*) method -> declaration;
        assert(method_declaration);
        AstMethodDeclarator* method_declarator =
            method_declaration -> method_declarator;

        left_tok = method_declarator -> LeftToken();
        right_tok = method_declarator -> RightToken();
    }
    else
    {
        left_tok = ThisType() -> declaration -> identifier_token;
        right_tok = ThisType() -> declaration -> left_brace_token - 1;
    }

    //
    // Return types must match.
    //
    if (hidden_method -> Type() != method -> Type())
    {
        //
        // We support covariant return types when loading from .class files,
        // even though this is not strictly legal in JLS2 (according to binary
        // compatibility, changing the return type need not be supported).
        // This is done in anticipation of JDK 1.5, when covariance is likely
        // to be introduced. The resultant effect is that the subclass must
        // conform to the narrower return type. Note that we currently only
        // support Object->Object covariance (and not primitive->primitive,
        // void->primitive, or void->Object). When loading from .java files,
        // however, we enforce exact return type matching.
        //
        if (hidden_method -> Type() -> IsSubtype(control.Object()) &&
            method -> Type() -> IsSubtype(hidden_method -> Type()))
        {
            // Silent acceptance for .class files only.
            // They must work, because the 1.5 library is covariant,
            // even for -source 1.4!
            if (control.option.source < JopaOption::SDK1_5 &&
                ! hidden_method -> containing_type ->
                  file_symbol -> IsClassOnly())
            {
                ReportSemError(SemanticError::COVARIANCE_UNSUPPORTED,
                               left_tok, right_tok, method -> Header(),
                               hidden_method -> Header());
            }
        }
        else if (method -> containing_type == base_type)
        {
            if (base_type -> ACC_INTERFACE() &&
                hidden_method -> containing_type == control.Object())
            {
                //
                // TODO: Review this when JLS3 is published.  See Sun bug
                // 4479715, which explains our current stance of allowing
                // int clone() throws MyException; or Object finalize();.
                //
                if (hidden_method -> ACC_PUBLIC())
                {
                    ReportSemError(SemanticError::MISMATCHED_IMPLICIT_METHOD,
                                   left_tok, right_tok, method -> Header(),
                                   hidden_method -> Header());
                    base_type -> MarkBad();
                }
                else
                {
                    ReportSemError(SemanticError::UNIMPLEMENTABLE_INTERFACE,
                                   left_tok, right_tok,
                                   base_type -> ContainingPackageName(),
                                   base_type -> ExternalName(),
                                   method -> Header(),
                                   hidden_method -> Header());
                }
            }
            else
            {
                ReportSemError(SemanticError::MISMATCHED_INHERITED_METHOD,
                               left_tok, right_tok, method -> Header(),
                               hidden_method -> Header(),
                               hidden_method -> containing_type -> ContainingPackageName(),
                               hidden_method -> containing_type -> ExternalName());
                base_type -> MarkBad();
            }
        }
        else
        {
            ReportSemError(SemanticError::MISMATCHED_INHERITED_METHOD_EXTERNALLY,
                           left_tok, right_tok, base_type -> ExternalName(),
                           method -> Header(),
                           method -> containing_type -> ContainingPackageName(),
                           method -> containing_type -> ExternalName(),
                           hidden_method -> Header(),
                           hidden_method -> containing_type -> ContainingPackageName(),
                           hidden_method -> containing_type -> ExternalName());
            base_type -> MarkBad();
        }
    }


    //
    // If base_type declared method, hidden_method must not be final. On the
    // other hand, if a type inherits a final method from a superclass and
    // an abstract method from an interface, it is legal.
    //
    if (method -> containing_type == base_type &&
        (hidden_method -> ACC_FINAL() ||
         hidden_method -> containing_type -> ACC_FINAL()))
    {
        if (base_type -> ACC_INTERFACE())
        {
            ReportSemError(SemanticError::FINAL_IMPLICIT_METHOD_OVERRIDE,
                           left_tok, right_tok, method -> Header(),
                           hidden_method -> Header());
        }
        else
        {
            ReportSemError(SemanticError::FINAL_METHOD_OVERRIDE,
                           left_tok, right_tok, method -> Header(),
                           hidden_method -> Header(),
                           hidden_method -> containing_type -> ContainingPackageName(),
                           hidden_method -> containing_type -> ExternalName());
        }
        base_type -> MarkBad();
    }

    //
    // Warn if a method we have source for is overriding a deprecated method.
    //
    if (control.option.deprecation &&
        hidden_method -> IsDeprecated() &&
        ! method -> containing_type -> file_symbol -> IsClassOnly())
    {
        ReportSemError(SemanticError::DEPRECATED_METHOD_OVERRIDE,
                       left_tok, right_tok, method -> Header(),
                       hidden_method -> containing_type -> ContainingPackageName(),
                       hidden_method -> containing_type -> ExternalName());
    }

    //
    // Both or neither versions must be static.
    //
    if (method -> ACC_STATIC() != hidden_method -> ACC_STATIC())
    {
        if (method -> containing_type == base_type)
        {
            ReportSemError((method -> ACC_STATIC()
                            ? SemanticError::INSTANCE_METHOD_OVERRIDE
                            : SemanticError::CLASS_METHOD_OVERRIDE),
                           left_tok, right_tok, method -> Header(),
                           hidden_method -> Header(),
                           hidden_method -> containing_type -> ContainingPackageName(),
                           hidden_method -> containing_type -> ExternalName());
        }
        else
        {
            assert(method -> ACC_STATIC());
            ReportSemError(SemanticError::INSTANCE_METHOD_OVERRIDE_EXTERNALLY,
                           left_tok, right_tok, base_type -> ExternalName(),
                           method -> Header(),
                           method -> containing_type -> ContainingPackageName(),
                           method -> containing_type -> ExternalName(),
                           hidden_method -> Header(),
                           hidden_method -> containing_type -> ContainingPackageName(),
                           hidden_method -> containing_type -> ExternalName());
        }
        base_type -> MarkBad();
    }

    //
    // An overriding method cannot be less accessible. On the other hand, it
    // is legal to inherit two abstract methods when one is not public.
    //
    if (hidden_method -> ACC_PUBLIC())
    {
        if (! method -> ACC_PUBLIC())
        {
            if (method -> containing_type == base_type)
            {
                ReportSemError(SemanticError::BAD_ACCESS_METHOD_OVERRIDE,
                               left_tok, right_tok, method -> Header(),
                               method -> AccessString(),
                               hidden_method -> Header(),
                               StringConstant::US_public,
                               hidden_method -> containing_type -> ContainingPackageName(),
                               hidden_method -> containing_type -> ExternalName());
                base_type -> MarkBad();
            }
            else if (! method -> ACC_ABSTRACT())
            {
                ReportSemError(SemanticError::BAD_ACCESS_METHOD_OVERRIDE_EXTERNALLY,
                               left_tok, right_tok,
                               base_type -> ExternalName(),
                               method -> Header(),
                               method -> AccessString(),
                               method -> containing_type -> ContainingPackageName(),
                               method -> containing_type -> ExternalName(),
                               hidden_method -> Header(),
                               StringConstant::US_public,
                               hidden_method -> containing_type -> ContainingPackageName(),
                               hidden_method -> containing_type -> ExternalName());
                base_type -> MarkBad();
            }
        }
    }
    else if (hidden_method -> ACC_PROTECTED())
    {
        if (! method -> ACC_PROTECTED() &&
            ! method -> ACC_PUBLIC())
        {
            ReportSemError(SemanticError::BAD_ACCESS_METHOD_OVERRIDE,
                           left_tok, right_tok, method -> Header(),
                           method -> AccessString(),
                           hidden_method -> Header(),
                           StringConstant::US_protected,
                           hidden_method -> containing_type -> ContainingPackageName(),
                           hidden_method -> containing_type -> ExternalName());
            base_type -> MarkBad();
        }
    }
    else if (method -> ACC_PRIVATE())
    {
        ReportSemError(SemanticError::BAD_ACCESS_METHOD_OVERRIDE,
                       left_tok, right_tok, method -> Header(),
                       StringConstant::US_private,
                       hidden_method -> Header(),
                       StringConstant::US_default,
                       hidden_method -> containing_type -> ContainingPackageName(),
                       hidden_method -> containing_type -> ExternalName());
        base_type -> MarkBad();
    }

    //
    // Check the throws clause, unless base_type is inheriting two abstract
    // methods.
    //
    if (method -> containing_type != base_type && method -> ACC_ABSTRACT())
        return;

    method -> ProcessMethodThrows(this, left_tok);
    hidden_method -> ProcessMethodThrows(this, left_tok);

    for (int i = static_cast<int>(method -> NumThrows()) - 1; i >= 0; i--)
    {
        TypeSymbol* exception = method -> Throws(i);

        if (! CheckedException(exception))
            continue;

        int k;
        for (k = static_cast<int>(hidden_method -> NumThrows()) - 1; k >= 0; k--)
        {
            if (exception -> IsSubclass(hidden_method -> Throws(k)))
                break;
        }

        if (k < 0)
        {
            if (method -> containing_type == base_type)
            {
                if (base_type -> ACC_INTERFACE() &&
                    hidden_method -> containing_type == control.Object())
                {
                    //
                    // TODO: Review this when JLS3 is published.  See Sun bug
                    // 4479715, which explains our current stance of allowing
                    // int clone() throws MyException; or Object finalize();.
                    //
                    if (hidden_method -> ACC_PUBLIC())
                    {
                        ReportSemError(SemanticError::MISMATCHED_IMPLICIT_OVERRIDDEN_EXCEPTION,
                                       left_tok, right_tok,
                                       exception -> Name(),
                                       method -> Header());
                        base_type -> MarkBad();
                    }
                }
                else
                {
                    ReportSemError(SemanticError::MISMATCHED_OVERRIDDEN_EXCEPTION,
                                   left_tok, right_tok,
                                   exception -> Name(),
                                   hidden_method -> Header(),
                                   hidden_method -> containing_type -> ContainingPackageName(),
                                   hidden_method -> containing_type -> ExternalName());
                    base_type -> MarkBad();
                }
            }
            else
            {
                ReportSemError(SemanticError::MISMATCHED_OVERRIDDEN_EXCEPTION_EXTERNALLY,
                               left_tok, right_tok,
                               base_type -> ExternalName(),
                               exception -> Name(), method -> Header(),
                               method -> containing_type -> ContainingPackageName(),
                               method -> containing_type -> ExternalName(),
                               hidden_method -> Header(),
                               hidden_method -> containing_type -> ContainingPackageName(),
                               hidden_method -> containing_type -> ExternalName());
                base_type -> MarkBad();
            }
        }
    }
}


void Semantic::AddInheritedTypes(TypeSymbol* base_type, TypeSymbol* super_type)
{
    if (super_type -> Bad())
    {
        base_type -> MarkBad();
        return;
    }

    ExpandedTypeTable& base_expanded_table =
        *(base_type -> expanded_type_table);
    ExpandedTypeTable& super_expanded_table =
        *(super_type -> expanded_type_table);

    for (unsigned i = 0; i < super_expanded_table.symbol_pool.Length(); i++)
    {
        TypeShadowSymbol* type_shadow_symbol =
            super_expanded_table.symbol_pool[i];
        TypeSymbol* type = type_shadow_symbol -> type_symbol;

        //
        // Note that since all types in an interface are implicitly public,
        // all other types encountered here are enclosed in a type that is a
        // super class of base_type.
        //
        if (type -> ACC_PUBLIC() ||
            type -> ACC_PROTECTED() ||
            (! type -> ACC_PRIVATE() &&
             super_type -> ContainingPackage() == base_type -> ContainingPackage()))
        {
            TypeShadowSymbol* shadow =
                base_expanded_table.FindTypeShadowSymbol(type -> Identity());

            if (! shadow || shadow -> type_symbol -> owner != base_type)
            {
                if (! shadow)
                    shadow = base_expanded_table.InsertTypeShadowSymbol(type);
                else shadow -> AddConflict(type);

                assert(type -> owner != super_type ||
                       type_shadow_symbol -> NumConflicts() == 0);
                for (unsigned j = 0;
                     j < type_shadow_symbol -> NumConflicts(); j++)
                {
                    shadow -> AddConflict(type_shadow_symbol -> Conflict(j));
                }
            }
        }
        //
        // The main type was not accessible. But it may have been inherited
        // from yet another class, in which case any conflicts (which are
        // necessarily public types from interfaces) are still inherited in
        // the base_type.
        //
        else if (! type -> ACC_PRIVATE() &&
                 type_shadow_symbol -> NumConflicts())
        {
            assert(type -> owner != super_type);
            TypeShadowSymbol* shadow =
                base_expanded_table.FindTypeShadowSymbol(type -> Identity());

            if (shadow)
                assert(shadow -> type_symbol -> owner == base_type);
            else
            {
                shadow = base_expanded_table.
                    InsertTypeShadowSymbol(type_shadow_symbol -> Conflict(0));
                for (unsigned k = 1;
                     k < type_shadow_symbol -> NumConflicts(); k++)
                {
                    shadow -> AddConflict(type_shadow_symbol -> Conflict(k));
                }
            }
        }
    }
}


void Semantic::AddInheritedFields(TypeSymbol* base_type,
                                  TypeSymbol* super_type)
{
    if (super_type -> Bad())
    {
        base_type -> MarkBad();
        return;
    }

    ExpandedFieldTable& base_expanded_table =
        *(base_type -> expanded_field_table);
    ExpandedFieldTable& super_expanded_table =
        *(super_type -> expanded_field_table);

    for (unsigned i = 0; i < super_expanded_table.symbol_pool.Length(); i++)
    {
        VariableShadowSymbol* variable_shadow_symbol =
            super_expanded_table.symbol_pool[i];
        VariableSymbol* variable = variable_shadow_symbol -> variable_symbol;
        //
        // Note that since all fields in an interface are implicitly public,
        // all other fields encountered here are enclosed in a type that is a
        // super class of base_type.
        //
        if (variable -> ACC_PUBLIC() ||
            variable -> ACC_PROTECTED() ||
            (! variable -> ACC_PRIVATE() &&
             super_type -> ContainingPackage() == base_type -> ContainingPackage()))
        {
            VariableShadowSymbol* shadow = base_expanded_table.
                FindVariableShadowSymbol(variable -> Identity());

            if (! shadow || shadow -> variable_symbol -> owner != base_type)
            {
                if (! shadow)
                    shadow = base_expanded_table.
                        InsertVariableShadowSymbol(variable);
                else shadow -> AddConflict(variable);

                assert(variable -> owner != super_type ||
                       variable_shadow_symbol -> NumConflicts() == 0);
                for (unsigned j = 0;
                     j < variable_shadow_symbol -> NumConflicts(); j++)
                {
                    shadow -> AddConflict(variable_shadow_symbol ->
                                          Conflict(j));
                }
            }
        }
        //
        // The main field was not accessible. But it may have been inherited
        // from yet another class, in which case any conflicts (which are
        // necessarily public fields from interfaces) are still inherited in
        // the base_type.
        //
        else if (! variable -> ACC_PRIVATE() &&
                 ! variable -> ACC_SYNTHETIC() &&
                 variable_shadow_symbol -> NumConflicts())
        {
            assert(variable -> owner != super_type);
            VariableShadowSymbol* shadow = base_expanded_table.
                FindVariableShadowSymbol(variable -> Identity());

            if (shadow)
                assert(shadow -> variable_symbol -> owner == base_type);
            else
            {
                shadow = base_expanded_table.
                    InsertVariableShadowSymbol(variable_shadow_symbol ->
                                               Conflict(0));

                for (unsigned k = 1;
                     k < variable_shadow_symbol -> NumConflicts(); k++)
                {
                    shadow -> AddConflict(variable_shadow_symbol ->
                                          Conflict(k));
                }
            }
        }
    }
}


void Semantic::AddInheritedMethods(TypeSymbol* base_type,
                                   TypeSymbol* super_type, TokenIndex tok)
{
    if (super_type -> Bad())
    {
        base_type -> MarkBad();
        return;
    }

    ExpandedMethodTable* base_expanded_table =
        base_type -> expanded_method_table;
    ExpandedMethodTable* super_expanded_table =
        super_type -> expanded_method_table;
    PackageSymbol* base_package = base_type -> ContainingPackage();
    unsigned i;

    for (i = 0; i < super_expanded_table -> symbol_pool.Length(); i++)
    {
        MethodShadowSymbol* method_shadow_symbol =
            super_expanded_table -> symbol_pool[i];
        MethodSymbol* method = method_shadow_symbol -> method_symbol;

        //
        // We have to special case interfaces, since they implicitly declare
        // the public methods of Object. In ComputeMethodsClosure, we add all
        // methods from Object after adding those from interfaces. Also, since
        // user code cannot invoke synthetic methods, we ignore those.
        //
        if ((base_type -> ACC_INTERFACE() &&
             super_type -> ACC_INTERFACE() &&
             method -> containing_type == control.Object()) ||
            method -> ACC_SYNTHETIC())
        {
            continue;
        }

        //
        // Note that since all methods in an interface are implicitly
        // public, all other methods encountered here are enclosed in a
        // type that is a super class of base_type.
        //
        if (method -> ACC_PUBLIC() || method -> ACC_PROTECTED() ||
            (! method -> ACC_PRIVATE() &&
             super_type -> ContainingPackage() == base_package))
        {
            //
            // Check that method is compatible with every method it
            // overrides.
            //
            MethodShadowSymbol* shadow = base_expanded_table ->
                FindOverloadMethodShadow(method, this, tok);
            if (shadow)
            {
                CheckMethodOverride(shadow -> method_symbol, method,
                                    base_type);
                for (unsigned m = 0;
                     m < method_shadow_symbol -> NumConflicts(); m++)
                {
                    CheckMethodOverride(shadow -> method_symbol,
                                        method_shadow_symbol -> Conflict(m),
                                        base_type);
                }
            }

            if (! shadow ||
                shadow -> method_symbol -> containing_type != base_type)
            {
                if (! shadow)
                    shadow = base_expanded_table -> Overload(method);
                else shadow -> AddConflict(method);

                assert(method -> containing_type != super_type ||
                       method_shadow_symbol -> NumConflicts() == 0);
                for (unsigned j = 0;
                     j < method_shadow_symbol -> NumConflicts(); j++)
                {
                    shadow -> AddConflict(method_shadow_symbol -> Conflict(j));
                }
            }
        }
        //
        // The main method was not accessible. But it may have been inherited
        // from yet another class, in which case any conflicts (which are
        // necessarily public methods from interfaces) are still inherited in
        // the base_type.
        //
        else if (! method -> ACC_PRIVATE())
        {
            MethodShadowSymbol* shadow = base_expanded_table ->
                FindOverloadMethodShadow(method, this, tok);
            if (method_shadow_symbol -> NumConflicts())
            {
                assert(method -> containing_type != super_type);

                if (shadow)
                {
                    assert(shadow -> method_symbol -> containing_type == base_type);
                    for (unsigned k = 0;
                         k < method_shadow_symbol -> NumConflicts(); k++)
                    {
                        CheckMethodOverride(shadow -> method_symbol,
                                            method_shadow_symbol -> Conflict(k),
                                            base_type);
                    }
                }
                else
                {
                    shadow = base_expanded_table ->
                        Overload(method_shadow_symbol -> Conflict(0));

                    for (unsigned l = 1;
                         l < method_shadow_symbol -> NumConflicts(); l++)
                    {
                        shadow -> AddConflict(method_shadow_symbol ->
                                              Conflict(l));
                    }
                }
            }
            else if (shadow && control.option.pedantic)
            {
                //
                // The base_type declares a method by the same name as a
                // method in the superclass, but the new method does not
                // override or hide the old. Warn the user about this fact,
                // although it is usually not an error.
                //
                assert(shadow -> method_symbol -> containing_type ==
                       base_type);
                TokenIndex left_tok;
                TokenIndex right_tok;

                if (ThisType() == base_type)
                {
                    AstMethodDeclaration* method_declaration =
                        (AstMethodDeclaration*) shadow -> method_symbol -> declaration;
                    AstMethodDeclarator* method_declarator =
                        method_declaration -> method_declarator;

                    left_tok = method_declarator -> LeftToken();
                    right_tok = method_declarator -> RightToken();
                }
                else
                {
                    left_tok = ThisType() -> declaration -> identifier_token;
                    right_tok =
                        ThisType() -> declaration -> right_brace_token - 1;
                }

                if (! method -> IsTyped())
                    method -> ProcessMethodSignature(this, tok);

                //
                // We filter here, because CompleteSymbolTable gives a
                // different warning for unimplementable abstract classes.
                //
                if (! method -> ACC_ABSTRACT() ||
                    method -> Type() == shadow -> method_symbol -> Type() ||
                    (! shadow -> method_symbol -> ACC_PUBLIC() &&
                     ! shadow -> method_symbol -> ACC_PROTECTED()))
                {
                    ReportSemError(SemanticError::DEFAULT_METHOD_NOT_OVERRIDDEN,
                                   left_tok, right_tok, method -> Header(),
                                   base_type -> ContainingPackageName(),
                                   base_type -> ExternalName(),
                                   super_type -> ContainingPackageName(),
                                   super_type -> ExternalName());
                }
            }
        }
    }
    //
    // Now, we must ensure that any time the inheritance tree left and
    // reentered the package, the non-inherited default methods were
    // correctly overridden or hidden if redeclared in this class. A method
    // is non-inherited only if a class C is in the package, it's subclass
    // is not, and there is no interface method also inherited into C.
    //
    while (super_type -> super)
    {
        TypeSymbol* prev = super_type;
        super_type = super_type -> super;
        if (prev -> ContainingPackage() == base_package ||
            super_type -> ContainingPackage() != base_package)
        {
            continue;
        }
        super_expanded_table = super_type -> expanded_method_table;
        for (i = 0; i < super_expanded_table -> symbol_pool.Length(); i++)
        {
            MethodShadowSymbol* method_shadow_symbol =
                super_expanded_table -> symbol_pool[i];
            MethodSymbol* method = method_shadow_symbol -> method_symbol;
            if (! method -> ACC_PUBLIC() && ! method -> ACC_PROTECTED() &&
                ! method -> ACC_PRIVATE() && ! method -> ACC_SYNTHETIC() &&
                method_shadow_symbol -> NumConflicts() == 0)
            {
                // found a non-inherited package scope method
                MethodShadowSymbol* shadow = base_expanded_table ->
                    FindOverloadMethodShadow(method, this, tok);
                if (shadow &&
                    shadow -> method_symbol -> containing_type == base_type)
                {
                    CheckMethodOverride(shadow -> method_symbol, method,
                                        base_type);
                }
            }
        }
    }
}


void Semantic::ComputeTypesClosure(TypeSymbol* type, TokenIndex tok)
{
    if (! type -> HeaderProcessed())
        type -> ProcessTypeHeaders();
    type -> expanded_type_table = new ExpandedTypeTable();

    TypeSymbol* super_class = type -> super;
    if (super_class)
    {
        if (! super_class -> expanded_type_table)
            ComputeTypesClosure(super_class, tok);
    }

    for (unsigned j = 0; j < type -> NumInterfaces(); j++)
    {
        TypeSymbol* interf = type -> Interface(j);
        if (! interf -> expanded_type_table)
            ComputeTypesClosure(interf, tok);
    }

    if (! type -> NestedTypesProcessed())
        type -> ProcessNestedTypeSignatures(this, tok);
    for (unsigned i = 0; i < type -> NumTypeSymbols(); i++)
    {
        if (! type -> TypeSym(i) -> Bad())
        {
            type -> expanded_type_table ->
                InsertTypeShadowSymbol(type -> TypeSym(i));
        }
    }
    if (super_class)
        AddInheritedTypes(type, super_class);
    for (unsigned k = 0; k < type -> NumInterfaces(); k++)
        AddInheritedTypes(type, type -> Interface(k));
    type -> expanded_type_table -> CompressSpace();
}


void Semantic::ComputeFieldsClosure(TypeSymbol* type, TokenIndex tok)
{
    type -> expanded_field_table = new ExpandedFieldTable();

    TypeSymbol* super_class = type -> super;
    if (super_class)
    {
        if (! super_class -> expanded_field_table)
            ComputeFieldsClosure(super_class, tok);
    }

    for (unsigned j = 0; j < type -> NumInterfaces(); j++)
    {
        TypeSymbol* interf = type -> Interface(j);
        if (! interf -> expanded_field_table)
            ComputeFieldsClosure(interf, tok);
    }

    assert(type -> FieldMembersProcessed());

    for (unsigned i = 0; i < type -> NumVariableSymbols(); i++)
    {
        VariableSymbol* variable = type -> VariableSym(i);
        type -> expanded_field_table -> InsertVariableShadowSymbol(variable);
    }

    //
    // As the type Object which is the super type of all interfaces does
    // not contain any field declarations, we don't have to do any special
    // check here as we have to when computing method closures.
    //
    if (super_class)
        AddInheritedFields(type, super_class);
    for (unsigned k = 0; k < type -> NumInterfaces(); k++)
        AddInheritedFields(type, type -> Interface(k));
    type -> expanded_field_table -> CompressSpace();
}


void Semantic::ComputeMethodsClosure(TypeSymbol* type, TokenIndex tok)
{
    type -> expanded_method_table = new ExpandedMethodTable();

    TypeSymbol* super_class = type -> super;
    if (super_class)
    {
        if (! super_class -> expanded_method_table)
            ComputeMethodsClosure(super_class, tok);
    }

    for (unsigned j = 0; j < type -> NumInterfaces(); j++)
    {
        TypeSymbol* interf = type -> Interface(j);

        if (! interf -> expanded_method_table)
            ComputeMethodsClosure(interf, tok);
    }

    assert(type -> MethodMembersProcessed());

    for (unsigned i = 0; i < type -> NumMethodSymbols(); i++)
    {
        MethodSymbol* method = type -> MethodSym(i);
        //
        // If the method in question is neither a constructor nor an
        // initializer, then ...
        //
        if (*(method -> Name()) != U_LESS)
        {
            type -> expanded_method_table -> Overload(method);
        }
    }

    //
    // We build in this order to guarantee that the first method listed in
    // the table will be declared in a class.  Conflicts, if any, are from
    // interfaces and are necessarily abstract; but if the first method
    // is not abstract, it implements all the conflicts.
    //
    if (super_class && ! type -> ACC_INTERFACE())
        AddInheritedMethods(type, super_class, tok);
    for (unsigned k = 0; k < type -> NumInterfaces(); k++)
        AddInheritedMethods(type, type -> Interface(k), tok);
    if (type -> ACC_INTERFACE()) // the super class is Object
        AddInheritedMethods(type, control.Object(), tok);
    type -> expanded_method_table -> CompressSpace();
}


void Semantic::ProcessFormalParameters(BlockSymbol* block,
                                       AstMethodDeclarator* method_declarator)
{
    for (unsigned i = 0; i < method_declarator -> NumFormalParameters(); i++)
    {
        AstFormalParameter* parameter =
            method_declarator -> FormalParameter(i);
        AccessFlags access_flags = ProcessFormalModifiers(parameter);
        ProcessType(parameter -> type);
        TypeSymbol* parm_type = parameter -> type -> symbol;

        AstVariableDeclaratorId* name =
            parameter -> formal_declarator -> variable_declarator_name;
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(name -> identifier_token);
        VariableSymbol* symbol = block -> FindVariableSymbol(name_symbol);
        if (symbol)
        {
            ReportSemError(SemanticError::DUPLICATE_FORMAL_PARAMETER,
                           name -> identifier_token, name_symbol -> Name());
        }
        else symbol = block -> InsertVariableSymbol(name_symbol);

        unsigned dims = parm_type -> num_dimensions + name -> NumBrackets();
        if (parameter -> ellipsis_token_opt)
        {
            assert(i == method_declarator -> NumFormalParameters() - 1);
            dims++;
            access_flags.SetACC_VARARGS();
            if (control.option.source < JopaOption::SDK1_5)
            {
                ReportSemError(SemanticError::VARARGS_UNSUPPORTED,
                               parameter -> ellipsis_token_opt);
            }
        }
        symbol -> SetType(parm_type -> GetArrayType(this, dims));
        symbol -> SetFlags(access_flags);
        symbol -> MarkComplete();
        symbol -> MarkInitialized();

        // Copy parameterized type information if present
        AstTypeName* type_name = parameter -> type -> TypeNameCast();
        if (type_name && type_name -> parameterized_type)
        {
            symbol -> parameterized_type = type_name -> parameterized_type;
        }

        parameter -> formal_declarator -> symbol = symbol;
    }
}


void Semantic::ProcessMethodDeclaration(AstMethodDeclaration* method_declaration)
{
    TypeSymbol* this_type = ThisType();
    AccessFlags access_flags = this_type -> ACC_INTERFACE()
        ? ProcessInterfaceMethodModifiers(method_declaration)
        : ProcessMethodModifiers(method_declaration);
    //
    // By JLS2 8.4.3.3, a private method and all methods declared in a
    // final class are implicitly final. Also, all methods in a strictfp
    // class are strictfp.
    //
    if (access_flags.ACC_PRIVATE() || this_type -> ACC_FINAL())
        access_flags.SetACC_FINAL();
    if (this_type -> ACC_STRICTFP())
        access_flags.SetACC_STRICTFP();

    //
    // A method enclosed in an inner type may not be declared static.
    //
    if (access_flags.ACC_STATIC() && this_type -> IsInner())
    {
        assert(method_declaration -> modifiers_opt &&
               method_declaration -> modifiers_opt -> static_token_opt);
        ReportSemError(SemanticError::STATIC_METHOD_IN_INNER_CLASS,
                       method_declaration -> modifiers_opt -> static_token_opt,
                       lex_stream -> NameString(method_declaration -> method_declarator -> identifier_token),
                       this_type -> Name(),
                       this_type -> FileLoc());
    }

    //
    // For generic methods, we need to process type parameters BEFORE processing
    // the return type, because the return type might reference the type parameters.
    // We create a temporary method symbol just for this purpose.
    //
    AstMethodDeclarator* method_declarator =
        method_declaration -> method_declarator;
    NameSymbol* name_symbol =
        lex_stream -> NameSymbol(method_declarator -> identifier_token);
    MethodSymbol* temp_method = new MethodSymbol(name_symbol);
    temp_method -> SetContainingType(this_type);

    // Temporarily set ThisMethod so type parameter lookup works
    MethodSymbol* saved_method = ThisMethod();
    ThisMethod() = temp_method;

    if (method_declaration -> type_parameters_opt)
    {
        ProcessMethodTypeParameters(temp_method, method_declaration -> type_parameters_opt);
    }

    //
    // To avoid deprecated type warnings when processing a deprecated method
    // declaration, we must temporarily mark this type as deprecated, because
    // the method symbol does not yet exist. We fix it after formal parameter
    // processing.
    //
    bool deprecated_method =
        lex_stream -> IsDeprecated(method_declaration -> LeftToken());
    bool deprecated_type = this_type -> IsDeprecated();
    if (deprecated_method)
        this_type -> MarkDeprecated();
    ProcessType(method_declaration -> type);
    TypeSymbol* method_type = method_declaration -> type -> symbol;

    if (method_declarator -> NumBrackets())
    {
        if (method_type == control.void_type)
        {
            ReportSemError(SemanticError::VOID_ARRAY,
                           method_declaration -> type -> LeftToken(),
                           method_declarator -> RightToken());
            method_type = control.no_type;
        }
        else ReportSemError(SemanticError::OBSOLESCENT_BRACKETS,
                            method_declarator);
    }

    if (name_symbol == this_type -> Identity())
    {
        ReportSemError(SemanticError::METHOD_WITH_CONSTRUCTOR_NAME,
                       method_declaration -> type -> LeftToken(),
                       method_declarator -> identifier_token,
                       name_symbol -> Name());
    }

    //
    // Warn against unconventional names.
    //
    if (name_symbol -> IsBadStyleForMethod())
    {
        ReportSemError(SemanticError::UNCONVENTIONAL_METHOD_NAME,
                       method_declarator -> identifier_token,
                       name_symbol -> Name());
    }

    //
    // As the body of the method may not have been parsed yet, we estimate a
    // size for its symbol table based on the number of lines in the body + a
    // margin for one-liners.
    //
    BlockSymbol* block_symbol =
        new BlockSymbol(method_declarator -> NumFormalParameters());
    block_symbol -> max_variable_index = (access_flags.ACC_STATIC() ? 0 : 1);
    ProcessFormalParameters(block_symbol, method_declarator);

    // Restore previous method context after processing return type and parameters
    ThisMethod() = saved_method;

    if (! deprecated_type && deprecated_method)
        this_type -> ResetDeprecated();

    MethodSymbol* method = this_type -> FindMethodSymbol(name_symbol);
    if (method && this_type -> FindOverloadMethod(method, method_declarator))
    {
        ReportSemError(SemanticError::DUPLICATE_METHOD,
                       method_declarator, name_symbol -> Name(),
                       this_type -> Name(), method -> FileLoc());
        delete block_symbol;
        return;
    }

    method = this_type -> InsertMethodSymbol(name_symbol);
    unsigned dims =
        method_type -> num_dimensions + method_declarator -> NumBrackets();
    method -> SetType(method_type -> GetArrayType(this, dims));

    //
    // if the method is not static, leave a slot for the "this" pointer.
    //
    method -> SetFlags(access_flags);
    method -> SetContainingType(this_type);
    method -> SetBlockSymbol(block_symbol);
    method -> declaration = method_declaration;
    method -> SetLocation();
    for (unsigned i = 0; i < method_declarator -> NumFormalParameters(); i++)
    {
        AstVariableDeclarator* formal_declarator =
            method_declarator -> FormalParameter(i) -> formal_declarator;
        VariableSymbol* symbol = formal_declarator -> symbol;

        symbol -> SetOwner(method);
        symbol -> SetLocalVariableIndex(block_symbol -> max_variable_index++);
        symbol -> MarkComplete();
        if (control.IsDoubleWordType(symbol -> Type()))
            block_symbol -> max_variable_index++;
        symbol -> declarator = formal_declarator;
        symbol -> SetLocation();
        method -> AddFormalParameter(symbol);
    }
    // Transfer type parameters from temp_method to real method
    for (unsigned i = 0; i < temp_method -> NumTypeParameters(); i++)
    {
        TypeParameterSymbol* type_param = temp_method -> TypeParameter(i);
        type_param -> owner = method;  // Update owner from temp to real method
        method -> AddTypeParameter(type_param);
    }
    // Detach type parameters and delete temp_method (type params now owned by real method)
    temp_method -> DetachTypeParameters();
    delete temp_method;

    // Propagate varargs flag from last parameter to method
    if (method -> NumFormalParameters() > 0)
    {
        VariableSymbol* last_param =
            method -> FormalParameter(method -> NumFormalParameters() - 1);
        if (last_param -> ACC_VARARGS())
        {
            method -> SetACC_VARARGS();
        }
    }

    method -> SetSignature(control);

    for (unsigned k = 0; k < method_declaration -> NumThrows(); k++)
    {
        AstTypeName* throw_expr = method_declaration -> Throw(k);
        ProcessType(throw_expr);
        method -> AddThrows(throw_expr -> symbol);
    }

    // save for processing bodies later.
    method_declaration -> method_symbol = method;

    if (method -> ACC_ABSTRACT() && ! this_type -> ACC_ABSTRACT())
    {
        ReportSemError(SemanticError::NON_ABSTRACT_TYPE_CONTAINS_ABSTRACT_METHOD,
                       method_declaration -> LeftToken(),
                       method_declarator -> identifier_token,
                       name_symbol -> Name(),
                       this_type -> Name());
    }

    if (deprecated_method)
        method -> MarkDeprecated();
}


//
// Return the type corresponding to a primitive type keyword.
//
TypeSymbol* Semantic::FindPrimitiveType(AstPrimitiveType* primitive_type)
{
    switch (primitive_type -> kind)
    {
    case Ast::INT:
        return control.int_type;
    case Ast::DOUBLE:
        return control.double_type;
    case Ast::CHAR:
        return control.char_type;
    case Ast::LONG:
        return control.long_type;
    case Ast::FLOAT:
        return control.float_type;
    case Ast::BYTE:
        return control.byte_type;
    case Ast::SHORT:
        return control.short_type;
    case Ast::BOOLEAN:
        return control.boolean_type;
    default:
        break;
    }

    return control.void_type;
}


//
// Return the wrapper type for a primitive type, or NULL if not primitive.
// Used for autoboxing conversions (JSR 201).
//
TypeSymbol* Semantic::GetWrapperType(TypeSymbol* type)
{
    if (!type || !type -> Primitive())
        return NULL;

    if (type == control.int_type)
        return control.Integer();
    if (type == control.boolean_type)
        return control.Boolean();
    if (type == control.byte_type)
        return control.Byte();
    if (type == control.char_type)
        return control.Character();
    if (type == control.short_type)
        return control.Short();
    if (type == control.long_type)
        return control.Long();
    if (type == control.float_type)
        return control.Float();
    if (type == control.double_type)
        return control.Double();

    return NULL;
}


//
// Return the primitive type for a wrapper type, or NULL if not a wrapper.
// Used for unboxing conversions (JSR 201).
//
TypeSymbol* Semantic::GetPrimitiveType(TypeSymbol* type)
{
    if (!type)
        return NULL;

    if (type == control.Integer())
        return control.int_type;
    if (type == control.Boolean())
        return control.boolean_type;
    if (type == control.Byte())
        return control.byte_type;
    if (type == control.Character())
        return control.char_type;
    if (type == control.Short())
        return control.short_type;
    if (type == control.Long())
        return control.long_type;
    if (type == control.Float())
        return control.float_type;
    if (type == control.Double())
        return control.double_type;

    return NULL;
}


//
// Check if a boxing conversion exists from source to target type.
// Boxing: primitive → wrapper (e.g., int → Integer)
//
bool Semantic::IsBoxingConversion(TypeSymbol* source, TypeSymbol* target)
{
    if (!source || !target)
        return false;

    return source -> Primitive() && GetWrapperType(source) == target;
}


//
// Check if an unboxing conversion exists from source to target type.
// Unboxing: wrapper → primitive (e.g., Integer → int)
//
bool Semantic::IsUnboxingConversion(TypeSymbol* source, TypeSymbol* target)
{
    if (!source || !target)
        return false;

    return target -> Primitive() && GetPrimitiveType(source) == target;
}


//
// Get the valueOf(primitive) method for boxing a primitive type.
// Returns the static valueOf method from the wrapper class.
//
MethodSymbol* Semantic::GetBoxingMethod(TypeSymbol* primitive_type)
{
    TypeSymbol* wrapper_type = GetWrapperType(primitive_type);
    if (!wrapper_type)
        return NULL;

    // Find valueOf(primitive) method
    NameSymbol* valueOf_name = control.FindOrInsertName(L"valueOf", 7);

    for (unsigned i = 0; i < wrapper_type -> NumMethodSymbols(); i++)
    {
        MethodSymbol* method = wrapper_type -> MethodSym(i);
        if (!method -> IsTyped())
        {
            // Process the method signature if not already done
            method -> ProcessMethodSignature((Semantic*)this, 0);
        }
        if (!method -> IsTyped())
            continue;  // Still not typed, skip it
        if (method -> Identity() == valueOf_name &&
            method -> ACC_STATIC() &&
            method -> NumFormalParameters() == 1 &&
            method -> FormalParameter(0) -> Type() == primitive_type)
        {
            return method;
        }
    }

    return NULL;
}


//
// Get the primitiveValue() method for unboxing a wrapper type.
// Returns the instance method that extracts the primitive value.
//
MethodSymbol* Semantic::GetUnboxingMethod(TypeSymbol* wrapper_type)
{
    TypeSymbol* primitive_type = GetPrimitiveType(wrapper_type);
    if (!primitive_type)
        return NULL;

    // Determine the method name (e.g., intValue, booleanValue)
    const wchar_t* method_name = NULL;
    if (primitive_type == control.int_type)
        method_name = L"intValue";
    else if (primitive_type == control.boolean_type)
        method_name = L"booleanValue";
    else if (primitive_type == control.byte_type)
        method_name = L"byteValue";
    else if (primitive_type == control.char_type)
        method_name = L"charValue";
    else if (primitive_type == control.short_type)
        method_name = L"shortValue";
    else if (primitive_type == control.long_type)
        method_name = L"longValue";
    else if (primitive_type == control.float_type)
        method_name = L"floatValue";
    else if (primitive_type == control.double_type)
        method_name = L"doubleValue";
    else
        return NULL;

    NameSymbol* unbox_name = control.FindOrInsertName(method_name, wcslen(method_name));

    for (unsigned i = 0; i < wrapper_type -> NumMethodSymbols(); i++)
    {
        MethodSymbol* method = wrapper_type -> MethodSym(i);
        if (!method -> IsTyped())
        {
            // Process the method signature if not already done
            method -> ProcessMethodSignature((Semantic*)this, 0);
        }
        if (!method -> IsTyped())
            continue;  // Still not typed, skip it
        if (method -> Identity() == unbox_name &&
            !method -> ACC_STATIC() &&
            method -> NumFormalParameters() == 0 &&
            method -> Type() == primitive_type)
        {
            return method;
        }
    }

    return NULL;
}


//
// Search the import-on-demand locations for a type with the given name. This
// returns inaccessible types if found, with no error message, but favors
// accessible ones. It will issue an error if the only way an accessible type
// was found is non-canonical. If no type is found, NULL is returned.
//
TypeSymbol* Semantic::ImportType(TokenIndex identifier_token,
                                 NameSymbol* name_symbol)
{
    //
    // To keep track of inaccessible types, we note the first one we find,
    // while leaving the location as NULL. Once we find an accessible type, we
    // set location, so that we know that future types are duplicates. We
    // pre-filtered duplicate import-on-demands, as well as adding java.lang.*.
    //
    TypeSymbol* type = NULL;
    PackageSymbol* location = NULL;

    for (unsigned i = 0; i < import_on_demand_packages.Length(); i++)
    {
        PackageSymbol* import_package =
            import_on_demand_packages[i] -> PackageCast();
        TypeSymbol* possible_type = NULL;

        if (import_package)
        {
            possible_type = import_package -> FindTypeSymbol(name_symbol);
            if (! possible_type)
            {
                FileSymbol* file_symbol =
                    Control::GetFile(control, import_package, name_symbol);
                if (file_symbol)
                    possible_type = ReadType(file_symbol, import_package,
                                             name_symbol, identifier_token);
            }
            else if (possible_type -> SourcePending())
                control.ProcessHeaders(possible_type -> file_symbol);
        }
        else
        {
            TypeSymbol* import_type =
                (TypeSymbol*) import_on_demand_packages[i];
            if (! import_type -> expanded_type_table)
                ComputeTypesClosure(import_type, identifier_token);
            TypeShadowSymbol* type_shadow_symbol = import_type ->
                expanded_type_table -> FindTypeShadowSymbol(name_symbol);
            if (type_shadow_symbol)
            {
                //
                // Only canonical names may be used in import statements, hence
                // the extra filter on the containing type being correct. If
                // we encounter conflicts, they are necessarily accessible
                // inherited types from interfaces (and hence non-canonical).
                //
                possible_type = (type_shadow_symbol -> NumConflicts()
                                 ? type_shadow_symbol -> Conflict(0)
                                 : type_shadow_symbol -> type_symbol);
                if (! possible_type -> ACC_PRIVATE() &&
                    import_type == possible_type -> owner)
                {
                    import_package = import_type -> ContainingPackage();
                }
            }
        }
        if (possible_type)
        {
            if (location && import_package &&
                (possible_type -> ACC_PUBLIC() ||
                 import_package == this_package))
            {
                ReportSemError(SemanticError::DUPLICATE_ON_DEMAND_IMPORT,
                               identifier_token, name_symbol -> Name(),
                               location -> PackageName(),
                               import_package -> PackageName());
            }
            else
            {
                type = possible_type;
                if (type -> ACC_PUBLIC() || import_package == this_package)
                    location = import_package; // may be NULL
            }
        }
    }

    if (type && ! location && ! type -> ACC_PRIVATE() &&
        (type -> ACC_PUBLIC() || type -> ContainingPackage() == this_package))
    {
        ReportSemError(SemanticError::IMPORT_NOT_CANONICAL,
                       identifier_token, type -> Name(),
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }

    // Keep track of referenced types.
    if (type && location)
        referenced_package_imports.AddElement(location);

    return type;
}


//
// Finds an accessible type by the name located at identifier_token, or returns
// NULL. If there are ambiguous accessible types, this issues an error in the
// process. Note that inaccessible types are skipped - if the caller wishes
// to use an inaccessible type, they must search for it.
//
TypeSymbol* Semantic::FindType(TokenIndex identifier_token)
{
    TypeSymbol* type;
    NameSymbol* name_symbol = lex_stream -> NameSymbol(identifier_token);

    SemanticEnvironment* env = NULL;
    if (state_stack.Size())
        env = state_stack.Top();

    // First check method type parameters if we're in a method context
    MethodSymbol* current_method = ThisMethod();
    if (current_method)
    {
        for (unsigned i = 0; i < current_method -> NumTypeParameters(); i++)
        {
            TypeParameterSymbol* type_param = current_method -> TypeParameter(i);
            if (type_param -> name_symbol == name_symbol)
            {
                // Found a method type parameter - return its erased type
                TypeSymbol* erased = type_param -> ErasedType();
                // If unbounded, default to Object
                return erased ? erased : control.Object();
            }
        }
    }

    for ( ; env; env = env -> previous)
    {
        // Check if this name matches a type parameter of the enclosing type
        TypeSymbol* enclosing_type = env -> Type();
        for (unsigned i = 0; i < enclosing_type -> NumTypeParameters(); i++)
        {
            TypeParameterSymbol* type_param = enclosing_type -> TypeParameter(i);
            if (type_param -> name_symbol == name_symbol)
            {
                // Found a type parameter - return its erased type
                // TODO: We should return a Type wrapper with TYPE_PARAMETER kind
                // For now, just return the erased type to keep things working
                TypeSymbol* erased = type_param -> ErasedType();
                // If unbounded, default to Object
                return erased ? erased : control.Object();
            }
        }

        // Search for local types, which are always accessible.
        type = env -> symbol_table.FindTypeSymbol(name_symbol);
        if (type)
            break;
        // Search for declared or inherited member types.
        type = env -> Type();
        if (! type -> expanded_type_table)
            ComputeTypesClosure(type, identifier_token);
        TypeShadowSymbol* type_shadow_symbol =
            type -> expanded_type_table -> FindTypeShadowSymbol(name_symbol);
        if (type_shadow_symbol)
        {
            type = FindTypeInShadow(type_shadow_symbol, identifier_token);
            if (type)
                break;
        }
    }

    if (env) // The type was found in some enclosing environment?
    {
        //
        // A static type cannot access a non-static member type of an enclosing
        // class by simple name.
        //
        TypeSymbol* this_type = ThisType();
        assert(this_type);
        if (this_type -> ACC_STATIC() && ! type -> ACC_STATIC() &&
            ! this_type -> IsSubclass(type -> ContainingType()))
        {
            ReportSemError(SemanticError::STATIC_TYPE_ACCESSING_MEMBER_TYPE,
                           identifier_token,
                           this_type -> ContainingPackageName(),
                           this_type -> ExternalName(),
                           type -> ContainingPackageName(),
                           type -> ExternalName(),
                           env -> Type() -> ContainingPackageName(),
                           env -> Type() -> ExternalName());
        }
        //
        // If the type was inherited, give a warning if it shadowed another
        // type of the same name within an enclosing lexical scope.
        //
        if (type -> owner -> TypeCast() && type -> owner != env -> Type())
        {
            TypeSymbol* supertype = (TypeSymbol*) type -> owner;
            for ( ; env; env = env -> previous)
            {
                //
                // First, check the enclosing type name - this is a caution,
                // because this behavior is opposite C++ when a type inherits
                // a membertype with the same name.
                //
                if (name_symbol == env -> Type() -> Identity() &&
                    env -> Type() != type)
                {
                    ReportSemError(SemanticError::INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_TYPE,
                                   identifier_token,
                                   lex_stream -> NameString(identifier_token),
                                   type -> ContainingPackageName(),
                                   type -> ExternalName(),
                                   env -> Type() -> ContainingPackageName(),
                                   env -> Type() -> ExternalName());
                    break;
                }
                if (env -> previous && control.option.pedantic)
                {
                    // Next, in pedantic mode, check local type
                    SemanticEnvironment* env2 = env -> previous;
                    TypeSymbol* outer_type =
                        env2 -> symbol_table.FindTypeSymbol(name_symbol);
                    if (outer_type)
                    {
                        assert(outer_type -> owner -> MethodCast());
                        ReportSemError(SemanticError::INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_LOCAL,
                                       identifier_token,
                                       lex_stream -> NameString(identifier_token),
                                       supertype -> ContainingPackageName(),
                                       supertype -> ExternalName(),
                                       ((MethodSymbol*) outer_type -> owner) -> Name());
                        break;
                    }
                    // If local type not found, check inner type.
                    if (! env2 -> Type() -> expanded_type_table)
                        ComputeTypesClosure(env2 -> Type(), identifier_token);
                    TypeShadowSymbol* type_shadow_symbol =
                        env2 -> Type() -> expanded_type_table ->
                        FindTypeShadowSymbol(name_symbol);
                    if (type_shadow_symbol)
                        outer_type = FindTypeInShadow(type_shadow_symbol,
                                                      identifier_token);
                    if (outer_type && outer_type != type &&
                        outer_type -> owner == env2 -> Type())
                    {
                        ReportSemError(SemanticError::INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_MEMBER,
                                       identifier_token,
                                       lex_stream -> NameString(identifier_token),
                                       supertype -> ContainingPackageName(),
                                       supertype -> ExternalName(),
                                       env2 -> Type() -> ContainingPackageName(),
                                       env2 -> Type() -> ExternalName());
                        break;
                    }
                }
            }
        }

        return type;
    }

    //
    // Search for the type in the current compilation unit if it was declared
    // as a class or interface or imported by a single-type-import declaration.
    //
    for (unsigned i = 0; i < single_type_imports.Length(); i++)
    {
        type = single_type_imports[i];
        if (name_symbol == type -> Identity())
        {
            // Keep track of referenced types.
            referenced_type_imports.AddElement(type);
            return type;
        }
    }

    //
    // Search for another file in the current package, and if that fails, check
    // for an accessible import-on-demand.
    //
    type = FindSimpleNameType(this_package, identifier_token);
    TypeSymbol* imported_type = (! type || type -> Bad()
                                 ? ImportType(identifier_token, name_symbol)
                                 : (TypeSymbol*) NULL);

    //
    // If a valid type can be imported on demand, choose that type.
    // Otherwise, if a type was found at all, do some final checks on it.
    //
    // Note that a type T contained in a package P is always accessible to all
    // other types contained in P. I.e., we do not need to perform access check
    // for type...
    //
    if (imported_type && TypeAccessCheck(imported_type))
        type = imported_type;
    else if (type)
    {
        //
        // If a type T was specified in a source file that is not called T.java
        // but X.java (where X != T) and we are not currently compiling file X,
        // issue a warning to alert the user that in some circumstances, this
        // may not be visible. (i.e., if the file X has not yet been compiled,
        // then T is invisile as the compiler will only look for T in T.java.)
        //
        FileSymbol* file_symbol = type -> file_symbol;
        if (file_symbol && type -> Identity() != file_symbol -> Identity() &&
            file_symbol != this -> source_file_symbol)
        {
            ReportSemError(SemanticError::REFERENCE_TO_TYPE_IN_MISMATCHED_FILE,
                           identifier_token, type -> Name(),
                           file_symbol -> Name());
        }
    }

    return type;
}


//
// Returns an inaccessible type of the given name, or 0 if there is none.
// No errors are reported by this method.
//
TypeSymbol* Semantic::FindInaccessibleType(AstName* name)
{
    assert(! name -> base_opt);
    NameSymbol* name_symbol =
        lex_stream -> NameSymbol(name -> identifier_token);

    // Check for inaccessible member type.
    if (state_stack.Size())
    {
        for (TypeSymbol* super_type = ThisType() -> super;
             super_type; super_type = super_type -> super)
        {
            assert(super_type -> expanded_type_table);
            TypeShadowSymbol* type_shadow_symbol = super_type ->
                expanded_type_table -> FindTypeShadowSymbol(name_symbol);
            if (type_shadow_symbol)
            {
                return type_shadow_symbol -> type_symbol;
            }
        }
    }
    // Check for an inaccessible import.
    return ImportType(name -> identifier_token, name_symbol);
}


//
// Finds a type by the given name, and add the dependence information. If one
// exists, but is not accessible, it is returned after an error. After other
// errors, control.no_type is returned.
//
TypeSymbol* Semantic::MustFindType(AstName* name)
{
    TypeSymbol* type;
    NameSymbol* name_symbol =
        lex_stream -> NameSymbol(name -> identifier_token);
    if (! name -> base_opt)
    {
        type = FindType(name -> identifier_token);
        //
        // If the type was not found, generate an appropriate error message.
        //
        if (! type)
        {
            type = FindInaccessibleType(name);
            if (type)
                ReportTypeInaccessible(name, type);
            else
            {
                // Try reading the file again, to force an error.
                NameSymbol* name_symbol =
                    lex_stream -> NameSymbol(name -> identifier_token);
                FileSymbol* file_symbol =
                    Control::GetFile(control, this_package, name_symbol);
                type = ReadType(file_symbol, this_package, name_symbol,
                                name -> identifier_token);
            }
        }
    }
    else // qualified name
    {
        ProcessPackageOrType(name -> base_opt);
        Symbol* symbol = name -> base_opt -> symbol;
        type = symbol -> TypeCast();
        if (type)
            type = MustFindNestedType(type, name);
        else
        {
            PackageSymbol* package = symbol -> PackageCast();
            type = package -> FindTypeSymbol(name_symbol);
            if (! type)
            {
                FileSymbol* file_symbol =
                    Control::GetFile(control, package, name_symbol);
                type = ReadType(file_symbol, package, name_symbol,
                                name -> identifier_token);
            }
            else if (type -> SourcePending())
                control.ProcessHeaders(type -> file_symbol);
            if (! TypeAccessCheck(type))
                ReportTypeInaccessible(name, type);
        }
    }

    //
    // Establish a dependence from the base_type to a type that it "must find".
    // Note that the only time an environment is not available is when were are
    // processing the type header of an outermost type.
    //
    if (state_stack.Size())
    {
        AddDependence(ThisType(), type);
        if (control.option.deprecation && type -> IsDeprecated() &&
            ! InDeprecatedContext())
        {
            ReportSemError(SemanticError::DEPRECATED_TYPE, name,
                           type -> ContainingPackageName(),
                           type -> ExternalName());
        }
    }
    if (type -> Anonymous() && ! type -> Bad())
    {
        ReportSemError(SemanticError::UNNAMED_TYPE_ACCESS, name,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }
    if (type -> ACC_SYNTHETIC() && ! type -> Bad())
    {
        ReportSemError(SemanticError::SYNTHETIC_TYPE_ACCESS, name,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }
    return type -> Bad() ? control.no_type : type;
}


void Semantic::ProcessType(AstType* type_expr)
{
    if (type_expr -> symbol)
        return; // already processed
    AstArrayType* array_type = type_expr -> ArrayTypeCast();
    AstType* actual_type = array_type ? array_type -> type : type_expr;
    AstTypeName* name = actual_type -> TypeNameCast();
    AstPrimitiveType* primitive_type = actual_type -> PrimitiveTypeCast();
    AstWildcard* wildcard_type = actual_type -> WildcardCast();
    if (wildcard_type)
    {
        // Process wildcard bounds if present
        if (wildcard_type -> bounds_opt)
        {
            ProcessType(wildcard_type -> bounds_opt);
        }

        // For type erasure, wildcards erase to their upper bound
        // ? -> Object
        // ? extends Number -> Number
        // ? super Integer -> Object
        TypeSymbol* wildcard_erasure;
        if (wildcard_type -> extends_token_opt && wildcard_type -> bounds_opt)
        {
            // ? extends T -> erases to T
            wildcard_erasure = wildcard_type -> bounds_opt -> symbol;
        }
        else
        {
            // ? or ? super T -> erases to Object
            wildcard_erasure = control.Object();
        }

        if (array_type)
            wildcard_erasure = wildcard_erasure -> GetArrayType(this, array_type -> NumBrackets());
        type_expr -> symbol = wildcard_erasure;
        return;
    }
    assert(name || primitive_type);
    //
    // Occaisionally, MustFindType finds a bad type (for example, if we
    // reference Other.java which has syntax errors), but does not know to
    // report the error.
    //
    unsigned error_count = NumErrors();
    TypeSymbol* type;
    if (primitive_type)
        type = FindPrimitiveType(primitive_type);
    else
    {
        if (name -> base_opt)
        {
            ProcessType(name -> base_opt);
            type = MustFindNestedType(name -> base_opt -> symbol,
                                      name -> name);
        }
        else type = MustFindType(name -> name);
        if (name -> type_arguments_opt)
        {
            name -> parameterized_type = ProcessTypeArguments(type, name -> type_arguments_opt);
        }
        else if (type -> IsGeneric() && control.option.source >= JopaOption::SDK1_5)
        {
            // Raw type usage: generic type used without type arguments
            // This is legal but generates unchecked warnings
            // Example: List instead of List<String>
            ReportSemError(SemanticError::UNCHECKED_TYPE_CONVERSION,
                          name,
                          type -> ContainingPackageName(),
                          type -> ExternalName());
        }
    }
    if (type -> Bad() && NumErrors() == error_count)
    {
        if (type == control.no_type)
        {
            ReportSemError(SemanticError::TYPE_NOT_FOUND, actual_type,
                           NULL,
                           lex_stream -> NameString(type_expr ->
                                                    IdentifierToken()));
        }
        else
        {
            ReportSemError(SemanticError::INVALID_TYPE_FOUND, actual_type,
                           lex_stream -> NameString(type_expr ->
                                                    IdentifierToken()));
        }
    }
    if (array_type)
        type = type -> GetArrayType(this, array_type -> NumBrackets());
    type_expr -> symbol = type;
}


//
// Initializes a static or instance field. In addition to checking the
// semantics, the initialization is added as a statement in the init_method,
// for easy bytecode emission, if it has an initializer and is not a constant.
//
void Semantic::InitializeVariable(AstFieldDeclaration* field_declaration,
                                  MethodSymbol* init_method)
{
    ThisMethod() = NULL;
    AstMethodDeclaration* declaration =
        (AstMethodDeclaration*) init_method -> declaration;
    assert(declaration -> method_body_opt);

    for (unsigned i = 0;
         i < field_declaration -> NumVariableDeclarators(); i++)
    {
        AstVariableDeclarator* variable_declarator =
            field_declaration -> VariableDeclarator(i);
        VariableSymbol* variable = variable_declarator -> symbol;

        if (variable)
        {
            ThisVariable() = variable;
            if (variable_declarator -> variable_initializer_opt)
            {
                variable_declarator -> pending = true;

                unsigned start_num_errors = NumErrors();
                ProcessVariableInitializer(variable_declarator);
                if (NumErrors() == start_num_errors)
                {
                    DefiniteFieldInitializer(variable_declarator);
                    if (! variable -> initial_value)
                    {
                        declaration -> method_body_opt ->
                            AddStatement(variable_declarator);
                    }
                }
                else if (variable -> ACC_FINAL())
                {
                    // Suppress further error messages.
                    DefinitelyAssignedVariables() ->
                        AssignElement(variable -> LocalVariableIndex());
                }

                variable_declarator -> pending = false;
            }
            variable -> MarkComplete();
        }
    }
}


//
// Adds an initializer block to the init_method, after checking its semantics,
// for easier bytecode emission.
//
inline void Semantic::ProcessInitializer(AstInitializerDeclaration* initializer,
                                         MethodSymbol* init_method)
{
    ThisVariable() = NULL;
    ThisMethod() = init_method;
    AstMethodDeclaration* declaration =
        (AstMethodDeclaration*) init_method -> declaration;

    LocalBlockStack().Push(declaration -> method_body_opt);
    LocalSymbolTable().Push(init_method -> block_symbol -> Table());

    //
    // Initializer blocks are always reachable, as prior blocks must be able
    // to complete normally.
    //
    initializer -> block -> is_reachable = true;

    if (initializer -> block -> explicit_constructor_opt)
    {
        AstStatement* explicit_ctor =
            initializer -> block -> explicit_constructor_opt;
        ReportSemError(SemanticError::MISPLACED_EXPLICIT_CONSTRUCTOR,
                       explicit_ctor);
    }
    ProcessInitializerModifiers(initializer);
    ProcessBlock(initializer -> block);
    DefiniteBlockInitializer(initializer -> block, LocalBlockStack().max_size);

    declaration -> method_body_opt -> AddStatement(initializer -> block);

    //
    // If the initializer has a higher max_variable_index than the overall
    // block, update max_variable_index in the method_body, accordingly.
    //
    if (init_method -> block_symbol -> max_variable_index <
        LocalBlockStack().TopMaxEnclosedVariableIndex())
    {
        init_method -> block_symbol -> max_variable_index =
            LocalBlockStack().TopMaxEnclosedVariableIndex();
    }

    if (! initializer -> block -> can_complete_normally)
        ReportSemError(SemanticError::ABRUPT_INITIALIZER, initializer);

    LocalBlockStack().Pop();
    LocalSymbolTable().Pop();
}


//
// Lazily create and return the static initializer for this type. The estimate
// is for the number of initializers that will be grouped into this method.
// This is called both when processing static initializers, and any time an
// assert statement is encountered (since assertions require an initialized
// static variable to operate).
//
MethodSymbol* Semantic::GetStaticInitializerMethod(unsigned estimate)
{
    TypeSymbol* this_type = ThisType();
    if (this_type -> static_initializer_method)
        return this_type -> static_initializer_method;

    StoragePool* ast_pool = compilation_unit -> ast_pool;
    TokenIndex loc = this_type -> declaration -> identifier_token;

    // The symbol table associated with this block has no elements.
    BlockSymbol* block_symbol = new BlockSymbol(0);
    block_symbol -> max_variable_index = 0;

    // The body of the static initializer. This will contain each initializer
    // block in sequence.
    AstMethodBody* block = ast_pool -> GenMethodBody();
    block -> left_brace_token = loc;
    block -> right_brace_token = loc;
    block -> block_symbol = block_symbol;
    block -> AllocateStatements(estimate);

    // The return type (void).
    AstType* return_type = ast_pool -> GenPrimitiveType(Ast::VOID_TYPE, loc);
    return_type -> symbol = control.void_type;

    // The method declaration. We leave some fields uninitialized, because
    // they are not needed in bytecode.cpp.
    AstMethodDeclaration* declaration = ast_pool -> GenMethodDeclaration();
    MethodSymbol* init_method =
        this_type -> InsertMethodSymbol(control.clinit_name_symbol);
    declaration -> type = return_type;
    declaration -> method_symbol = init_method;
    declaration -> method_body_opt = block;

    // The method symbol.
    init_method -> SetType(control.void_type);
    init_method -> SetFlags(AccessFlags::ACCESS_PRIVATE |
                            AccessFlags::ACCESS_FINAL |
                            AccessFlags::ACCESS_STATIC);
    if (this_type -> ACC_STRICTFP())
        init_method -> SetACC_STRICTFP();
    init_method -> SetContainingType(this_type);
    init_method -> SetBlockSymbol(block_symbol);
    init_method -> SetSignature(control);
    init_method -> declaration = declaration;

    this_type -> static_initializer_method = init_method;
    return init_method;
}


void Semantic::ProcessStaticInitializers(AstClassBody* class_body)
{
    //
    // Notice that the bodies of methods have not been processed yet when this
    // is called.  If any method contains an assert, it will generate a static
    // initializer for the $noassert variable as needed.  On the other hand, if
    // we already encountered an assert statement in an instance initializer,
    // the static initializer already exists.  The assert variable initializer
    // is magically implemented by bytecode.cpp, rather than adding all the AST
    // structure to the block of the static initializer.
    //
    TypeSymbol* this_type = ThisType();

    // Enum types need static initializers for enum constants even if there
    // are no explicit static initializers or class variables
    bool is_enum_with_constants = this_type -> IsEnum() && class_body -> owner &&
                                   class_body -> owner -> EnumDeclarationCast();

    if (class_body -> NumStaticInitializers() == 0 &&
        class_body -> NumClassVariables() == 0 &&
        !is_enum_with_constants)
    {
        return;
    }

    LocalBlockStack().max_size = 1;
    assert(FinalFields());

    //
    // Work out how many statements we'll need.
    //
    unsigned estimate = class_body -> NumStaticInitializers();
    for (unsigned i = 0; i < class_body -> NumClassVariables(); ++i)
    {
        estimate += class_body -> ClassVariable(i) -> NumVariableDeclarators();
    }

    // For enums, add space for enum constant initializations
    if (this_type -> IsEnum() && class_body -> owner)
    {
        AstEnumDeclaration* enum_decl = class_body -> owner -> EnumDeclarationCast();
        if (enum_decl)
        {
            estimate += enum_decl -> NumEnumConstants();
        }
    }

    MethodSymbol* init_method = GetStaticInitializerMethod(estimate);

    //
    // For enum types, initialize enum constants first
    //
    if (this_type -> IsEnum() && class_body -> owner)
    {
        AstEnumDeclaration* enum_decl = class_body -> owner -> EnumDeclarationCast();
        if (enum_decl && enum_decl -> NumEnumConstants() > 0)
        {
            StoragePool* ast_pool = compilation_unit -> ast_pool;
            TokenIndex loc = class_body -> identifier_token;
            AstMethodDeclaration* declaration =
                (AstMethodDeclaration*) init_method -> declaration;

            assert(declaration && "init_method declaration is null");
            assert(declaration -> method_body_opt && "method_body_opt is null");

            for (unsigned i = 0; i < enum_decl -> NumEnumConstants(); i++)
            {
                AstEnumConstant* enum_constant = enum_decl -> EnumConstant(i);
                VariableSymbol* field_symbol = enum_constant -> field_symbol;

                if (field_symbol)
                {
                    // Create: CONSTANT_NAME = new EnumType("CONSTANT_NAME", ordinal)

                    // Left side: field reference
                    AstName* field_ref = ast_pool -> GenName(enum_constant -> identifier_token);
                    field_ref -> symbol = field_symbol;

                    // Right side: new EnumType("name", ordinal)
                    AstClassCreationExpression* creation = ast_pool -> GenClassCreationExpression();
                    creation -> new_token = loc;
                    AstName* type_name = ast_pool -> GenName(loc);
                    type_name -> symbol = this_type;
                    creation -> class_type = ast_pool -> GenTypeName(type_name);
                    creation -> class_type -> symbol = this_type;

                    // Arguments: ("CONSTANT_NAME", ordinal)
                    creation -> arguments = ast_pool -> GenArguments(loc, loc);
                    creation -> arguments -> AllocateArguments(2);

                    // String name argument - create string literal
                    const NameSymbol* name_symbol = field_symbol -> Identity();
                    AstStringLiteral* name_arg = ast_pool -> GenStringLiteral(loc);
                    name_arg -> value = name_symbol -> Utf8_literal;
                    name_arg -> symbol = control.String();
                    creation -> arguments -> AddArgument(name_arg);

                    // int ordinal argument - create integer literal
                    AstIntegerLiteral* ordinal_arg = ast_pool -> GenIntegerLiteral(loc);
                    ordinal_arg -> value = control.int_pool.FindOrInsert(enum_constant -> ordinal);
                    ordinal_arg -> symbol = control.int_type;
                    creation -> arguments -> AddArgument(ordinal_arg);

                    // Set the constructor symbol (not the type!)
                    // For enums, we need the private constructor with (String, int) parameters
                    // Find it by iterating through all methods in the type
                    MethodSymbol* constructor = NULL;
                    for (unsigned m = 0; m < this_type -> NumMethodSymbols(); m++)
                    {
                        MethodSymbol* method = this_type -> MethodSym(m);
                        if (method -> Identity() == control.init_name_symbol &&
                            method -> NumFormalParameters() == 2)
                        {
                            constructor = method;
                            break;
                        }
                    }
                    creation -> symbol = constructor;

                    // Create assignment: field = creation
                    AstAssignmentExpression* assignment = ast_pool -> GenAssignmentExpression(
                        AstAssignmentExpression::SIMPLE_EQUAL, loc);
                    assignment -> left_hand_side = field_ref;
                    assignment -> expression = creation;
                    assignment -> symbol = this_type;

                    // Wrap in expression statement
                    AstExpressionStatement* stmt = ast_pool -> GenExpressionStatement();
                    stmt -> expression = assignment;
                    stmt -> semicolon_token_opt = loc;

                    // Add to static initializer
                    declaration -> method_body_opt -> AddStatement(stmt);
                }
            }
        }
    }

    //
    // The static initializers and class variable initializers are executed
    // in textual order, with the exception that assignments may occur before
    // declaration. See JLS 8.5.
    //
    unsigned j = 0;
    unsigned k = 0;
    while (j < class_body -> NumClassVariables() &&
           k < class_body -> NumStaticInitializers())
    {
        if (class_body -> ClassVariable(j) -> semicolon_token <
            class_body -> StaticInitializer(k) -> block -> right_brace_token)
        {
            InitializeVariable(class_body -> ClassVariable(j++), init_method);
        }
        else
        {
            ProcessInitializer(class_body -> StaticInitializer(k++),
                               init_method);
        }
    }
    while (j < class_body -> NumClassVariables())
    {
        InitializeVariable(class_body -> ClassVariable(j++), init_method);
    }
    while (k < class_body -> NumStaticInitializers())
    {
        ProcessInitializer(class_body -> StaticInitializer(k++),
                           init_method);
    }

    //
    // Check that each static final variable has been initialized by now.
    // If not, issue an error and assume it is.  Notice that for inner
    // classes, we have already reported that a non-constant static
    // field is illegal, so we only need an error here for top-level
    // and static classes.
    //
    for (unsigned l = 0; l < FinalFields() -> Length(); l++)
    {
        VariableSymbol* final_var = (*FinalFields())[l];

        // Enum constants are static final but initialized in <clinit>
        if (final_var -> ACC_ENUM())
            continue;

        if (final_var -> ACC_STATIC() &&
            ! DefinitelyAssignedVariables() -> da_set[l])
        {
            if (! this_type -> IsInner())
            {
                ReportSemError(SemanticError::UNINITIALIZED_STATIC_FINAL_VARIABLE,
                               final_var -> declarator, final_var -> Name());
            }
            DefinitelyAssignedVariables() -> AssignElement(l);
        }
    }

    //
    // If an initialization method has been defined, update its
    // max_block_depth.
    //
    if (this_type -> static_initializer_method)
    {
        MethodSymbol* init_method = this_type -> static_initializer_method;
        init_method -> max_block_depth = LocalBlockStack().max_size;
        init_method -> block_symbol -> CompressSpace(); // space optimization
    }
}


void Semantic::ProcessInstanceInitializers(AstClassBody* class_body)
{
    //
    // For instance initializers, we create a method to do all the
    // initialization. We name the method 'this', which is legal in VM's
    // but an illegal user name, to avoid name clashes. Constructors which
    // do not invoke another constructor via the this() statement will call
    // the instance initializer method after calling super(). We rely on the
    // fact that VM's allow assignment of final instance variables in an
    // instance method, rather than requiring it to be in a constructor.
    //
    if (class_body -> NumInstanceInitializers() == 0 &&
        class_body -> NumInstanceVariables() == 0)
    {
        return;
    }

    TypeSymbol* this_type = ThisType();
    LocalBlockStack().max_size = 1;
    StoragePool* ast_pool = compilation_unit -> ast_pool;
    TokenIndex loc = this_type -> declaration -> identifier_token;

    // The symbol table associated with this block has one element, the
    // current instance 'this'.
    BlockSymbol* block_symbol = new BlockSymbol(1);
    block_symbol -> max_variable_index = 1;

    // The combined block of the instance initializations. This will contain
    // each initializer block in sequence, and be inlined into constructors.
    AstMethodBody* block = ast_pool -> GenMethodBody();
    block -> left_brace_token = loc;
    block -> right_brace_token = loc;
    block -> block_symbol = block_symbol;

    // The return type (void).
    AstType* return_type = ast_pool -> GenPrimitiveType(Ast::VOID_TYPE, loc);
    return_type -> symbol = control.void_type;

    // The method declaration. We leave some fields uninitialized, because
    // they are not needed in bytecode.cpp.
    AstMethodDeclaration* declaration = ast_pool -> GenMethodDeclaration();
    MethodSymbol* init_method =
        this_type -> InsertMethodSymbol(control.block_init_name_symbol);
    declaration -> type = return_type;
    declaration -> method_symbol = init_method;
    declaration -> method_body_opt = block;

    // The method symbol.
    init_method -> SetType(control.void_type);
    init_method -> SetFlags(AccessFlags::ACCESS_PRIVATE |
                            AccessFlags::ACCESS_FINAL |
                            AccessFlags::ACCESS_SYNTHETIC);
    if (this_type -> ACC_STRICTFP())
        init_method -> SetACC_STRICTFP();
    init_method -> SetContainingType(this_type);
    init_method -> SetBlockSymbol(block_symbol);
    init_method -> SetSignature(control);
    init_method -> declaration = declaration;

    assert(this_type -> instance_initializer_method == NULL);
    this_type -> instance_initializer_method = init_method;

    //
    // Make sure the instance final fields are properly set.
    //
    assert(FinalFields());
    for (unsigned i = 0; i < FinalFields() -> Length(); i++)
    {
        VariableSymbol* variable_symbol = (*FinalFields())[i];
        if (variable_symbol -> ACC_STATIC())
        {
            DefinitelyAssignedVariables() -> AssignElement(i);
            BlankFinals() -> RemoveElement(i);
        }
        else
            DefinitelyAssignedVariables() -> ReclaimElement(i);
    }

    //
    // Work out how many statements we'll need.
    //
    unsigned estimate = class_body -> NumInstanceInitializers();
    for (unsigned i = 0; i < class_body -> NumInstanceVariables(); ++i)
    {
        estimate += class_body -> InstanceVariable(i) ->
            NumVariableDeclarators();
    }
    block -> AllocateStatements(estimate);

    //
    // Initialization code is executed by every constructor that does not call
    // this(), just after the superclass constructor is called, in textual
    // order along with any instance variable initializations.
    //
    unsigned j = 0;
    unsigned k = 0;
    while (j < class_body -> NumInstanceVariables() &&
           k < class_body -> NumInstanceInitializers())
    {
        if (class_body -> InstanceVariable(j) -> semicolon_token <
            class_body -> InstanceInitializer(k) -> block -> right_brace_token)
        {
            InitializeVariable(class_body -> InstanceVariable(j++),
                               init_method);
        }
        else
        {
            ProcessInitializer(class_body -> InstanceInitializer(k++),
                               init_method);
        }
    }
    while (j < class_body -> NumInstanceVariables())
    {
        InitializeVariable(class_body -> InstanceVariable(j++), init_method);
    }
    while (k < class_body -> NumInstanceInitializers())
    {
        ProcessInitializer(class_body -> InstanceInitializer(k++),
                           init_method);
    }

    //
    // Note that unlike the case of static fields, we do not ensure here that
    // each final instance variable has been initialized at this point. This
    // is because the user may choose instead to initialize such a final
    // variable in every constructor instead. See body.cpp
    //
    init_method -> max_block_depth = LocalBlockStack().max_size;
    init_method -> block_symbol -> CompressSpace(); // space optimization
}


} // Close namespace Jopa block

