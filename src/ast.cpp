#include "ast.h"
#include "symbol.h"
#ifdef JOPA_DEBUG
# include "stream.h"
#endif // JOPA_DEBUG


namespace Jopa { // Open namespace Jopa block
#ifdef JOPA_DEBUG
unsigned Ast::count = 0;
#endif


StoragePool::~StoragePool()
{
    // Delete BlockSymbol objects that were registered for cleanup
    for (BlockSymbol* symbol : block_symbols_to_delete)
        delete symbol;

    if (base)
        for (unsigned i = 0; i <= base_index; i++)
            delete [] base[i];
    delete [] base;
}


//
// Allocate another block of storage for the VariableSymbol array.
//
void VariableSymbolArray::AllocateMoreSpace()
{
    //
    //
    // The variable size always indicates the maximum number of
    // elements that has been allocated for the array.
    // Initially, it is set to 0 to indicate that the array is empty.
    // The pool of available elements is divided into segments of size
    // 2**log_blksize each. Each segment is pointed to by a slot in
    // the array base.
    //
    // By dividing size by the size of the segment we obtain the
    // index for the next segment in base. If base is full, it is
    // reallocated.
    //
    //
    size_t k = size >> log_blksize; /* which segment? */

    //
    // If the base is overflowed, reallocate it and initialize the new
    // elements to NULL.
    //
    if (k == base_size)
    {
        int old_base_size = base_size;
        T** old_base = base;

        base_size += base_increment;

        // There must be enough room to allocate base
        assert(base_size <= pool -> Blksize());

        base = (T**) pool -> Alloc(sizeof(T*) * base_size);

        if (old_base)
        {
            memcpy(base, old_base, old_base_size * sizeof(T*));
        }
        memset(&base[old_base_size], 0,
               (base_size - old_base_size) * sizeof(T*));
    }

    //
    // We allocate a new segment and place its adjusted address in
    // base[k]. The adjustment allows us to index the segment directly,
    // instead of having to perform a subtraction for each reference.
    // See operator[] below. There must be enough room to allocate block.
    //
    assert(Blksize() <= pool -> Blksize());

    base[k] = (T*) pool -> Alloc(sizeof(T) * Blksize());
    base[k] -= size;

    //
    // Finally, we update size.
    //
    size += Blksize();
}


VariableSymbolArray::VariableSymbolArray(StoragePool* p,
                                         unsigned estimate = 0)
    : pool(p)
{
    // There must be enough space in the storage pool to move !!!
    assert(pool -> Blksize() >= 256);

    if (estimate == 0)
        log_blksize = 6; // take a guess
    else
    {
        for (log_blksize = 1;
             ((1U << log_blksize) < estimate) && (log_blksize < 31);
             log_blksize++)
            ;
    }

    //
    // Increment a base_increment size that is big enough not to have to
    // be reallocated. Find a block size that is smaller that the block
    // size of the pool.
    //
    base_increment = (Blksize() > pool -> Blksize()
                      ? Blksize() / pool -> Blksize() : 1) * 2;
    while (Blksize() >= pool -> Blksize())
        log_blksize--;

    base_size = 0;
    size = 0;
    top = 0;
    base = NULL;
}


void AstCompilationUnit::FreeAst()
{
     delete ast_pool;
}

//
// This procedure uses a quick sort algorithm to sort the cases in a switch
// statement. Element 0 is not sorted, because it is the default case (and
// may be NULL).
//
void AstSwitchStatement::SortCases()
{
    int lower;
    int upper;
    int lostack[32];
    int histack[32];
    int top = 0;
    int i;
    int j;
    CaseElement pivot;
    CaseElement temp;

    lostack[top] = 1;
    histack[top] = num_cases;

    while (top >= 0)
    {
        lower = lostack[top];
        upper = histack[top];
        top--;

        while (upper > lower)
        {
            //
            // The array is most-likely almost sorted. Therefore,
            // we use the middle element as the pivot element.
            //
            i = (lower + upper) >> 1;
            pivot = *cases[i];
            *cases[i] = *cases[lower];

            //
            // Split the array section indicated by LOWER and UPPER
            // using ARRAY(LOWER) as the pivot.
            //
            i = lower;
            for (j = lower + 1; j <= upper; j++)
                if (*cases[j] < pivot)
                {
                    temp = *cases[++i];
                    *cases[i] = *cases[j];
                    *cases[j] = temp;
                }
            *cases[lower] = *cases[i];
            *cases[i] = pivot;

            top++;
            if ((i - lower) < (upper - i))
            {
                lostack[top] = i + 1;
                histack[top] = upper;
                upper = i - 1;
            }
            else
            {
                histack[top] = i - 1;
                lostack[top] = lower;
                lower = i + 1;
            }
        }
    }
}

//
// Performs a binary search to locate the correct case (including the
// default case) for a constant expression value. Returns NULL if the switch
// is a no-op for this constant.
//
CaseElement* AstSwitchStatement::CaseForValue(i4 value)
{
    unsigned lower = 1;
    unsigned upper = num_cases;
    while (lower <= upper)
    {
        unsigned mid = (lower + upper) >> 1;
        CaseElement* elt = cases[mid];
        if (elt -> value == value)
            return elt;
        if (elt -> value > value)
            upper = mid - 1;
        else
            lower = mid + 1;
    }
    return cases[0];
}


TypeSymbol* AstMemberValue::Type()
{
    // Check if this is an expression with a resolved type (for generics type substitution)
    AstExpression* expr = ExpressionCast();
    if (expr && expr -> resolved_type)
        return expr -> resolved_type;

    return ! symbol ? (TypeSymbol*) NULL
        : symbol -> Kind() == Symbol::TYPE
        ? (TypeSymbol*) symbol
        : symbol -> Kind() == Symbol::VARIABLE
        ? ((VariableSymbol*) symbol) -> Type()
        : symbol -> Kind() == Symbol::METHOD
        ? ((MethodSymbol*) symbol) -> Type()
        : (TypeSymbol*) NULL;
}


Ast* AstBlock::Clone(StoragePool* ast_pool)
{
    AstBlock* clone = ast_pool -> GenBlock();
    clone -> CloneBlock(ast_pool, this);
    return clone;
}

void AstBlock::CloneBlock(StoragePool* ast_pool, AstBlock* orig)
{
    other_tag = orig -> other_tag;
    label_opt = orig -> label_opt;
    nesting_level = orig -> nesting_level;
    left_brace_token = orig -> left_brace_token;
    unsigned count = orig -> NumStatements();
    AllocateStatements(count);
    for (unsigned i = 0; i < count; i++)
        AddStatement((AstStatement*) orig -> Statement(i) -> Clone(ast_pool));
    right_brace_token = orig -> right_brace_token;
    no_braces = orig -> no_braces;
}

Ast* AstName::Clone(StoragePool* ast_pool)
{
    AstName* clone = ast_pool -> GenName(identifier_token);
    if (base_opt)
        clone -> base_opt = (AstName*) base_opt -> Clone(ast_pool);
    if (resolution_opt)
        clone -> resolution_opt =
            (AstExpression*) resolution_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstPrimitiveType::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenPrimitiveType(kind, primitive_kind_token);
}

Ast* AstBrackets::Clone(StoragePool* ast_pool)
{
    AstBrackets* clone =
        ast_pool -> GenBrackets(left_bracket_token, right_bracket_token);
    clone -> dims = dims;
    return clone;
}

Ast* AstArrayType::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenArrayType((AstType*) type -> Clone(ast_pool),
                                    ((AstBrackets*) brackets ->
                                     Clone(ast_pool)));
}

Ast* AstWildcard::Clone(StoragePool* ast_pool)
{
    AstWildcard* clone = ast_pool -> GenWildcard(question_token);
    clone -> extends_token_opt = extends_token_opt;
    clone -> super_token_opt = super_token_opt;
    if (bounds_opt)
        clone -> bounds_opt = (AstType*) bounds_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstTypeArguments::Clone(StoragePool* ast_pool)
{
    AstTypeArguments* clone = ast_pool -> GenTypeArguments(left_angle_token,
                                                           right_angle_token);
    clone -> AllocateTypeArguments(NumTypeArguments());
    for (unsigned i = 0; i < NumTypeArguments(); i++)
        clone -> AddTypeArgument((AstType*) TypeArgument(i) ->
                                 Clone(ast_pool));
    return clone;
}

Ast* AstTypeName::Clone(StoragePool* ast_pool)
{
    AstTypeName* clone =
        ast_pool -> GenTypeName((AstName*) name -> Clone(ast_pool));
    if (base_opt)
        clone -> base_opt = (AstTypeName*) base_opt -> Clone(ast_pool);
    if (type_arguments_opt)
        clone -> type_arguments_opt =
            (AstTypeArguments*) type_arguments_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstMemberValuePair::Clone(StoragePool* ast_pool)
{
    AstMemberValuePair* clone = ast_pool -> GenMemberValuePair();
    clone -> identifier_token_opt = identifier_token_opt;
    clone -> member_value = (AstMemberValue*) member_value -> Clone(ast_pool);
    return clone;
}

Ast* AstAnnotation::Clone(StoragePool* ast_pool)
{
    AstAnnotation* clone = ast_pool -> GenAnnotation();
    clone -> at_token = at_token;
    clone -> name = (AstName*) name -> Clone(ast_pool);
    clone -> AllocateMemberValuePairs(NumMemberValuePairs());
    for (unsigned i = 0; i < NumMemberValuePairs(); i++)
        clone -> AddMemberValuePair((AstMemberValuePair*)
                                    MemberValuePair(i) -> Clone(ast_pool));
    clone -> right_paren_token_opt = right_paren_token_opt;
    return clone;
}

Ast* AstModifierKeyword::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenModifierKeyword(modifier_token);
}

Ast* AstModifiers::Clone(StoragePool* ast_pool)
{
    AstModifiers* clone = ast_pool -> GenModifiers();
    clone -> AllocateModifiers(NumModifiers());
    for (unsigned i = 0; i < NumModifiers(); i++)
    {
        if (Modifier(i) -> ModifierKeywordCast())
            clone -> AddModifier((AstModifierKeyword*)
                                 Modifier(i) -> Clone(ast_pool));
        else clone -> AddModifier((AstAnnotation*)
                                  Modifier(i) -> Clone(ast_pool));
    }
    clone -> static_token_opt = static_token_opt;
    return clone;
}

Ast* AstPackageDeclaration::Clone(StoragePool* ast_pool)
{
    AstPackageDeclaration* clone = ast_pool -> GenPackageDeclaration();
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    clone -> package_token = package_token;
    clone -> name = (AstName*) name -> Clone(ast_pool);
    clone -> semicolon_token = semicolon_token;
    return clone;
}

Ast* AstImportDeclaration::Clone(StoragePool* ast_pool)
{
    AstImportDeclaration* clone = ast_pool -> GenImportDeclaration();
    clone -> import_token = import_token;
    clone -> static_token_opt = static_token_opt;
    clone -> name = (AstName*) name -> Clone(ast_pool);
    clone -> star_token_opt = star_token_opt;
    clone -> semicolon_token = semicolon_token;
    return clone;
}

Ast* AstCompilationUnit::Clone(StoragePool* ast_pool)
{
    unsigned i;
    AstCompilationUnit* clone = ast_pool -> GenCompilationUnit();
    clone -> other_tag = other_tag;
    if (package_declaration_opt)
        clone -> package_declaration_opt = (AstPackageDeclaration*)
            package_declaration_opt -> Clone(ast_pool);
    clone -> AllocateImportDeclarations(NumImportDeclarations());
    for (i = 0; i < NumImportDeclarations(); i++)
        clone -> AddImportDeclaration((AstImportDeclaration*)
                                      ImportDeclaration(i) -> Clone(ast_pool));
    clone -> AllocateTypeDeclarations(NumTypeDeclarations());
    for (i = 0; i < NumTypeDeclarations(); i++)
        clone -> AddTypeDeclaration((AstDeclaredType*) TypeDeclaration(i) ->
                                    Clone(ast_pool));
    return clone;
}

Ast* AstEmptyDeclaration::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenEmptyDeclaration(semicolon_token);
}

Ast* AstClassBody::Clone(StoragePool* ast_pool)
{
    AstClassBody* clone = ast_pool -> GenClassBody();
    clone -> identifier_token = identifier_token;
    clone -> left_brace_token = left_brace_token;
    clone -> AllocateClassBodyDeclarations(NumClassBodyDeclarations());
    clone -> AllocateInstanceVariables(NumInstanceVariables());
    clone -> AllocateClassVariables(NumClassVariables());
    clone -> AllocateMethods(NumMethods());
    clone -> AllocateConstructors(NumConstructors());
    clone -> AllocateStaticInitializers(NumStaticInitializers());
    clone -> AllocateInstanceInitializers(NumInstanceInitializers());
    clone -> AllocateNestedClasses(NumNestedClasses());
    clone -> AllocateNestedEnums(NumNestedEnums());
    clone -> AllocateNestedInterfaces(NumNestedInterfaces());
    clone -> AllocateNestedAnnotations(NumNestedAnnotations());
    clone -> AllocateEmptyDeclarations(NumEmptyDeclarations());
    for (unsigned i = 0; i < NumClassBodyDeclarations(); i++)
        clone -> AddClassBodyDeclaration((AstDeclaredType*)
                                         ClassBodyDeclaration(i) ->
                                         Clone(ast_pool));
    clone -> right_brace_token = right_brace_token;
    return clone;
}

void AstClassBody::AddClassBodyDeclaration(AstDeclared* member)
{
    assert(class_body_declarations);
    AstFieldDeclaration* field_declaration = member -> FieldDeclarationCast();
    AstMethodDeclaration* method_declaration =
        member -> MethodDeclarationCast();
    AstConstructorDeclaration* constructor_declaration =
        member -> ConstructorDeclarationCast();
    AstInitializerDeclaration* initializer =
        member -> InitializerDeclarationCast();
    AstClassDeclaration* class_declaration = member -> ClassDeclarationCast();
    AstEnumDeclaration* enum_declaration = member -> EnumDeclarationCast();
    AstInterfaceDeclaration* interface_declaration =
        member -> InterfaceDeclarationCast();
    AstAnnotationDeclaration* annotation_declaration =
        member -> AnnotationDeclarationCast();

    class_body_declarations -> Next() = member;
    if (field_declaration)
    {
        if (field_declaration -> StaticFieldCast())
            AddClassVariable(field_declaration);
        else AddInstanceVariable(field_declaration);
    }
    else if (method_declaration)
        AddMethod(method_declaration);
    else if (constructor_declaration)
        AddConstructor(constructor_declaration);
    else if (initializer)
    {
        if (initializer -> StaticInitializerCast())
            AddStaticInitializer(initializer);
        else AddInstanceInitializer(initializer);
    }
    else if (class_declaration)
        AddNestedClass(class_declaration);
    else if (enum_declaration)
        AddNestedEnum(enum_declaration);
    else if (interface_declaration)
        AddNestedInterface(interface_declaration);
    else if (annotation_declaration)
        AddNestedAnnotation(annotation_declaration);
    else AddEmptyDeclaration((AstEmptyDeclaration*) member);
}

Ast* AstTypeParameter::Clone(StoragePool* ast_pool)
{
    AstTypeParameter* clone = ast_pool -> GenTypeParameter(identifier_token);
    clone -> AllocateBounds(NumBounds());
    for (unsigned i = 0; i < NumBounds(); i++)
        clone -> AddBound((AstTypeName*) Bound(i) -> Clone(ast_pool));
    return clone;
}

Ast* AstTypeParameters::Clone(StoragePool* ast_pool)
{
    AstTypeParameters* clone = ast_pool -> GenTypeParameters();
    clone -> left_angle_token = left_angle_token;
    clone -> AllocateTypeParameters(NumTypeParameters());
    for (unsigned i = 0; i < NumTypeParameters(); i++)
        clone -> AddTypeParameter((AstTypeParameter*) TypeParameter(i) ->
                                  Clone(ast_pool));
    clone -> right_angle_token = right_angle_token;
    return clone;
}

Ast* AstClassDeclaration::Clone(StoragePool* ast_pool)
{
    AstClassDeclaration* clone = ast_pool -> GenClassDeclaration();
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    clone -> class_token = class_token;
    if (type_parameters_opt)
        clone -> type_parameters_opt =
            (AstTypeParameters*) type_parameters_opt -> Clone(ast_pool);
    if (super_opt)
        clone -> super_opt = (AstTypeName*) super_opt -> Clone(ast_pool);
    clone -> AllocateInterfaces(NumInterfaces());
    for (unsigned i = 0; i < NumInterfaces(); i++)
        clone -> AddInterface((AstTypeName*) Interface(i) -> Clone(ast_pool));
    clone -> class_body = (AstClassBody*) class_body -> Clone(ast_pool);
    clone -> class_body -> owner = clone;
    return clone;
}

Ast* AstArrayInitializer::Clone(StoragePool* ast_pool)
{
    AstArrayInitializer* clone = ast_pool -> GenArrayInitializer();
    clone -> left_brace_token = left_brace_token;
    clone -> AllocateVariableInitializers(NumVariableInitializers());
    for (unsigned i = 0; i < NumVariableInitializers(); i++)
        clone -> AddVariableInitializer((AstMemberValue*)
                                        VariableInitializer(i) ->
                                        Clone(ast_pool));
    clone -> right_brace_token = right_brace_token;
    return clone;
}

Ast* AstVariableDeclaratorId::Clone(StoragePool* ast_pool)
{
    AstVariableDeclaratorId* clone = ast_pool -> GenVariableDeclaratorId();
    clone -> identifier_token = identifier_token;
    if (brackets_opt)
        clone -> brackets_opt = (AstBrackets*) brackets_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstVariableDeclarator::Clone(StoragePool* ast_pool)
{
    AstVariableDeclarator* clone = ast_pool -> GenVariableDeclarator();
    clone -> variable_declarator_name = (AstVariableDeclaratorId*)
        variable_declarator_name -> Clone(ast_pool);
    if (variable_initializer_opt)
        clone -> variable_initializer_opt =
            variable_initializer_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstFieldDeclaration::Clone(StoragePool* ast_pool)
{
    AstFieldDeclaration* clone = ast_pool -> GenFieldDeclaration();
    clone -> other_tag = other_tag;
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    clone -> type = (AstType*) type -> Clone(ast_pool);
    clone -> AllocateVariableDeclarators(NumVariableDeclarators());
    for (unsigned i = 0; i < NumVariableDeclarators(); i++)
        clone -> AddVariableDeclarator((AstVariableDeclarator*)
                                       VariableDeclarator(i) ->
                                       Clone(ast_pool));
    clone -> semicolon_token = semicolon_token;
    return clone;
}

Ast* AstFormalParameter::Clone(StoragePool* ast_pool)
{
    AstFormalParameter* clone = ast_pool -> GenFormalParameter();
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    clone -> type = (AstType*) type -> Clone(ast_pool);
    clone -> ellipsis_token_opt = ellipsis_token_opt;
    clone -> formal_declarator =
        (AstVariableDeclarator*) formal_declarator -> Clone(ast_pool);
    return clone;
}

Ast* AstMethodDeclarator::Clone(StoragePool* ast_pool)
{
    AstMethodDeclarator* clone = ast_pool -> GenMethodDeclarator();
    clone -> identifier_token = identifier_token;
    clone -> left_parenthesis_token = left_parenthesis_token;
    clone -> AllocateFormalParameters(NumFormalParameters());
    for (unsigned i = 0; i < NumFormalParameters(); i++)
        clone -> AddFormalParameter((AstFormalParameter*)
                                    FormalParameter(i) -> Clone(ast_pool));
    clone -> right_parenthesis_token = right_parenthesis_token;
    if (brackets_opt)
        clone -> brackets_opt = (AstBrackets*) brackets_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstMethodBody::Clone(StoragePool* ast_pool)
{
    AstMethodBody* clone = ast_pool -> GenMethodBody();
    clone -> CloneBlock(ast_pool, this);
    if (explicit_constructor_opt)
        clone -> explicit_constructor_opt =
            (AstStatement*) explicit_constructor_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstMethodDeclaration::Clone(StoragePool* ast_pool)
{
    AstMethodDeclaration* clone = ast_pool -> GenMethodDeclaration();
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    if (type_parameters_opt)
        clone -> type_parameters_opt =
            (AstTypeParameters*) type_parameters_opt -> Clone(ast_pool);
    clone -> type = (AstType*) type -> Clone(ast_pool);
    clone -> method_declarator =
        (AstMethodDeclarator*) method_declarator -> Clone(ast_pool);
    clone -> AllocateThrows(NumThrows());
    for (unsigned i = 0; i < NumThrows(); i++)
        clone -> AddThrow((AstTypeName*) Throw(i) -> Clone(ast_pool));
    if (default_value_opt)
        clone -> default_value_opt =
            (AstMemberValue*) default_value_opt -> Clone(ast_pool);
    if (method_body_opt)
        clone -> method_body_opt =
            (AstMethodBody*) method_body_opt -> Clone(ast_pool);
    clone -> semicolon_token_opt = semicolon_token_opt;
    return clone;
}

Ast* AstInitializerDeclaration::Clone(StoragePool* ast_pool)
{
    AstInitializerDeclaration* clone = ast_pool -> GenInitializerDeclaration();
    clone -> other_tag = other_tag;
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    clone -> block = (AstMethodBody*) block -> Clone(ast_pool);
    return clone;
}

Ast* AstArguments::Clone(StoragePool* ast_pool)
{
    unsigned i;
    AstArguments* clone = ast_pool -> GenArguments(left_parenthesis_token,
                                                   right_parenthesis_token);
    clone -> AllocateArguments(NumArguments());
    for (i = 0; i < NumArguments(); i++)
        clone -> AddArgument((AstExpression*) Argument(i) -> Clone(ast_pool));
    clone -> AllocateLocalArguments(NumLocalArguments());
    for (i = 0; i < NumLocalArguments(); i++)
        clone -> AddLocalArgument((AstName*) LocalArgument(i) ->
                                  Clone(ast_pool));
    clone -> other_tag = other_tag;
    return clone;
}

Ast* AstThisCall::Clone(StoragePool* ast_pool)
{
    AstThisCall* clone = ast_pool -> GenThisCall();
    if (type_arguments_opt)
        clone -> type_arguments_opt =
            (AstTypeArguments*) type_arguments_opt -> Clone(ast_pool);
    clone -> this_token = this_token;
    clone -> arguments = (AstArguments*) arguments -> Clone(ast_pool);
    clone -> semicolon_token = semicolon_token;
    return clone;
}

Ast* AstSuperCall::Clone(StoragePool* ast_pool)
{
    AstSuperCall* clone = ast_pool -> GenSuperCall();
    if (base_opt)
        clone -> base_opt = (AstExpression*) base_opt -> Clone(ast_pool);
    if (type_arguments_opt)
        clone -> type_arguments_opt =
            (AstTypeArguments*) type_arguments_opt -> Clone(ast_pool);
    clone -> super_token = super_token;
    clone -> arguments = (AstArguments*) arguments -> Clone(ast_pool);
    clone -> semicolon_token = semicolon_token;
    return clone;
}

Ast* AstConstructorDeclaration::Clone(StoragePool* ast_pool)
{
    AstConstructorDeclaration* clone = ast_pool -> GenConstructorDeclaration();
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    if (type_parameters_opt)
        clone -> type_parameters_opt =
            (AstTypeParameters*) type_parameters_opt -> Clone(ast_pool);
    clone -> constructor_declarator =
        (AstMethodDeclarator*) constructor_declarator -> Clone(ast_pool);
    clone -> AllocateThrows(NumThrows());
    for (unsigned i = 0; i < NumThrows(); i++)
        clone -> AddThrow((AstTypeName*) Throw(i) -> Clone(ast_pool));
    clone -> constructor_body =
        (AstMethodBody*) constructor_body -> Clone(ast_pool);
    return clone;
}

Ast* AstEnumDeclaration::Clone(StoragePool* ast_pool)
{
    unsigned i;
    AstEnumDeclaration* clone = ast_pool -> GenEnumDeclaration();
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    clone -> AllocateInterfaces(NumInterfaces());
    for (i = 0; i < NumInterfaces(); i++)
        clone -> AddInterface((AstTypeName*) Interface(i) -> Clone(ast_pool));
    clone -> AllocateEnumConstants(NumEnumConstants());
    for (i = 0; i < NumEnumConstants(); i++)
        clone -> AddEnumConstant((AstEnumConstant*) EnumConstant(i) ->
                                 Clone(ast_pool));
    clone -> class_body = (AstClassBody*) class_body -> Clone(ast_pool);
    clone -> class_body -> owner = clone;
    return clone;
}

Ast* AstEnumConstant::Clone(StoragePool* ast_pool)
{
    AstEnumConstant* clone = ast_pool -> GenEnumConstant(identifier_token);
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    if (arguments_opt)
        clone -> arguments_opt =
            (AstArguments*) arguments_opt -> Clone(ast_pool);
    if (class_body_opt)
        clone -> class_body_opt =
            (AstClassBody*) class_body_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstInterfaceDeclaration::Clone(StoragePool* ast_pool)
{
    AstInterfaceDeclaration* clone = ast_pool -> GenInterfaceDeclaration();
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    clone -> interface_token = interface_token;
    if (type_parameters_opt)
        clone -> type_parameters_opt =
            (AstTypeParameters*) type_parameters_opt -> Clone(ast_pool);
    clone -> AllocateInterfaces(NumInterfaces());
    for (unsigned i = 0; i < NumInterfaces(); i++)
        clone -> AddInterface((AstTypeName*) Interface(i) -> Clone(ast_pool));
    clone -> class_body = (AstClassBody*) class_body -> Clone(ast_pool);
    clone -> class_body -> owner = clone;
    return clone;
}

Ast* AstAnnotationDeclaration::Clone(StoragePool* ast_pool)
{
    AstAnnotationDeclaration* clone =
        ast_pool -> GenAnnotationDeclaration(interface_token);
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    clone -> class_body = (AstClassBody*) class_body -> Clone(ast_pool);
    clone -> class_body -> owner = clone;
    return clone;
}

Ast* AstLocalVariableStatement::Clone(StoragePool* ast_pool)
{
    AstLocalVariableStatement* clone = ast_pool -> GenLocalVariableStatement();
    if (modifiers_opt)
        clone -> modifiers_opt =
            (AstModifiers*) modifiers_opt -> Clone(ast_pool);
    clone -> type = (AstType*) type -> Clone(ast_pool);
    clone -> AllocateVariableDeclarators(NumVariableDeclarators());
    for (unsigned i = 0; i < NumVariableDeclarators(); i++)
        clone -> AddVariableDeclarator((AstVariableDeclarator*)
                                       VariableDeclarator(i) ->
                                       Clone(ast_pool));
    clone -> semicolon_token_opt = semicolon_token_opt;
    return clone;
}

Ast* AstLocalClassStatement::Clone(StoragePool* ast_pool)
{
    Ast* p = declaration -> Clone(ast_pool);
    if (p -> ClassDeclarationCast())
        return ast_pool -> GenLocalClassStatement((AstClassDeclaration*) p);
    else return ast_pool -> GenLocalClassStatement((AstEnumDeclaration*) p);
}

Ast* AstIfStatement::Clone(StoragePool* ast_pool)
{
    AstIfStatement* clone = ast_pool -> GenIfStatement();
    clone -> if_token = if_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> true_statement = (AstBlock*) true_statement -> Clone(ast_pool);
    if (false_statement_opt)
        clone -> false_statement_opt =
            (AstBlock*) false_statement_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstEmptyStatement::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenEmptyStatement(semicolon_token);
}

Ast* AstExpressionStatement::Clone(StoragePool* ast_pool)
{
    AstExpressionStatement* clone = ast_pool -> GenExpressionStatement();
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> semicolon_token_opt = semicolon_token_opt;
    return clone;
}

Ast* AstSwitchLabel::Clone(StoragePool* ast_pool)
{
    AstSwitchLabel* clone = ast_pool -> GenSwitchLabel();
    clone -> case_token = case_token;
    if (expression_opt)
        clone -> expression_opt =
            (AstExpression*) expression_opt -> Clone(ast_pool);
    clone -> colon_token = colon_token;
    clone -> map_index = map_index;
    return clone;
}

Ast* AstSwitchBlockStatement::Clone(StoragePool* ast_pool)
{
    AstSwitchBlockStatement* clone = ast_pool -> GenSwitchBlockStatement();
    clone -> CloneBlock(ast_pool, this);
    clone -> AllocateSwitchLabels(NumSwitchLabels());
    for (unsigned i = 0; i < NumSwitchLabels(); i++)
        clone -> AddSwitchLabel((AstSwitchLabel*) SwitchLabel(i) ->
                                Clone(ast_pool));
    return clone;
}

Ast* AstSwitchStatement::Clone(StoragePool* ast_pool)
{
    AstSwitchStatement* clone = ast_pool -> GenSwitchStatement();
    clone -> switch_token = switch_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> switch_block = (AstBlock*) switch_block -> Clone(ast_pool);
    clone -> AllocateCases(NumCases());
    if (DefaultCase())
    {
        clone -> DefaultCase() = ast_pool -> GenCaseElement(0, 0);
        *clone -> DefaultCase() = *DefaultCase();
    }
    for (unsigned i = 0; i < NumCases(); i++)
    {
        CaseElement* elt = ast_pool -> GenCaseElement(0, 0);
        *elt = *Case(i);
        clone -> AddCase(elt);
    }
    return clone;
}

Ast* AstWhileStatement::Clone(StoragePool* ast_pool)
{
    AstWhileStatement* clone = ast_pool -> GenWhileStatement();
    clone -> while_token = while_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> statement = (AstBlock*) statement -> Clone(ast_pool);
    return clone;
}

Ast* AstDoStatement::Clone(StoragePool* ast_pool)
{
    AstDoStatement* clone = ast_pool -> GenDoStatement();
    clone -> do_token = do_token;
    clone -> statement = (AstBlock*) statement -> Clone(ast_pool);
    clone -> while_token = while_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> semicolon_token = semicolon_token;
    return clone;
}

Ast* AstForStatement::Clone(StoragePool* ast_pool)
{
    unsigned i;
    AstForStatement* clone = ast_pool -> GenForStatement();
    clone -> for_token = for_token;
    clone -> AllocateForInitStatements(NumForInitStatements());
    for (i = 0; i < NumForInitStatements(); i++)
        clone -> AddForInitStatement((AstStatement*)
                                     ForInitStatement(i) -> Clone(ast_pool));
    if (end_expression_opt)
        clone -> end_expression_opt =
            (AstExpression*) end_expression_opt -> Clone(ast_pool);
    clone -> AllocateForUpdateStatements(NumForUpdateStatements());
    for (i = 0; i < NumForUpdateStatements(); i++)
        clone -> AddForUpdateStatement((AstExpressionStatement*)
                                       ForUpdateStatement(i) ->
                                       Clone(ast_pool));
    clone -> statement = (AstBlock*) statement -> Clone(ast_pool);
    return clone;
}

Ast* AstForeachStatement::Clone(StoragePool* ast_pool)
{
    AstForeachStatement* clone = ast_pool -> GenForeachStatement();
    clone -> for_token = for_token;
    clone -> formal_parameter =
        (AstFormalParameter*) formal_parameter -> Clone(ast_pool);
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> statement = (AstBlock*) statement -> Clone(ast_pool);
    return clone;
}

Ast* AstBreakStatement::Clone(StoragePool* ast_pool)
{
    AstBreakStatement* clone = ast_pool -> GenBreakStatement();
    clone -> break_token = break_token;
    clone -> identifier_token_opt = identifier_token_opt;
    clone -> semicolon_token = semicolon_token;
    clone -> nesting_level = nesting_level;
    return clone;
}

Ast* AstContinueStatement::Clone(StoragePool* ast_pool)
{
    AstContinueStatement* clone = ast_pool -> GenContinueStatement();
    clone -> continue_token = continue_token;
    clone -> identifier_token_opt = identifier_token_opt;
    clone -> semicolon_token = semicolon_token;
    clone -> nesting_level = nesting_level;
    return clone;
}

Ast* AstReturnStatement::Clone(StoragePool* ast_pool)
{
    AstReturnStatement* clone = ast_pool -> GenReturnStatement();
    clone -> return_token = return_token;
    if (expression_opt)
        clone -> expression_opt =
            (AstExpression*) expression_opt -> Clone(ast_pool);
    clone -> semicolon_token = semicolon_token;
    return clone;
}

Ast* AstThrowStatement::Clone(StoragePool* ast_pool)
{
    AstThrowStatement* clone = ast_pool -> GenThrowStatement();
    clone -> throw_token = throw_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> semicolon_token = semicolon_token;
    return clone;
}

Ast* AstSynchronizedStatement::Clone(StoragePool* ast_pool)
{
    AstSynchronizedStatement* clone = ast_pool -> GenSynchronizedStatement();
    clone -> synchronized_token = synchronized_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> block = (AstBlock*) block -> Clone(ast_pool);
    return clone;
}

Ast* AstAssertStatement::Clone(StoragePool* ast_pool)
{
    AstAssertStatement* clone = ast_pool -> GenAssertStatement();
    clone -> assert_token = assert_token;
    clone -> condition = (AstExpression*) condition -> Clone(ast_pool);
    if (message_opt)
        clone -> message_opt = (AstExpression*) message_opt -> Clone(ast_pool);
    clone -> semicolon_token = semicolon_token;
    return clone;
}

Ast* AstCatchClause::Clone(StoragePool* ast_pool)
{
    AstCatchClause* clone = ast_pool -> GenCatchClause();
    clone -> catch_token = catch_token;
    clone -> formal_parameter =
        (AstFormalParameter*) formal_parameter -> Clone(ast_pool);
    clone -> block = (AstBlock*) block -> Clone(ast_pool);
    return clone;
}

Ast* AstFinallyClause::Clone(StoragePool* ast_pool)
{
    AstFinallyClause* clone = ast_pool -> GenFinallyClause();
    clone -> finally_token = finally_token;
    clone -> block = (AstBlock*) block -> Clone(ast_pool);
    return clone;
}

Ast* AstTryStatement::Clone(StoragePool* ast_pool)
{
    AstTryStatement* clone = ast_pool -> GenTryStatement();
    clone -> try_token = try_token;
    clone -> block = (AstBlock*) block -> Clone(ast_pool);
    clone -> AllocateCatchClauses(NumCatchClauses());
    for (unsigned i = 0; i < NumCatchClauses(); i++)
        clone -> AddCatchClause((AstCatchClause*) CatchClause(i) ->
                                Clone(ast_pool));
    if (finally_clause_opt)
        clone -> finally_clause_opt =
            (AstFinallyClause*) finally_clause_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstIntegerLiteral::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenIntegerLiteral(integer_literal_token);
}

Ast* AstLongLiteral::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenLongLiteral(long_literal_token);
}

Ast* AstFloatLiteral::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenFloatLiteral(float_literal_token);
}

Ast* AstDoubleLiteral::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenDoubleLiteral(double_literal_token);
}

Ast* AstTrueLiteral::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenTrueLiteral(true_literal_token);
}

Ast* AstFalseLiteral::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenFalseLiteral(false_literal_token);
}

Ast* AstStringLiteral::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenStringLiteral(string_literal_token);
}

Ast* AstCharacterLiteral::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenCharacterLiteral(character_literal_token);
}

Ast* AstNullLiteral::Clone(StoragePool* ast_pool)
{
    return ast_pool -> GenNullLiteral(null_token);
}

Ast* AstClassLiteral::Clone(StoragePool* ast_pool)
{
    AstClassLiteral* clone = ast_pool -> GenClassLiteral(class_token);
    clone -> type = (AstTypeName*) type -> Clone(ast_pool);
    if (resolution_opt)
        clone -> resolution_opt =
            (AstExpression*) resolution_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstThisExpression::Clone(StoragePool* ast_pool)
{
    AstThisExpression* clone = ast_pool -> GenThisExpression(this_token);
    if (base_opt)
        clone -> base_opt = (AstTypeName*) base_opt -> Clone(ast_pool);
    if (resolution_opt)
        clone -> resolution_opt =
            (AstExpression*) resolution_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstSuperExpression::Clone(StoragePool* ast_pool)
{
    AstSuperExpression* clone = ast_pool -> GenSuperExpression(super_token);
    if (base_opt)
        clone -> base_opt = (AstTypeName*) base_opt -> Clone(ast_pool);
    if (resolution_opt)
        clone -> resolution_opt =
            (AstExpression*) resolution_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstParenthesizedExpression::Clone(StoragePool* ast_pool)
{
    AstParenthesizedExpression* clone =
        ast_pool -> GenParenthesizedExpression();
    clone -> left_parenthesis_token = left_parenthesis_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> right_parenthesis_token = right_parenthesis_token;
    return clone;
}

Ast* AstClassCreationExpression::Clone(StoragePool* ast_pool)
{
    AstClassCreationExpression* clone =
        ast_pool -> GenClassCreationExpression();
    if (base_opt)
        clone -> base_opt = (AstExpression*) base_opt -> Clone(ast_pool);
    clone -> new_token = new_token;
    if (type_arguments_opt)
        clone -> type_arguments_opt =
            (AstTypeArguments*) type_arguments_opt -> Clone(ast_pool);
    clone -> class_type = (AstTypeName*) class_type -> Clone(ast_pool);
    clone -> arguments = (AstArguments*) arguments -> Clone(ast_pool);
    if (class_body_opt)
        clone -> class_body_opt =
            (AstClassBody*) class_body_opt -> Clone(ast_pool);
    if (resolution_opt)
        clone -> resolution_opt =
            (AstClassCreationExpression*) resolution_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstDimExpr::Clone(StoragePool* ast_pool)
{
    AstDimExpr* clone = ast_pool -> GenDimExpr();
    clone -> left_bracket_token = left_bracket_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> right_bracket_token = right_bracket_token;
    return clone;
}

Ast* AstArrayCreationExpression::Clone(StoragePool* ast_pool)
{
    AstArrayCreationExpression* clone =
        ast_pool -> GenArrayCreationExpression();
    clone -> new_token = new_token;
    clone -> array_type = (AstType*) array_type -> Clone(ast_pool);
    clone -> AllocateDimExprs(NumDimExprs());
    for (unsigned i = 0; i < NumDimExprs(); i++)
        clone -> AddDimExpr((AstDimExpr*) DimExpr(i) -> Clone(ast_pool));
    if (brackets_opt)
        clone -> brackets_opt = (AstBrackets*) brackets_opt -> Clone(ast_pool);
    if (array_initializer_opt)
        clone -> array_initializer_opt =
            (AstArrayInitializer*) array_initializer_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstFieldAccess::Clone(StoragePool* ast_pool)
{
    AstFieldAccess* clone = ast_pool -> GenFieldAccess();
    clone -> base = (AstExpression*) base -> Clone(ast_pool);
    clone -> identifier_token = identifier_token;
    if (resolution_opt)
        clone -> resolution_opt =
            (AstExpression*) resolution_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstMethodInvocation::Clone(StoragePool* ast_pool)
{
    AstMethodInvocation* clone =
        ast_pool -> GenMethodInvocation(identifier_token);
    if (base_opt)
        clone -> base_opt = (AstExpression*) base_opt -> Clone(ast_pool);
    if (type_arguments_opt)
        clone -> type_arguments_opt =
            (AstTypeArguments*) type_arguments_opt -> Clone(ast_pool);
    clone -> identifier_token = identifier_token;
    clone -> arguments = (AstArguments*) arguments -> Clone(ast_pool);
    if (resolution_opt)
        clone -> resolution_opt =
            (AstExpression*) resolution_opt -> Clone(ast_pool);
    return clone;
}

Ast* AstArrayAccess::Clone(StoragePool* ast_pool)
{
    AstArrayAccess* clone = ast_pool -> GenArrayAccess();
    clone -> base = (AstExpression*) base -> Clone(ast_pool);
    clone -> left_bracket_token = left_bracket_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> right_bracket_token = right_bracket_token;
    return clone;
}

Ast* AstPostUnaryExpression::Clone(StoragePool* ast_pool)
{
    AstPostUnaryExpression* clone =
        ast_pool -> GenPostUnaryExpression((PostUnaryExpressionTag) other_tag);
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> post_operator_token = post_operator_token;
    return clone;
}

Ast* AstPreUnaryExpression::Clone(StoragePool* ast_pool)
{
    AstPreUnaryExpression* clone =
        ast_pool -> GenPreUnaryExpression((PreUnaryExpressionTag) other_tag);
    clone -> pre_operator_token = pre_operator_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    return clone;
}

Ast* AstCastExpression::Clone(StoragePool* ast_pool)
{
    AstCastExpression* clone = ast_pool -> GenCastExpression();
    clone -> left_parenthesis_token = left_parenthesis_token;
    if (type)
        clone -> type = (AstType*) type -> Clone(ast_pool);
    clone -> right_parenthesis_token = right_parenthesis_token;
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    return clone;
}

Ast* AstBinaryExpression::Clone(StoragePool* ast_pool)
{
    AstBinaryExpression* clone =
        ast_pool -> GenBinaryExpression((BinaryExpressionTag) other_tag);
    clone -> left_expression =
        (AstExpression*) left_expression -> Clone(ast_pool);
    clone -> binary_operator_token = binary_operator_token;
    clone -> right_expression =
        (AstExpression*) right_expression -> Clone(ast_pool);
    return clone;
}

Ast* AstInstanceofExpression::Clone(StoragePool* ast_pool)
{
    AstInstanceofExpression* clone = ast_pool -> GenInstanceofExpression();
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    clone -> instanceof_token = instanceof_token;
    clone -> type = (AstType*) type -> Clone(ast_pool);
    return clone;
}

Ast* AstConditionalExpression::Clone(StoragePool* ast_pool)
{
    AstConditionalExpression* clone = ast_pool -> GenConditionalExpression();
    clone -> test_expression =
        (AstExpression*) test_expression -> Clone(ast_pool);
    clone -> question_token = question_token;
    clone -> true_expression =
        (AstExpression*) true_expression -> Clone(ast_pool);
    clone -> colon_token = colon_token;
    clone -> false_expression =
        (AstExpression*) false_expression -> Clone(ast_pool);
    return clone;
}

Ast* AstAssignmentExpression::Clone(StoragePool* ast_pool)
{
    AstAssignmentExpression* clone = ast_pool ->
        GenAssignmentExpression((AssignmentExpressionTag) other_tag,
                                assignment_operator_token);
    clone -> left_hand_side =
        (AstExpression*) left_hand_side -> Clone(ast_pool);
    clone -> expression = (AstExpression*) expression -> Clone(ast_pool);
    return clone;
}


#ifdef JOPA_DEBUG

//
// These methods allow printing the Ast structure to Coutput (usually stdout).
//
void AstBlock::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (";
    if (label_opt)
        Coutput << lex_stream.NameString(label_opt) << ": ";
    Coutput << "Block at level " << nesting_level;
    if (block_symbol)
        Coutput << ", max_variable_index "
                << block_symbol -> max_variable_index
                << ", helper_variable_index "
                << block_symbol -> helper_variable_index;
    else Coutput << ", BLOCK_SYMBOL NOT SET";
    Coutput << ')';

    if (NumStatements() > 0)
    {
        Coutput << "    {";
        for (i = 0; i < NumStatements(); i++)
        {
            if (i % 10 == 0)
                Coutput << endl << "        ";
            Coutput << " #" << Statement(i) -> id;
        }
        Coutput << "    }" << endl;
        for (i = 0; i < NumStatements(); i++)
            Statement(i) -> Print(lex_stream);
    }
    else Coutput << endl;
}

void AstName::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (Name):  #"
            << (base_opt ? base_opt -> id : 0) << '.'
            << lex_stream.NameString(identifier_token) << endl;
    if (base_opt)
        base_opt -> Print(lex_stream);
}

void AstPrimitiveType::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (PrimitiveType):  "
            << lex_stream.NameString(primitive_kind_token) << endl;
}

void AstBrackets::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (Brackets, dims=" << dims << "):  ";
    for (TokenIndex i = left_bracket_token; i <= right_bracket_token; i++)
        Coutput << lex_stream.NameString(i);
    Coutput << endl;
}

void AstArrayType::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ArrayType):  "
            << '#' << type -> id << ' ' << brackets -> id << endl;
    type -> Print(lex_stream);
    brackets -> Print(lex_stream);
}

void AstWildcard::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (Wildcard):  "
            << lex_stream.NameString(question_token);
    if (extends_token_opt)
        Coutput << ' ' << lex_stream.NameString(extends_token_opt) << " #"
                << bounds_opt -> id;
    else if (super_token_opt)
        Coutput << ' ' << lex_stream.NameString(super_token_opt) << " #"
                << bounds_opt -> id;
    Coutput << endl;
    if (bounds_opt)
        bounds_opt -> Print(lex_stream);
}

void AstTypeArguments::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (TypeArguments):  <";
    for (i = 0; i < NumTypeArguments(); i++)
        Coutput << " #" << TypeArgument(i) -> id;
    Coutput << '>' << endl;
    for (i = 0; i < NumTypeArguments(); i++)
        TypeArgument(i) -> Print(lex_stream);
}

void AstTypeName::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (TypeName):  #"
            << (base_opt ? base_opt -> id : 0) << ".#" << name -> id << "<#"
            << (type_arguments_opt ? type_arguments_opt -> id : 0) << '>'
            << endl;
    if (base_opt)
        base_opt -> Print(lex_stream);
    name -> Print(lex_stream);
    if (type_arguments_opt)
        type_arguments_opt -> Print(lex_stream);
}

void AstMemberValuePair::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (MemberValuePair):  "
            << (identifier_token_opt
                ? lex_stream.NameString(identifier_token_opt) : L"(value)")
            << "=#" << member_value -> id << endl;
    member_value -> Print(lex_stream);
}

void AstAnnotation::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (Annotation):  #" << name -> id << '(';
    for (i = 0; i < NumMemberValuePairs(); i++)
    {
        if (i % 10 == 0)
            Coutput << endl << "       ";
        Coutput << " #" << MemberValuePair(i) -> id;
    }
    Coutput << ')' << endl;
    name -> Print(lex_stream);
    for (i = 0; i < NumMemberValuePairs(); i++)
        MemberValuePair(i) -> Print(lex_stream);
}

void AstModifierKeyword::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ModifierKeyword):  "
            << lex_stream.NameString(modifier_token) << endl;
}

void AstModifiers::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (Modifiers): ";
    for (i = 0; i < NumModifiers(); i++)
        Coutput << " #" << Modifier(i) -> id;
    Coutput << endl;
    for (i = 0; i < NumModifiers(); i++)
        Modifier(i) -> Print(lex_stream);
}

void AstPackageDeclaration::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (PackageDeclaration):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0) << ' '
            << lex_stream.NameString(package_token)
            << " #" << name -> id << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    name -> Print(lex_stream);
}

void AstImportDeclaration::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ImportDeclaration):  ";
    if (static_token_opt)
        Coutput << lex_stream.NameString(static_token_opt) << ' ';
    Coutput << lex_stream.NameString(import_token)
            << " #" << name -> id;
    if (star_token_opt)
        Coutput << '.' << lex_stream.NameString(star_token_opt);
    Coutput << endl;
    name -> Print(lex_stream);
}

void AstCompilationUnit::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << endl << "AST structure for "
            << lex_stream.FileName()
            << ':' << endl << endl
            << '#' << id << " (CompilationUnit):  #"
            << (package_declaration_opt ? package_declaration_opt -> id : 0)
            << " (";
    for (i = 0; i < NumImportDeclarations(); i++)
        Coutput << " #" << ImportDeclaration(i) -> id;
    Coutput << " ) (";
    for (i = 0; i < NumTypeDeclarations(); i++)
        Coutput << " #" << TypeDeclaration(i) -> id;
    Coutput << ')' << endl;

    if (package_declaration_opt)
        package_declaration_opt -> Print(lex_stream);
    for (i = 0; i < NumImportDeclarations(); i++)
        ImportDeclaration(i) -> Print(lex_stream);
    for (i = 0; i < NumTypeDeclarations(); i++)
        TypeDeclaration(i) -> Print(lex_stream);
}

void AstEmptyDeclaration::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (EmptyDeclaration):  "
            << lex_stream.NameString(semicolon_token) << endl;
}

void AstClassBody::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (ClassBody):  "
            << endl << "    {";
    for (i = 0; i < NumClassBodyDeclarations(); i++)
    {
        if (i % 10 == 0)
            Coutput << endl << "       ";
        Coutput << " #" << ClassBodyDeclaration(i) -> id;
    }
    Coutput << endl << "    }" << endl;

    for (i = 0; i < NumClassBodyDeclarations(); i++)
        ClassBodyDeclaration(i) -> Print(lex_stream);
}

void AstTypeParameter::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (TypeParameter):  "
            << lex_stream.NameString(identifier_token) << " (";
    for (i = 0; i < NumBounds(); i++)
        Coutput << " #" << Bound(i) -> id;
    Coutput << ')' << endl;
    for (i = 0; i < NumBounds(); i++)
        Bound(i) -> Print(lex_stream);
}

void AstTypeParameters::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (TypeParameters): <";
    for (i = 0; i < NumTypeParameters(); i++)
        Coutput << " #" << TypeParameter(i) -> id;
    Coutput << '>' << endl;
    for (i = 0; i < NumTypeParameters(); i++)
        TypeParameter(i) -> Print(lex_stream);
}

void AstClassDeclaration::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (ClassDeclaration):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0) << ' '
            << lex_stream.NameString(class_token) << ' '
            << lex_stream.NameString(class_body -> identifier_token) << " #"
            << (type_parameters_opt ? type_parameters_opt -> id : 0)
            << " #" << (super_opt ? super_opt -> id : 0) << '(';
    for (i = 0; i < NumInterfaces(); i++)
        Coutput << " #" << Interface(i) -> id;
    Coutput << ") #" << class_body -> id << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    if (type_parameters_opt)
        type_parameters_opt -> Print(lex_stream);
    if (super_opt)
        super_opt -> Print(lex_stream);
    for (i = 0; i < NumInterfaces(); i++)
        Interface(i) -> Print(lex_stream);
    class_body -> Print(lex_stream);
}

void AstArrayInitializer::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (ArrayInitializer):  "
            << endl << "    {";
    for (i = 0; i < NumVariableInitializers(); i++)
    {
        if (i % 10 == 0)
            Coutput << endl << "       ";
        Coutput << " #" << VariableInitializer(i) -> id;
    }
    Coutput << endl << "    }" << endl;

    for (i = 0; i < NumVariableInitializers(); i++)
        VariableInitializer(i) -> Print(lex_stream);
}

void AstVariableDeclaratorId::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (VariableDeclaratorId):  "
            << lex_stream.NameString(identifier_token) << " #"
            << (brackets_opt ? brackets_opt -> id : 0) << endl;
    if (brackets_opt)
        brackets_opt -> Print(lex_stream);
}

void AstVariableDeclarator::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (VariableDeclarator):  " << '#'
            << variable_declarator_name -> id << " #"
            << (variable_initializer_opt ? variable_initializer_opt -> id : 0)
            << endl;
    variable_declarator_name -> Print(lex_stream);
    if (variable_initializer_opt)
        variable_initializer_opt -> Print(lex_stream);

}

void AstFieldDeclaration::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (FieldDeclaration):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0)
            << " #" << type -> id << '(';
    for (i = 0; i < NumVariableDeclarators(); i++)
        Coutput << " #" << VariableDeclarator(i) -> id;
    Coutput << ')' << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    type -> Print(lex_stream);
    for (i = 0; i < NumVariableDeclarators(); i++)
        VariableDeclarator(i) -> Print(lex_stream);
}

void AstFormalParameter::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (FormalParameter):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0)
            << " #" << type -> id << " #" << formal_declarator -> id << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    type -> Print(lex_stream);
    if (ellipsis_token_opt)
        Coutput << lex_stream.NameString(ellipsis_token_opt);
    formal_declarator -> Print(lex_stream);
}

void AstMethodDeclarator::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (MethodDeclarator):  "
            << lex_stream.NameString(identifier_token)
            << " (";
    for (i = 0; i < NumFormalParameters(); i++)
        Coutput << " #" << FormalParameter(i) -> id;
    Coutput << " ) #" << (brackets_opt ? brackets_opt -> id : 0) << endl;
    for (i = 0; i < NumFormalParameters(); i++)
        FormalParameter(i) -> Print(lex_stream);
    if (brackets_opt)
        brackets_opt -> Print(lex_stream);
}

void AstMethodBody::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (MethodBody):  ";
    if (explicit_constructor_opt)
        Coutput << " #" << explicit_constructor_opt -> id << endl;
    else Coutput << " #0" << endl;
    AstBlock::Print(lex_stream);

    if (explicit_constructor_opt)
        explicit_constructor_opt -> Print(lex_stream);
}

void AstMethodDeclaration::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (MethodDeclaration):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0) << " <#"
            << (type_parameters_opt ? type_parameters_opt -> id : 0)
            << "> #" << type -> id << " #" << method_declarator -> id
            << " throws: (";
    for (i = 0; i < NumThrows(); i++)
        Coutput << " #" << Throw(i) -> id;
    Coutput << ") default #"
            << (default_value_opt ? default_value_opt -> id : 0) << ' '
            << (method_body_opt ? method_body_opt -> id : 0) << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    if (type_parameters_opt)
        type_parameters_opt -> Print(lex_stream);
    type -> Print(lex_stream);
    method_declarator -> Print(lex_stream);
    for (i = 0; i < NumThrows(); i++)
        Throw(i) -> Print(lex_stream);
    if (default_value_opt)
        default_value_opt -> Print(lex_stream);
    if (method_body_opt)
        method_body_opt -> Print(lex_stream);
}

void AstInitializerDeclaration::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (InitializerDeclaration):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0)
            << " #" << block -> id << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    block -> Print(lex_stream);
}

void AstArguments::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (Arguments):  (";
    for (i = 0; i < NumArguments(); i++)
        Coutput << " #" << Argument(i) -> id;
    Coutput << ')' << endl;
    for (i = 0; i < NumArguments(); i++)
        Argument(i) -> Print(lex_stream);
}

void AstThisCall::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ThisCall):  #"
            << (type_arguments_opt ? type_arguments_opt -> id : 0)
            << lex_stream.NameString(this_token) << " #" << arguments -> id
            << endl;
    if (type_arguments_opt)
        type_arguments_opt -> Print(lex_stream);
    arguments -> Print(lex_stream);
}

void AstSuperCall::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (SuperCall):  #"
            << (base_opt ? base_opt -> id : 0) << ".#"
            << (type_arguments_opt ? type_arguments_opt -> id : 0)
            << lex_stream.NameString(super_token) << " #" << arguments -> id
            << endl;
    if (base_opt)
        base_opt -> Print(lex_stream);
    if (type_arguments_opt)
        type_arguments_opt -> Print(lex_stream);
    arguments -> Print(lex_stream);
}

void AstConstructorDeclaration::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (ConstructorDeclaration):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0) << " <#"
            << (type_parameters_opt ? type_parameters_opt -> id : 0)
            << " #" << constructor_declarator -> id << " throws: (";
    for (i = 0; i < NumThrows(); i++)
        Coutput << " #" << Throw(i) -> id;
    Coutput << ") #" << constructor_body -> id << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    if (type_parameters_opt)
        type_parameters_opt -> Print(lex_stream);
    constructor_declarator -> Print(lex_stream);
    for (i = 0; i < NumThrows(); i++)
        Throw(i) -> Print(lex_stream);
    constructor_body -> Print(lex_stream);
}

void AstEnumDeclaration::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (EnumDeclaration):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0) << ' '
            << lex_stream.NameString(enum_token) << ' '
            << lex_stream.NameString(class_body -> identifier_token) << " (";
    for (i = 0; i < NumInterfaces(); i++)
        Coutput << " #" << Interface(i) -> id;
    Coutput << ") {";
    for (i = 0; i < NumEnumConstants(); i++)
        Coutput << " #" << EnumConstant(i) -> id;
    Coutput << "} #" << class_body -> id << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    for (i = 0; i < NumInterfaces(); i++)
        Interface(i) -> Print(lex_stream);
    for (i = 0; i < NumEnumConstants(); i++)
        EnumConstant(i) -> Print(lex_stream);
    class_body -> Print(lex_stream);
}

void AstEnumConstant::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (EnumConstant):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0) << ' '
            << lex_stream.NameString(identifier_token) << " #"
            << (arguments_opt ? arguments_opt -> id : 0) << " #"
            << (class_body_opt ? class_body_opt -> id : 0) << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    if (arguments_opt)
        arguments_opt -> Print(lex_stream);
    if (class_body_opt)
        class_body_opt -> Print(lex_stream);
}

void AstInterfaceDeclaration::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (InterfaceDeclaration):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0) << ' '
            << lex_stream.NameString(interface_token) << ' '
            << lex_stream.NameString(class_body -> identifier_token) << " #"
            << (type_parameters_opt ? type_parameters_opt -> id : 0) << " (";
    for (i = 0; i < NumInterfaces(); i++)
        Coutput << " #" << Interface(i) -> id;
    Coutput << ") #" << class_body -> id << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    if (type_parameters_opt)
        type_parameters_opt -> Print(lex_stream);
    for (i = 0; i < NumInterfaces(); i++)
        Interface(i) -> Print(lex_stream);
    class_body -> Print(lex_stream);
}

void AstAnnotationDeclaration::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (AnnotationDeclaration):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0) << " @"
            << lex_stream.NameString(interface_token) << ' '
            << lex_stream.NameString(class_body -> identifier_token) << " #"
            << class_body -> id << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    class_body -> Print(lex_stream);
}

void AstLocalVariableStatement::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (LocalVariableStatement):  #"
            << (modifiers_opt ? modifiers_opt -> id : 0)
            << " #" << type -> id << '(';
    for (i = 0; i < NumVariableDeclarators(); i++)
        Coutput << " #" << VariableDeclarator(i) -> id;
    Coutput << ')' << endl;
    if (modifiers_opt)
        modifiers_opt -> Print(lex_stream);
    type -> Print(lex_stream);
    for (i = 0; i < NumVariableDeclarators(); i++)
        VariableDeclarator(i) -> Print(lex_stream);
}

void AstLocalClassStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (LocalClassStatement): #"
            << declaration -> id << endl;
    declaration -> Print(lex_stream);
}

void AstIfStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (IfStatement):  "
            << lex_stream.NameString(if_token)
            << " ( #" << expression -> id << " ) #" << true_statement -> id;
    if (false_statement_opt)
        Coutput << " else #" << false_statement_opt -> id;
    else Coutput << " #0";
    Coutput << endl;

    expression -> Print(lex_stream);
    true_statement -> Print(lex_stream);
    if (false_statement_opt)
        false_statement_opt -> Print(lex_stream);
}

void AstEmptyStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (EmptyStatement):  "
            << lex_stream.NameString(semicolon_token)
            << endl;
}

void AstExpressionStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ExpressionStatement):  #" << expression -> id
            << endl;
    expression -> Print(lex_stream);
}

void AstSwitchLabel::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (SwitchLabel, map_index " << map_index << "):  "
            << lex_stream.NameString(case_token) << '#'
            << (expression_opt ? expression_opt -> id : 0) << ':' << endl;
    if (expression_opt)
        expression_opt -> Print(lex_stream);
}

void AstSwitchBlockStatement::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (SwitchBlockStatement): ";
    for (i = 0; i < NumSwitchLabels(); i++)
    {
        if (i % 10 == 0)
            Coutput << endl << "        ";
        Coutput << " #" << SwitchLabel(i) -> id << ':';
    }
    Coutput << endl;
    for (i = 0; i < NumSwitchLabels(); i++)
        SwitchLabel(i) -> Print(lex_stream);
    AstBlock::Print(lex_stream);
}

void AstSwitchStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (SwitchStatement):  "
            << lex_stream.NameString(switch_token)
            << " ( #" << expression -> id << " ) #" << switch_block -> id
            << endl;
    for (unsigned i = 0; i <= num_cases; i++)
    {
        Coutput << " case index: " << i;
        if (cases[i])
            Coutput << "  block: " << cases[i] -> block_index
                    << "  label: " << cases[i] -> case_index
                    << "  value: " << cases[i] -> value << endl;
        else Coutput << "(none)" << endl;
    }
    expression -> Print(lex_stream);
    switch_block -> Print(lex_stream);
}

void AstWhileStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (WhileStatement):  "
            << lex_stream.NameString(while_token)
            << " ( #" << expression -> id << " ) #" << statement -> id << endl;
    expression -> Print(lex_stream);
    statement -> Print(lex_stream);
}

void AstDoStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (DoStatement):  "
            << lex_stream.NameString(do_token)
            << " { #" << statement -> id << " } "
            << lex_stream.NameString(while_token)
            << " ( #" << expression -> id << " ) #" << endl;

    statement -> Print(lex_stream);
    expression -> Print(lex_stream);
}

void AstForStatement::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (ForStatement):  ("
            << lex_stream.NameString(for_token);
    for (i = 0; i < NumForInitStatements(); i++)
        Coutput << " #" << ForInitStatement(i) -> id;
    Coutput << "; #" << (end_expression_opt ? end_expression_opt -> id : 0)
            << ';';
    for (i = 0; i < NumForUpdateStatements(); i++)
        Coutput << " #" << ForUpdateStatement(i) -> id;
    Coutput << ") #" << statement -> id << endl;

    for (i = 0; i < NumForInitStatements(); i++)
        ForInitStatement(i) -> Print(lex_stream);
    if (end_expression_opt)
        end_expression_opt -> Print(lex_stream);
    for (i = 0; i < NumForUpdateStatements(); i++)
        ForUpdateStatement(i) -> Print(lex_stream);
    statement -> Print(lex_stream);
}

void AstForeachStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ForeachStatement):  ("
            << lex_stream.NameString(for_token) << "( #"
            << formal_parameter -> id << ": #" << expression -> id
            << ") #" << statement -> id << endl;
    formal_parameter -> Print(lex_stream);
    expression -> Print(lex_stream);
    statement -> Print(lex_stream);
}

void AstBreakStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (BreakStatement):  "
            << lex_stream.NameString(break_token) << ' '
            << (identifier_token_opt
                ? lex_stream.NameString(identifier_token_opt) : L"")
            << " at nesting_level " << nesting_level << endl;
}

void AstContinueStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ContinueStatement):  "
            << lex_stream.NameString(continue_token) << ' '
            << (identifier_token_opt
                ? lex_stream.NameString(identifier_token_opt) : L"")
            << " at nesting_level " << nesting_level << endl;
}

void AstReturnStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ReturnStatement):  "
            << lex_stream.NameString(return_token)
            << ' '
            << " #" << (expression_opt ? expression_opt -> id : 0) << endl;
    if (expression_opt)
        expression_opt -> Print(lex_stream);
}

void AstThrowStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ThrowStatement):  "
            << lex_stream.NameString(throw_token)
            << ' '
            << " #" << expression -> id << endl;
    expression -> Print(lex_stream);
}

void AstSynchronizedStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (SynchronizedStatement):  "
            << lex_stream.NameString(synchronized_token)
            << " ( #" << expression -> id
            << " ) #" << block -> id << endl;
    expression -> Print(lex_stream);
    block -> Print(lex_stream);
}

void AstAssertStatement::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (AssertStatement):  "
            << lex_stream.NameString(assert_token)
            << " ( #" << condition -> id;
    if (message_opt)
        Coutput << " : " << message_opt -> id;
    else Coutput << " #0";
    Coutput << " ;" << endl;
    condition -> Print(lex_stream);
    if (message_opt)
        message_opt -> Print(lex_stream);
}

void AstCatchClause::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (CatchClause):  "
            << lex_stream.NameString(catch_token)
            << " #" << formal_parameter -> id
            << " #" << block -> id << endl;
    formal_parameter -> Print(lex_stream);
    block -> Print(lex_stream);
}

void AstFinallyClause::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (FinallyClause):  "
            << lex_stream.NameString(finally_token)
            << " #" << block -> id << endl;
    block -> Print(lex_stream);
}

void AstTryStatement::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (TryStatement):  "
            << lex_stream.NameString(try_token)
            << " #" << block -> id
            << " catch (";
    for (i = 0; i < NumCatchClauses(); i++)
        Coutput << " #" << CatchClause(i) -> id;
    Coutput << ") finally #"
            << (finally_clause_opt ? finally_clause_opt -> id : 0) << endl;

    block -> Print(lex_stream);
    for (i = 0; i < NumCatchClauses(); i++)
        CatchClause(i) -> Print(lex_stream);
    if (finally_clause_opt)
        finally_clause_opt -> Print(lex_stream);
}

void AstIntegerLiteral::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (IntegerLiteral):  "
            << lex_stream.NameString(integer_literal_token)
            << endl;
}

void AstLongLiteral::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (LongLiteral):  "
            << lex_stream.NameString(long_literal_token)
            << endl;
}

void AstFloatLiteral::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (FloatLiteral):  "
            << lex_stream.NameString(float_literal_token)
            << endl;
}

void AstDoubleLiteral::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (DoubleLiteral):  "
            << lex_stream.NameString(double_literal_token)
            << endl;
}

void AstTrueLiteral::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (TrueLiteral):  "
            << lex_stream.NameString(true_literal_token)
            << endl;
}

void AstFalseLiteral::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (FalseLiteral):  "
            << lex_stream.NameString(false_literal_token)
            << endl;
}

void AstStringLiteral::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (StringLiteral):  "
            << lex_stream.NameString(string_literal_token)
            << endl;
}

void AstCharacterLiteral::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (CharacterLiteral):  "
            << lex_stream.NameString(character_literal_token)
            << endl;
}

void AstNullLiteral::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (NullLiteral):  "
            << lex_stream.NameString(null_token)
            << endl;
}

void AstClassLiteral::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ClassLiteral):  #" << type -> id << ". "
            << lex_stream.NameString(class_token) << endl;
    type -> Print(lex_stream);
}

void AstThisExpression::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ThisExpression):  ";
    if (base_opt)
        Coutput << '#' << base_opt -> id << ". ";
    Coutput << lex_stream.NameString(this_token) << endl;
    if (base_opt)
        base_opt -> Print(lex_stream);
}

void AstSuperExpression::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (SuperExpression):  ";
    if (base_opt)
        Coutput << '#' << base_opt -> id << ". ";
    Coutput << lex_stream.NameString(super_token) << endl;
    if (base_opt)
        base_opt -> Print(lex_stream);
}

void AstParenthesizedExpression::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ParenthesizedExpression):  "
            << lex_stream.NameString(left_parenthesis_token)
            << '#' << expression -> id
            << lex_stream.NameString(right_parenthesis_token)
            << endl;
    expression -> Print(lex_stream);
}

void AstClassCreationExpression::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ClassCreationExpression):  #"
            << (base_opt ? base_opt -> id : 0) << ' '
            << lex_stream.NameString(new_token) << " #"
            << (type_arguments_opt ? type_arguments_opt -> id : 0) << " #"
            << class_type -> id << " #" << arguments -> id << " #"
            << (class_body_opt ? class_body_opt -> id : 0) << endl;
    if (base_opt)
        base_opt -> Print(lex_stream);
    if (type_arguments_opt)
        type_arguments_opt -> Print(lex_stream);
    class_type -> Print(lex_stream);
    arguments -> Print(lex_stream);
    if (class_body_opt)
        class_body_opt -> Print(lex_stream);
}

void AstDimExpr::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (DimExpr):  [ #" << expression -> id << " ]"
            << endl;
    expression -> Print(lex_stream);
}

void AstArrayCreationExpression::Print(LexStream& lex_stream)
{
    unsigned i;
    Coutput << '#' << id << " (ArrayCreationExpression):  "
            << lex_stream.NameString(new_token)
            << " #" << array_type -> id << "dimexpr:( ";
    for (i = 0; i < NumDimExprs(); i++)
        Coutput << " #" << DimExpr(i) -> id;
    Coutput << ") brackets:#" << (brackets_opt ? brackets_opt -> id : 0)
            << " initializer:#"
            << (array_initializer_opt ? array_initializer_opt -> id : 0)
            << endl;
    array_type -> Print(lex_stream);
    for (i = 0; i < NumDimExprs(); i++)
        DimExpr(i) -> Print(lex_stream);
    if (brackets_opt)
        brackets_opt -> Print(lex_stream);
    if (array_initializer_opt)
        array_initializer_opt -> Print(lex_stream);
}

void AstFieldAccess::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (FieldAccess):  "
            << " #" << base -> id << ' '
            << lex_stream.NameString(identifier_token)
            << endl;

    base -> Print(lex_stream);
}

void AstMethodInvocation::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (MethodInvocation):  #"
            << (base_opt ? base_opt -> id : 0) << ".#"
            << (type_arguments_opt ? type_arguments_opt -> id : 0) << ' '
            << lex_stream.NameString(identifier_token)
            << " #" << arguments -> id << endl;
    if (base_opt)
        base_opt -> Print(lex_stream);
    if (type_arguments_opt)
        type_arguments_opt -> Print(lex_stream);
    arguments -> Print(lex_stream);
}

void AstArrayAccess::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ArrayAccess):  "
            << '#' << base -> id
            << " [ #" << expression -> id << " ]" << endl;

    base -> Print(lex_stream);
    expression -> Print(lex_stream);
}

void AstPostUnaryExpression::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (PostUnaryExpression):  "
            << '#' << expression -> id
            << lex_stream.NameString(post_operator_token)
            << endl;

    expression -> Print(lex_stream);
}

void AstPreUnaryExpression::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (PreUnaryExpression):  "
            << lex_stream.NameString(pre_operator_token)
            << " #" << expression -> id << endl;

    expression -> Print(lex_stream);
}

void AstCastExpression::Print(LexStream& lex_stream)
{
    if (type)
    {
        Coutput << '#' << id << " #" << expression -> id << endl;
        type -> Print(lex_stream);
    }
    else
    {
        Coutput << '#' << id << " (Java Semantic Cast to " << Type() -> Name()
                << "):  #" << expression -> id << endl;
    }
    expression -> Print(lex_stream);
}

void AstBinaryExpression::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (BinaryExpression):  "
            << '#' << left_expression -> id << ' '
            << lex_stream.NameString(binary_operator_token)
            << " #" << right_expression -> id << endl;

    left_expression -> Print(lex_stream);
    right_expression -> Print(lex_stream);
}

void AstInstanceofExpression::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (InstanceofExpression):  #"
            << expression -> id << ' '
            << lex_stream.NameString(instanceof_token)
            << " #" << type -> id << endl;
    expression -> Print(lex_stream);
    type -> Print(lex_stream);
}

void AstConditionalExpression::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (ConditionalExpression):  "
            << '#' << test_expression -> id
            << " ? #" << true_expression -> id
            << " : #" << false_expression -> id << endl;

    test_expression -> Print(lex_stream);
    true_expression -> Print(lex_stream);
    false_expression -> Print(lex_stream);
}

void AstAssignmentExpression::Print(LexStream& lex_stream)
{
    Coutput << '#' << id << " (AssignmentExpression):  "
            << '#' << left_hand_side -> id << ' '
            << lex_stream.NameString(assignment_operator_token)
            << " #" << expression -> id << endl;

    left_hand_side -> Print(lex_stream);
    expression -> Print(lex_stream);
}

#endif // JOPA_DEBUG



} // Close namespace Jopa block
