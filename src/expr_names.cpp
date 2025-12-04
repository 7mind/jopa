// Semantic expression processing - name and variable resolution
// Split from expr.cpp for maintainability

#include "platform.h"
#include "double.h"
#include "parser.h"
#include "semantic.h"
#include "control.h"
#include "table.h"
#include "tuple.h"
#include "spell.h"
#include "option.h"
#include "stream.h"
#include "typeparam.h"
#include "paramtype.h"
#include <cstring>

namespace Jopa {

VariableSymbol* Semantic::FindVariableInType(TypeSymbol* type,
                                             AstExpression* expr,
                                             NameSymbol* name_symbol)
{
    Tuple<VariableSymbol*> variable_set(2); // Stores variable conflicts.
    AstFieldAccess* field_access = expr -> FieldAccessCast();
    AstName* name = expr -> NameCast();
    AstExpression* base = name ? name -> base_opt : field_access -> base;
    assert(base);
    VariableSymbol* variable;
    if (! name_symbol)
        name_symbol = lex_stream -> NameSymbol(expr -> RightToken());
    if (! type -> expanded_field_table)
        ComputeFieldsClosure(type, expr -> RightToken());

    //
    // Find the accessible fields with the correct name in the type.
    //
    VariableShadowSymbol* variable_shadow =
        type -> expanded_field_table -> FindVariableShadowSymbol(name_symbol);

    if (variable_shadow)
    {
        variable = variable_shadow -> variable_symbol;
        if (! variable -> IsTyped())
            variable -> ProcessVariableSignature(this, expr -> RightToken());
        if (MemberAccessCheck(type, variable, base))
            variable_set.Next() = variable;

        for (unsigned i = 0; i < variable_shadow -> NumConflicts(); i++)
        {
            variable = variable_shadow -> Conflict(i);
            if (! variable -> IsTyped())
                variable -> ProcessVariableSignature(this,
                                                     expr -> RightToken());
            if (MemberAccessCheck(type, variable, base))
                variable_set.Next() = variable;
        }
    }

    if (variable_set.Length() == 0)
        return NULL;
    else if (variable_set.Length() > 1)
    {
        ReportSemError(SemanticError::AMBIGUOUS_FIELD, expr,
                       name_symbol -> Name(),
                       variable_set[0] -> ContainingType() -> ContainingPackageName(),
                       variable_set[0] -> ContainingType() -> ExternalName(),
                       variable_set[1] -> ContainingType() -> ContainingPackageName(),
                       variable_set[1] -> ContainingType() -> ExternalName());
    }

    variable = variable_set[0];
    if (variable -> ACC_SYNTHETIC())
    {
        ReportSemError(SemanticError::SYNTHETIC_VARIABLE_ACCESS, expr,
                       variable -> Name(),
                       variable -> ContainingType() -> ContainingPackageName(),
                       variable -> ContainingType() -> ExternalName());
    }

    if (control.option.deprecation && variable -> IsDeprecated() &&
        ! InDeprecatedContext())
    {
        ReportSemError(SemanticError::DEPRECATED_FIELD, expr,
                       variable -> Name(),
                       variable -> ContainingType() -> ContainingPackageName(),
                       variable -> ContainingType() -> ExternalName());
    }
    return variable;
}


//
// Called when no accessible variable was found. The access must be one of
// AstFieldAccess or AstSimpleName. This checks in order: an accessible no-arg
// method by the same name, an inaccessible field in a superclass, a
// misspelled field name, a type by the same name, and finally the field was
// not found.
//
void Semantic::ReportVariableNotFound(AstExpression* access, TypeSymbol* type)
{
    TokenIndex id_token = access -> RightToken();
    NameSymbol* name_symbol = lex_stream -> NameSymbol(id_token);
    VariableShadowSymbol* variable_shadow;

    if (! type -> expanded_field_table)
        ComputeFieldsClosure(type, id_token);
    if (! type -> expanded_method_table)
        ComputeMethodsClosure(type, id_token);

    //
    // Search for an accessible no-arg method of the same name.
    //
    MethodShadowSymbol* method_shadow;
    for (method_shadow = type -> expanded_method_table ->
             FindMethodShadowSymbol(name_symbol);
         method_shadow; method_shadow = method_shadow -> next_method)
    {
        MethodSymbol* method = method_shadow -> method_symbol;

        //
        // Make sure that method has been fully prepared.
        //
        if (! method -> IsTyped())
            method -> ProcessMethodSignature(this, id_token);

        if (method -> NumFormalParameters() == 0 &&
            MemberAccessCheck(type, method))
        {
            ReportSemError(SemanticError::METHOD_NOT_FIELD,
                           id_token, name_symbol -> Name(),
                           method -> containing_type -> ContainingPackageName(),
                           method -> containing_type -> ExternalName());
            return;
        }
    }

    //
    // Check if the field is inaccessible.
    //
    for (TypeSymbol* super_type = type;
         super_type; super_type = super_type -> super)
    {
        variable_shadow = super_type -> expanded_field_table ->
            FindVariableShadowSymbol(name_symbol);
        if (variable_shadow)
        {
            VariableSymbol* variable = variable_shadow -> variable_symbol;
            TypeSymbol* containing_type = variable -> owner -> TypeCast();

            //
            // A protected instance field in the superclass is inaccessible if
            // the base expression is the wrong type.
            //
            if (variable -> ACC_PROTECTED() &&
                ! variable -> ACC_STATIC() &&
                ThisType() -> HasProtectedAccessTo(containing_type))
            {
                ReportSemError(SemanticError::PROTECTED_INSTANCE_FIELD_NOT_ACCESSIBLE,
                               id_token, name_symbol -> Name(),
                               containing_type -> ContainingPackageName(),
                               containing_type -> ExternalName(),
                               ThisType() -> ContainingPackageName(),
                               ThisType() -> ExternalName());
            }
            else
            {
                ReportSemError(SemanticError::FIELD_NOT_ACCESSIBLE,
                               id_token, name_symbol -> Name(),
                               containing_type -> ContainingPackageName(),
                               containing_type -> ExternalName(),
                               variable -> AccessString());
            }
            return;
        }
    }

    //
    // Try various possibilities of what the user might have meant.
    //
    AstName* ast_name = access -> NameCast();
    TypeSymbol* inaccessible_type = (! ast_name || ast_name -> base_opt)
        ? NULL : FindInaccessibleType(ast_name);
    VariableSymbol* variable = FindMisspelledVariableName(type, access);
    if (variable)
    {
        //
        // There is a field with a similar name.
        //
        ReportSemError(SemanticError::FIELD_NAME_MISSPELLED,
                       id_token, name_symbol -> Name(),
                       type -> ContainingPackageName(),
                       type -> ExternalName(),
                       variable -> Name());
    }
    else if (FindType(id_token))
    {
        //
        // There is a type or package of the same name.
        //
        ReportSemError(SemanticError::TYPE_NOT_FIELD,
                       id_token, name_symbol -> Name());
    }
    else if (inaccessible_type)
    {
        //
        // There is an inaccessible type of the same name.
        //
        ReportTypeInaccessible(ast_name, inaccessible_type);
    }
    else if (access -> symbol && access -> symbol -> PackageCast())
    {
        ReportSemError(SemanticError::UNKNOWN_AMBIGUOUS_NAME,
                       access, name_symbol -> Name());
    }
    else
    {
        //
        // Give up. We didn't find it.
        //
        ReportSemError(SemanticError::FIELD_NOT_FOUND,
                       id_token, name_symbol -> Name(),
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }
}


void Semantic::FindVariableInEnvironment(Tuple<VariableSymbol*>& variables_found,
                                         SemanticEnvironment*& where_found,
                                         SemanticEnvironment* envstack,
                                         NameSymbol* name_symbol,
                                         TokenIndex identifier_token)
{
    variables_found.Reset();
    where_found = (SemanticEnvironment*) NULL;

    for (SemanticEnvironment* env = envstack; env; env = env -> previous)
    {
        VariableSymbol* variable_symbol =
            env -> symbol_table.FindVariableSymbol(name_symbol);
        if (variable_symbol) // a local variable
        {
            variables_found.Next() = variable_symbol;
            where_found = env;
            break;
        }

        TypeSymbol* type = env -> Type();
        if (! type -> expanded_field_table)
            ComputeFieldsClosure(type, identifier_token);
        VariableShadowSymbol* variable_shadow = type ->
            expanded_field_table -> FindVariableShadowSymbol(name_symbol);
        if (variable_shadow)
        {
            //
            // Since type -> IsOwner(this_type()), i.e., type encloses
            // this_type(), variable_symbol is accessible, even if it is
            // private.
            //
            variables_found.Next() = variable_shadow -> variable_symbol;

            //
            // Recall that even an inaccessible member x of a super class (or
            // interface) S, in addition to not been inherited by a subclass,
            // hides all other occurrences of x that may appear in a super
            // class (or super interface) of S (see 8.3).
            //
            for (unsigned i = 0; i < variable_shadow -> NumConflicts(); i++)
                variables_found.Next() = variable_shadow -> Conflict(i);
            where_found = env;
            break;
        }
    }

    // Java 5: If not found in environment, check single static imports
    // TODO: Implement lazy lookup using StaticImportInfo
    // For now, disabled while testing
    if (false && variables_found.Length() == 0 &&
        control.option.source >= JopaOption::SDK1_5)
    {
        // Will implement lazy lookup here
    }
}


VariableSymbol* Semantic::FindVariableInEnvironment(SemanticEnvironment*& where_found,
                                                    TokenIndex identifier_token)
{
    Tuple<VariableSymbol*> variables_found(2);
    NameSymbol* name_symbol = lex_stream -> NameSymbol(identifier_token);
    SemanticEnvironment* envstack = state_stack.Top();
    FindVariableInEnvironment(variables_found, where_found, envstack,
                              name_symbol, identifier_token);

    VariableSymbol* variable_symbol =
        (VariableSymbol*) (variables_found.Length() > 0
                            ? variables_found[0] : NULL);

    if (variable_symbol)
    {
        if (variable_symbol -> IsLocal()) // a local variable
        {
            if (where_found != envstack)
            {
                TypeSymbol* type = envstack -> Type();

                if (! variable_symbol -> ACC_FINAL())
                {
                    MethodSymbol* method =
                        variable_symbol -> owner -> MethodCast();

                    //
                    // TODO: What if the method is a constructor ?
                    // if (method -> Identity() != control.init_symbol &&
                    //     method -> Identity() != control.block_init_symbol &&
                    //     method -> Identity() != control.clinit_symbol)
                    //
                    ReportSemError(SemanticError::INNER_CLASS_REFERENCE_TO_NON_FINAL_LOCAL_VARIABLE,
                                   identifier_token,
                                   type -> ContainingPackageName(),
                                   type -> ExternalName(),
                                   lex_stream -> NameString(identifier_token),
                                   method -> ExternalName());
                }
                else if (! variable_symbol -> initial_value)
                {
                    //
                    // The variable is not constant, so we need to insert a
                    // variable shadow in the outermost local class within the
                    // scope of the variable, and use that shadow instead.
                    //
                    variable_symbol = FindLocalVariable(variable_symbol,
                                                        envstack -> Type());
                    TypeSymbol* shadow_owner =
                        variable_symbol -> ContainingType();
                    assert(shadow_owner);
                    where_found = shadow_owner -> semantic_environment;
                }
            }
        }
        else if (variable_symbol -> owner != where_found -> Type())
        {
            //
            // The field was inherited.
            //
            TypeSymbol* type = (TypeSymbol*) variable_symbol -> owner;
            if (variable_symbol -> ACC_SYNTHETIC())
            {
                ReportSemError(SemanticError::SYNTHETIC_VARIABLE_ACCESS,
                               identifier_token,
                               variable_symbol -> Name(),
                               type -> ContainingPackageName(),
                               type -> ExternalName());
            }
            else if (control.option.pedantic)
            {
                //
                // Give a pedantic warning if the inherited field shadowed
                // a field of the same name within an enclosing lexical scope.
                //
                Tuple<VariableSymbol*> others(2);
                SemanticEnvironment* found_other;
                SemanticEnvironment* previous_env = where_found -> previous;
                FindVariableInEnvironment(others, found_other, previous_env,
                                          name_symbol, identifier_token);

                if (others.Length() > 0 &&
                    where_found -> Type() != found_other -> Type())
                {
                    for (unsigned i = 0; i < others.Length(); i++)
                    {
                        if (others[i] != variable_symbol)
                        {
                            MethodSymbol* method =
                                others[i] -> owner -> MethodCast();

                            if (method)
                            {
                                ReportSemError(SemanticError::INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_LOCAL,
                                               identifier_token,
                                               lex_stream -> NameString(identifier_token),
                                               type -> ContainingPackageName(),
                                               type -> ExternalName(),
                                               method -> Name());
                                break;
                            }
                            else if (others[i] -> owner == found_other -> Type())
                            {
                                ReportSemError(SemanticError::INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_MEMBER,
                                               identifier_token,
                                               lex_stream -> NameString(identifier_token),
                                               type -> ContainingPackageName(),
                                               type -> ExternalName(),
                                               found_other -> Type() -> ContainingPackageName(),
                                               found_other -> Type() -> ExternalName());
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    for (unsigned i = 1; i < variables_found.Length(); i++)
    {
        ReportSemError(SemanticError::AMBIGUOUS_FIELD, identifier_token,
                       variable_symbol -> Name(),
                       variable_symbol -> ContainingType() -> ContainingPackageName(),
                       variable_symbol -> ContainingType() -> ExternalName(),
                       variables_found[i] -> ContainingType() -> ContainingPackageName(),
                       variables_found[i] -> ContainingType() -> ExternalName());
    }

    if (variable_symbol)
    {
        if (control.option.deprecation && variable_symbol -> IsDeprecated() &&
            ! InDeprecatedContext())
        {
            ReportSemError(SemanticError::DEPRECATED_FIELD, identifier_token,
                           variable_symbol -> Name(),
                           variable_symbol -> ContainingType() -> ContainingPackageName(),
                           variable_symbol -> ContainingType() -> ExternalName());
        }

        if (! variable_symbol -> IsTyped())
            variable_symbol -> ProcessVariableSignature(this, identifier_token);
    }
    return variable_symbol;
}


//
// Find a variable shadow in the outermost local class within the scope of
// the variable, and return a local variable shadow to it instead.
//
VariableSymbol* Semantic::FindLocalVariable(VariableSymbol* local,
                                            TypeSymbol* type)
{
    while (local -> accessed_local)
        local = local -> accessed_local;
    assert(local -> IsLocal());

    TypeSymbol* containing_type = local -> ContainingType();
    if (type == containing_type)
        return local;

    while (type && type -> ContainingType() != containing_type)
    {
        if (! type -> EnclosingType())
        {
            assert(type -> Anonymous());
            break;
        }
        type = type -> ContainingType();
    }
    assert(type && type -> IsLocal());
    return type -> FindOrInsertLocalShadow(local);
}


//
// Using the this$0 variable, locate the appropriate enclosing instance.
//
AstExpression* Semantic::FindEnclosingInstance(AstExpression* base,
                                               TypeSymbol* environment_type,
                                               bool exact)
{
    TypeSymbol* base_type = base -> Type();
    assert(base_type != environment_type &&
           base_type -> HasEnclosingInstance(environment_type, exact));
    VariableSymbol* this0 = base_type -> EnclosingInstance();
    if (! this0)
    {
        //
        // For anonymous/local classes we expect a synthetic this$0. If it
        // wasn't created yet, synthesize it so we can still walk enclosing
        // instances (needed for qualified this used inside arguments of an
        // explicit constructor invocation).
        //
        TypeSymbol* enclosing_type = base_type -> EnclosingType();
        if (! enclosing_type)
            enclosing_type = base_type -> ContainingType();
        if (enclosing_type)
            this0 = base_type -> InsertThis0();
        if (! this0)
        {
            assert(base_type -> Anonymous() && base_type -> IsLocal());
            return NULL;
        }
    }

    TokenIndex tok = base -> RightToken();

    AstFieldAccess* field_access =
        compilation_unit -> ast_pool -> GenFieldAccess();
    field_access -> base = base;
    field_access -> identifier_token = tok;
    field_access -> symbol = this0;

    if (exact ? (this0 -> Type() == environment_type)
        : (this0 -> Type() -> IsSubclass(environment_type)))
    {
        return field_access;
    }
    return FindEnclosingInstance(field_access, environment_type, exact);
}


//
// Generate access to the correct enclosing instance.
//
AstExpression* Semantic::CreateAccessToType(Ast* source,
                                            TypeSymbol* environment_type)
{
    TypeSymbol* this_type = ThisType();

    TokenIndex left_tok;
    TokenIndex right_tok;

    AstName* variable = source -> NameCast();
    AstMethodInvocation* method = source -> MethodInvocationCast();
    AstSuperCall* super_call = source -> SuperCallCast();
    AstThisExpression* this_expr = source -> ThisExpressionCast();
    AstSuperExpression* super_expr = source -> SuperExpressionCast();
    AstClassCreationExpression* class_creation =
        source -> ClassCreationExpressionCast();
    bool exact = false;

    if (variable)
    {
        assert(! variable -> base_opt);
        left_tok = right_tok = variable -> identifier_token;
        //
        // If this type subclasses the enclosing type, then CreateAccess was
        // called because the simple name was not inherited into this type
        // (ie. the variable is private or else hidden in a superclass). In
        // this case, turn on exact enclosing type checking.
        //
        if (this_type -> IsSubclass(environment_type))
            exact = true;
    }
    else if (method)
    {
        assert(! method -> base_opt);
        left_tok = right_tok = method -> identifier_token;
        //
        // If this type subclasses the enclosing type, then CreateAccess was
        // called because the simple name was not inherited into this type
        // (ie. the method is private or else hidden in a superclass). In
        // this case, turn on exact enclosing type checking.
        //
        if (this_type -> IsSubclass(environment_type))
            exact = true;
    }
    else if (class_creation)
    {
        left_tok = right_tok = class_creation -> new_token;
        //
        // For local classes, we need exact matching. The enclosing instance
        // must be the one captured when the local class was defined, not just
        // any subclass-compatible instance. JLS 15.9.2
        //
        TypeSymbol* created_type = class_creation -> class_type -> symbol;
        if (created_type && created_type -> IsLocal())
            exact = true;
    }
    else if (super_call)
        left_tok = right_tok = super_call -> super_token;
    else if (this_expr)
    {
        assert(this_expr -> base_opt);
        left_tok = this_expr -> LeftToken();
        right_tok = this_expr -> this_token;
        exact = true;
    }
    else if (super_expr)
    {
        assert(super_expr -> base_opt);
        left_tok = super_expr -> LeftToken();
        right_tok = super_expr -> super_token;
        exact = true;
    }
    else assert(false && "create access to invalid expression");

    AstExpression* resolution;

    if (! this_type -> HasEnclosingInstance(environment_type, exact))
    {
        ReportSemError((ExplicitConstructorInvocation() &&
                        this_type -> IsSubclass(environment_type)
                        ? SemanticError::ENCLOSING_INSTANCE_ACCESS_FROM_CONSTRUCTOR_INVOCATION
                        : SemanticError::ENCLOSING_INSTANCE_NOT_ACCESSIBLE),
                       left_tok, right_tok,
                       environment_type -> ContainingPackageName(),
                       environment_type -> ExternalName());
        resolution = compilation_unit -> ast_pool -> GenName(left_tok);
        resolution -> symbol = control.no_type;
    }
    else
    {
        //
        // Collapse everything except qualified this or super to the innermost
        // class. Start from the parameter this$0 in an explicit constructor
        // invocation, else start from this.
        //
        if (ExplicitConstructorInvocation())
        {
            VariableSymbol* variable = LocalSymbolTable().
                FindVariableSymbol(control.this_name_symbol);
            assert(variable);
            resolution = compilation_unit -> ast_pool -> GenName(left_tok);
            resolution -> symbol = variable;
        }
        else
        {
            resolution =
                compilation_unit -> ast_pool -> GenThisExpression(left_tok);
            resolution -> symbol = this_type;
        }
        TypeSymbol* resolved_type = resolution -> Type();
        if (resolved_type != environment_type &&
            (! resolved_type -> IsSubclass(environment_type) || exact))
        {
            AstExpression* intermediate =
                FindEnclosingInstance(resolution, environment_type, exact);
            if (! intermediate)
            {
                ReportSemError(SemanticError::ENCLOSING_INSTANCE_ACCESS_ACROSS_STATIC_REGION,
                               left_tok, right_tok,
                               environment_type -> ContainingPackageName(),
                               environment_type -> ExternalName());
                resolution -> symbol = control.no_type;
            }
            else resolution = intermediate;
        }
    }
    if (super_expr)
        environment_type = environment_type -> super;
    return ConvertToType(resolution, environment_type);
}


void Semantic::CreateAccessToScopedVariable(AstName* name,
                                            TypeSymbol* environment_type)
{
    assert(! name -> base_opt);
    VariableSymbol* variable = (VariableSymbol*) name -> symbol;
    assert(variable -> owner -> TypeCast());
    AstExpression* access_expression;
    if (variable -> ACC_STATIC())
    {
        access_expression = compilation_unit -> ast_pool ->
            GenName(name -> identifier_token);
        access_expression -> symbol = environment_type;
    }
    else
    {
        AstThisExpression* this_expr = compilation_unit -> ast_pool ->
            GenThisExpression(name -> identifier_token);
        this_expr -> resolution_opt =
            CreateAccessToType(name, environment_type);
        this_expr -> symbol = this_expr -> resolution_opt -> symbol;
        access_expression = this_expr;
    }

    if (access_expression -> symbol != control.no_type)
    {
        TypeSymbol* containing_type = variable -> ContainingType();

        if (variable -> ACC_PRIVATE() ||
            (variable -> ACC_PROTECTED() &&
             ! ProtectedAccessCheck(containing_type)))
        {
            assert((variable -> ACC_PRIVATE() &&
                    environment_type == containing_type) ||
                   (variable -> ACC_PROTECTED() &&
                    environment_type -> IsSubclass(containing_type)));

            TokenIndex loc = name -> identifier_token;
            AstArguments* args =
                compilation_unit -> ast_pool -> GenArguments(loc, loc);
            if (! variable -> ACC_STATIC())
            {
                // TODO: WARNING: sharing of Ast subtree !!!
                args -> AllocateArguments(1);
                args -> AddArgument(access_expression);
            }

            AstMethodInvocation* accessor =
                compilation_unit -> ast_pool -> GenMethodInvocation(loc);
            accessor -> base_opt = access_expression;
            accessor -> arguments = args;
            // The default base type of the accessor method is appropriate.
            accessor -> symbol =
                environment_type -> GetReadAccessMethod(variable);

            name -> resolution_opt = accessor;
        }
        else
        {
            AstFieldAccess* field_access =
                compilation_unit -> ast_pool -> GenFieldAccess();
            field_access -> base = access_expression;
            field_access -> identifier_token = name -> identifier_token;
            field_access -> symbol = variable;

            name -> resolution_opt = field_access;
        }
    }
}


void Semantic::CreateAccessToScopedMethod(AstMethodInvocation* method_call,
                                          TypeSymbol* environment_type)
{
    assert(environment_type -> IsOwner(ThisType()));
    assert(! method_call -> base_opt);
    MethodSymbol* method = (MethodSymbol*) method_call -> symbol;
    AstExpression* access_expression;
    if (method -> ACC_STATIC())
    {
        access_expression = compilation_unit -> ast_pool ->
            GenName(method_call -> identifier_token);
        access_expression -> symbol = environment_type;
    }
    else
    {
        AstThisExpression* this_expr = compilation_unit -> ast_pool ->
            GenThisExpression(method_call -> identifier_token);
        this_expr -> resolution_opt =
            CreateAccessToType(method_call, environment_type);
        this_expr -> symbol = this_expr -> resolution_opt -> symbol;
        access_expression = this_expr;
    }

    if (access_expression -> symbol != control.no_type)
    {
        method_call -> base_opt = access_expression;
        TypeSymbol* containing_type = method -> containing_type;

        if (method -> ACC_PRIVATE() ||
            (method -> ACC_PROTECTED() &&
             ! ProtectedAccessCheck(containing_type)))
        {
            assert((method -> ACC_PRIVATE() &&
                    environment_type == containing_type) ||
                   (method -> ACC_PROTECTED() &&
                    environment_type -> IsSubclass(containing_type)));

            AstArguments* args = compilation_unit -> ast_pool ->
                GenArguments(method_call -> arguments -> left_parenthesis_token,
                             method_call -> arguments -> right_parenthesis_token);
            unsigned num_args = method_call -> arguments -> NumArguments();
            if (! method -> ACC_STATIC())
            {
                args -> AllocateArguments(num_args + 1);
                args -> AddArgument(access_expression);
            }
            else args -> AllocateArguments(num_args);
            for (unsigned i = 0; i < num_args; i++)
                args -> AddArgument(method_call -> arguments -> Argument(i));

            AstMethodInvocation* accessor = compilation_unit -> ast_pool ->
                GenMethodInvocation(method_call -> identifier_token);
            accessor -> base_opt = access_expression;
            accessor -> arguments = args;
            accessor -> symbol =
                // default base type is appropriate
                environment_type -> GetReadAccessMethod(method);

            method_call -> symbol = method;
            method_call -> resolution_opt = accessor;
        }
    }
}


void Semantic::CheckSimpleName(AstName* name, SemanticEnvironment* where_found)
{
    VariableSymbol* variable_symbol = name -> symbol -> VariableCast();
    assert(variable_symbol && ! name -> base_opt);

    if (StaticRegion() && ! ExplicitConstructorInvocation())
    {
        if (! (variable_symbol -> IsLocal() ||
               variable_symbol -> ACC_STATIC()))
        {
            ReportSemError(SemanticError::NAME_NOT_CLASS_VARIABLE,
                           name -> identifier_token,
                           lex_stream -> NameString(name -> identifier_token));
        }
        else if (variable_symbol -> owner -> TypeCast() &&
                 ! variable_symbol -> IsDeclarationComplete() &&
                 ! ProcessingSimpleAssignment() &&
                 ThisVariable())  // Only report error in field initializers, not in method bodies
        {
            ReportSemError(SemanticError::NAME_NOT_YET_AVAILABLE,
                           name -> identifier_token,
                           lex_stream -> NameString(name -> identifier_token));
        }
    }
    else if (! variable_symbol -> ACC_STATIC()) // an instance variable?
    {
        // an instance field member ?
        TypeSymbol* containing_type = variable_symbol -> owner -> TypeCast();

        // variable must be a field for these next errors to be valid
        if (containing_type && ! variable_symbol -> accessed_local)
        {
            if (containing_type == ThisType() &&
                ! variable_symbol -> IsDeclarationComplete() &&
                ! ProcessingSimpleAssignment()) // forward reference?
            {
                ReportSemError(SemanticError::NAME_NOT_YET_AVAILABLE,
                               name -> identifier_token,
                               lex_stream -> NameString(name -> identifier_token));
            }
            else if (ExplicitConstructorInvocation() &&
                     where_found == state_stack.Top())
            {
                //
                // If the variable in question is an instance variable that is
                // declared in this_type (this_type is definitely a class) or
                // one of its super classes, then we have an error:
                //
                ReportSemError(SemanticError::INSTANCE_VARIABLE_IN_EXPLICIT_CONSTRUCTOR,
                               name -> identifier_token,
                               lex_stream -> NameString(name -> identifier_token),
                               containing_type -> Name());
            }
        }
    }
}


void Semantic::ProcessExpressionOrStringConstant(AstExpression* expr)
{
    ProcessExpression(expr);
    //
    // If the expression is of type String, check whether or not it is
    // constant, and if so, compute the result.
    //
    if (expr -> symbol == control.String() && ! expr -> IsConstant())
        control.Utf8_pool.CheckStringConstant(expr);
}


void Semantic::ProcessName(Ast* expr)
{
    AstName* name = (AstName*) expr;
    ProcessAmbiguousName(name);
    TypeSymbol* type = name -> Type();
    if (type == control.no_type)
        return; // ProcessAmbiguousName already reported the error
    if (! type || name -> symbol -> TypeCast())
    {
        ReportVariableNotFound(name, ThisType());
        name -> symbol = control.no_type;
    }
}


//
// Returns true if the type is accessible from the current semantic location.
//
bool Semantic::TypeAccessCheck(TypeSymbol* type)
{
    // According to JLS 6.6.1, a type T[] is accessible if T is accessible.
    if (type -> IsArray())
        type = type -> base_type;

    //
    // Outside a class body, only public types from other packages, or
    // non-private types in the current package, are accessible. For a member
    // type, as in T1.T2, this does not check that T1 is also accessible; that
    // requires additional checks by the caller.
    //
    assert(this_package);
    if (type -> ACC_PUBLIC() ||
        (type -> ContainingPackage() == this_package &&
         ! type -> ACC_PRIVATE()))
    {
        return true;
    }
    if (state_stack.Size() > 0)
    {
        //
        // Inside a class body, all types listed above are accessible.
        // Additionally, declared or inherited member types are accessible.
        //
        TypeSymbol* this_type = ThisType();
        assert(this_type -> ContainingPackage() == this_package);
        if (this_type -> outermost_type == type -> outermost_type ||
            (type -> ACC_PROTECTED() &&
             this_type -> HasProtectedAccessTo(type)))
        {
            return true;
        }
    }
    return false;
}


//
// Returns true if the constructor is accessible. The invocation is used to
// distinguish between different rules for class instance creation and explicit
// constructor invocation.
//
bool Semantic::ConstructorAccessCheck(MethodSymbol* constructor,
                                      bool explicit_ctor)
{
    TypeSymbol* this_type = ThisType();
    TypeSymbol* containing_type = constructor -> containing_type;
    if (this_type -> outermost_type != containing_type -> outermost_type &&
        constructor -> ACC_PRIVATE())
    {
        return false;
    }

    //
    // Default constructors are not accessible outside the package, and
    // protected constructors can only be accessed by a call to super(). This
    // includes anonymous classes, where we will later generate a super() call.
    //
    if (containing_type -> ContainingPackage() != this_package &&
        ! constructor -> ACC_PUBLIC())
    {
        return constructor -> ACC_PROTECTED() && explicit_ctor;
    }
    return true;
}


//
// Returns true if the field or method member symbol can be accessed from this
// semantic point, when the qualifier of the access is base_type. base
// is the qualifying expression for the access, and is NULL for simple names.
//
bool Semantic::MemberAccessCheck(TypeSymbol* base_type, Symbol* symbol,
                                 AstExpression* base)
{
    TypeSymbol* this_type = ThisType();

    VariableSymbol* variable_symbol = symbol -> VariableCast();
    MethodSymbol* method_symbol = symbol -> MethodCast();
    assert(variable_symbol || method_symbol);

    AccessFlags* flags = (variable_symbol ? (AccessFlags*) variable_symbol
                          : (AccessFlags*) method_symbol);
    TypeSymbol* containing_type = (variable_symbol
                                   ? variable_symbol -> ContainingType()
                                   : method_symbol -> containing_type);
    assert(containing_type);

    //
    // When this function, MemberAccessCheck is invoked, it is assumed that
    // the base type has been checked as follows:
    //
    //    if (! TypeAccessCheck(base_type))
    //        ReportTypeInaccessible(base, base_type);
    //

    if (this_type -> outermost_type != containing_type -> outermost_type)
    {
        if (flags -> ACC_PRIVATE())
            return false;
        else if (flags -> ACC_PROTECTED())
        {
            //
            // Within the same package, protected is accessible. Super access
            // has special priveleges (contrary to JLS2 15.11.2,
            // super.name != ((S)this).name; ). JLS2 6.6.2: When packages
            // differ, subclasses may access protected static members without
            // further restrictions, but accessing instance members requires
            // that the qualifier be the subclass or lower.
            // JLS 9.2: Interfaces have no protected members.
            //
            if (base && base -> Type() -> ACC_INTERFACE())
            {
                // Object has no fields, so this would be the protected
                // methods "inherited" into an interface from Object.
                assert(method_symbol);
                return false;
            }
            if (containing_type -> ContainingPackage() == this_package ||
                (base && base -> SuperExpressionCast()))
            {
                return true;
            }
            if (this_type -> HasProtectedAccessTo(containing_type))
            {
                if (flags -> ACC_STATIC())
                    return true;
                for (SemanticEnvironment* env =
                         this_type -> semantic_environment;
                     env; env = env -> previous)
                {
                    if (base_type -> IsSubclass(env -> Type()))
                        return true;
                }
            }
            return false;
        }
        else if (! flags -> ACC_PUBLIC() &&
                 containing_type -> ContainingPackage() != this_package)
        {
            return false;
        }
    }
    return true;
}


//
// Returns true if the current type can access a protected member declared in
// the containing type, without an accessor method. This does not test
// whether the target type and member are accessible, since those checks are
// assumed to be already done.
//
bool Semantic::ProtectedAccessCheck(TypeSymbol* containing_type)
{
    return ThisType() -> IsSubclass(containing_type) ||
        this_package == containing_type -> ContainingPackage();
}


//
// FindVariableMember resolves a qualified field reference. The parameter
// type is the type of the qualifying expression, field_access is the
// expression being resolved.
//
void Semantic::FindVariableMember(TypeSymbol* type, AstExpression* expr)
{
    //
    // TypeCast() returns true for super, this, and instance creation as
    // well as true type names, hence the extra check
    //
    AstFieldAccess* field_access = expr -> FieldAccessCast();
    AstName* name = expr -> NameCast();
    AstExpression* base = name ? name -> base_opt : field_access -> base;
    TokenIndex id_token = expr -> RightToken();
    bool base_is_type = base -> symbol -> TypeCast() && base -> NameCast();

    if (type -> Bad())
    {
        //
        // If no error has been detected so far, report this as an error so
        // that we don't try to generate code later. On the other hand, if an
        // error had been detected prior to this, don't flood the user with
        // spurious messages.
        //
        if (NumErrors() == 0)
            ReportVariableNotFound(expr, type);
        expr -> symbol = control.no_type;
    }
    else if (type == control.null_type || type -> Primitive())
    {
        ReportSemError(SemanticError::TYPE_NOT_REFERENCE, base,
                       type -> Name());
        expr -> symbol = control.no_type;
    }
    else
    {
        TypeSymbol* this_type = ThisType();
        if (! TypeAccessCheck(type))
        {
            ReportTypeInaccessible(base, type);
            expr -> symbol = control.no_type;
            return;
        }

        VariableSymbol* variable = FindVariableInType(type, expr);
        if (variable)
        {
            assert(variable -> IsTyped());

            if (base_is_type && ! variable -> ACC_STATIC())
            {
                ReportSemError(SemanticError::NAME_NOT_CLASS_VARIABLE,
                               id_token, lex_stream -> NameString(id_token));
                expr -> symbol = control.no_type;
                return;
            }
            if (variable -> ACC_STATIC() && ! base_is_type)
            {
                ReportSemError(SemanticError::CLASS_FIELD_ACCESSED_VIA_INSTANCE,
                               id_token, lex_stream -> NameString(id_token));
            }
            //
            // If a variable is FINAL, initialized with a constant expression,
            // and of the form TypeName.Identifier, we substitute the
            // expression here - JLS 15.28. If it is of any other form, we
            // still compute the initial value, which will be inlined in
            // bytecode, but do not treat the expression as a constant - JLS2
            // clarifications.
            //
            if (variable -> ACC_FINAL())
            {
                if (! variable -> IsInitialized())
                    ComputeFinalValue(variable);
                if (base_is_type)
                {
                    assert(variable -> IsInitialized());
                    expr -> value = variable -> initial_value;
                }
            }

            //
            // Access to a private or protected variable in or via an enclosing
            // type? If the base is a super expression, be sure to start from
            // the correct enclosing instance.
            //
            TypeSymbol* containing_type = variable -> ContainingType();
            TypeSymbol* target_type = containing_type;
            if (! variable -> ACC_STATIC() && base -> SuperExpressionCast())
            {
                AstSuperExpression* super_expr = (AstSuperExpression*) base;
                if (super_expr -> base_opt)
                    target_type = super_expr -> base_opt -> symbol;
            }
            //
            // Need accessor if:
            // 1. Private field declared in a different class (including superclass)
            // 2. Access to enclosing class's private/protected field
            // 3. Protected field access across packages without proper subclass relationship
            //
            bool need_accessor = false;
            if (variable -> ACC_PRIVATE())
            {
                // Private fields always require accessor if not in the declaring class
                need_accessor = (containing_type != this_type);
            }
            else if (variable -> ACC_PROTECTED())
            {
                // Protected field needs accessor when accessing through enclosing class
                // or across packages without proper relationship
                need_accessor = this_type != target_type &&
                    (! ProtectedAccessCheck(containing_type) ||
                     target_type != containing_type);
            }
            if (need_accessor)
            {
                if (expr -> IsConstant())
                    expr -> symbol = variable;
                else
                {
                    //
                    // Find the right enclosing class to place the accessor
                    // method in. For private fields, the containing type; for
                    // protected fields, an enclosing class which is related
                    // to the containing type.
                    //
                    TypeSymbol* environment_type = containing_type;
                    if (variable -> ACC_PROTECTED())
                    {
                        for (SemanticEnvironment* env =
                                 this_type -> semantic_environment;
                             env; env = env -> previous)
                        {
                            if (env -> Type() -> IsSubclass(target_type))
                            {
                                environment_type = env -> Type();
                                break;
                            }
                        }
                        assert(environment_type != containing_type &&
                               environment_type != this_type);
                    }

                    AstArguments* args =
                        compilation_unit -> ast_pool -> GenArguments(id_token,
                                                                     id_token);
                    if (! variable -> ACC_STATIC())
                    {
                        args -> AllocateArguments(1);
                        args -> AddArgument(base);
                    }

                    AstMethodInvocation* accessor = compilation_unit ->
                        ast_pool -> GenMethodInvocation(id_token);
                    accessor -> base_opt = base;
                    accessor -> arguments = args;
                    accessor -> symbol = environment_type ->
                        GetReadAccessMethod(variable, base -> Type());

                    if (name)
                        name -> resolution_opt = accessor;
                    else
                        field_access -> resolution_opt = accessor;
                    expr -> symbol = accessor -> symbol;
                }
            }
            else
                expr -> symbol = variable;
        }
        else
        {
            TypeSymbol* inner_type = FindNestedType(type, id_token);
            if (inner_type)
            {
                if (base_is_type)
                {
                    expr -> symbol = inner_type;
                    if (! TypeAccessCheck(inner_type))
                        ReportTypeInaccessible(expr, inner_type);
                }
                else
                {
                    ReportSemError(SemanticError::TYPE_NOT_FIELD, id_token,
                                   lex_stream -> NameString(id_token));
                    expr -> symbol = control.no_type;
                }
            }
            else
            {
                ReportVariableNotFound(expr, type);
                expr -> symbol = control.no_type;
            }
        }
    }
}

//
// Note that method names are not processed here but by the function
// ProcessMethodName.
//
void Semantic::ProcessAmbiguousName(AstName* name)
{
    TypeSymbol* this_type = ThisType();
    //
    // JLS2 6.5.2: If the ambiguous name is a simple name,...
    //
    if (! name -> base_opt)
    {
        TypeSymbol* type;
        //
        // ... If the Identifier appears within the scope (6.3) if a local
        // variable declaration (14.3) or parameter declaration (8.4.1,
        // 8.6.1, 14.18) with that name, then the ambiguous name is
        // reclassified as an ExpressionName...
        //
        // ...Otherwise, consider the class or interface C within whose
        // declaration the Identifier occurs. If C has one or more fields
        // with that name, which may be either declared within it or inherited,
        // then the Ambiguous name is reclassified as an ExpressionName....
        //
        SemanticEnvironment* where_found;
        VariableSymbol* variable_symbol =
            FindVariableInEnvironment(where_found, name -> identifier_token);
        if (variable_symbol)
        {
            assert(variable_symbol -> IsTyped());

            //
            // A variable_symbol that is FINAL may have an initial value.
            // If variable_symbol is not final then its initial value is NULL.
            //
            if (variable_symbol -> ACC_FINAL() &&
                ! variable_symbol -> IsInitialized())
            {
                ComputeFinalValue(variable_symbol);
            }
            name -> value = variable_symbol -> initial_value;
            name -> symbol = variable_symbol;

            //
            // For type parameter types with multiple bounds (intersection types):
            // If the field's type is T where T extends A & B, we need to track
            // the secondary bounds for method lookup. Set secondary_resolved_type
            // to the second bound (first interface bound) so that methods from
            // both A and B are accessible.
            //
            const char* gen_sig = variable_symbol -> GenericSignatureString();
            if (gen_sig && gen_sig[0] == 'T')
            {
                // Field type is a type parameter (TV; format)
                // Extract type parameter name
                TypeSymbol* containing = variable_symbol -> ContainingType();
                if (containing)
                {
                    // Find the type parameter with matching name
                    for (unsigned i = 0; i < containing -> NumTypeParameters(); i++)
                    {
                        TypeParameterSymbol* tp = containing -> TypeParameter(i);
                        if (tp)
                        {
                            const char* tp_utf8 = tp -> Utf8Name();
                            if (tp_utf8)
                            {
                                unsigned tp_len = strlen(tp_utf8);
                                // gen_sig is "T<name>;" - compare name part
                                if (strncmp(gen_sig + 1, tp_utf8, tp_len) == 0 &&
                                    gen_sig[tp_len + 1] == ';')
                                {
                                    // Found the type parameter
                                    // If it has multiple bounds, set secondary for intersection
                                    if (tp -> NumBounds() > 1)
                                        name -> secondary_resolved_type = tp -> Bound(1);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            //
            // Type substitution for fields inherited from generic superclasses:
            // If a field's declared type is a type parameter (e.g., T value in class Value<T>),
            // and we're accessing it from a subclass with concrete type arguments
            // (e.g., IntegerValue extends Value<Integer>), substitute the type parameter
            // with the actual type argument.
            //
            TypeSymbol* field_containing_type = variable_symbol -> ContainingType();
            if (field_containing_type && field_containing_type != this_type &&
                field_containing_type -> NumTypeParameters() > 0)
            {
                // Field is from a generic superclass - check if its type is a type parameter
                TypeSymbol* field_type = variable_symbol -> Type();

                // Walk superclass chain to find the parameterized version of field_containing_type
                ParameterizedType* param_super = NULL;
                for (TypeSymbol* search = this_type; search && !param_super; search = search -> super)
                {
                    if (search -> super == field_containing_type)
                    {
                        param_super = search -> GetParameterizedSuper();
                        break;
                    }
                }

                // If we found the parameterized superclass, try to substitute
                if (param_super && param_super -> NumTypeArguments() > 0)
                {
                    // Get the field's declared type name
                    // We need to match by NAME, not by erasure, because multiple
                    // type parameters can have the same erasure (e.g., K and V both erase to Object)
                    const char* field_type_param_name = NULL;

                    // First check if the field's generic signature indicates a type parameter (TV; format)
                    // Use GenericSignatureString() which returns the generic signature, not erased
                    const char* sig_str = variable_symbol -> GenericSignatureString();
                    if (sig_str && sig_str[0] == 'T')
                    {
                        // Type parameter signature: T<name>;
                        // Extract the type parameter name between 'T' and ';'
                        field_type_param_name = sig_str + 1;
                    }

                    // Also try from source file if available
                    const wchar_t* field_type_name_wide = NULL;
                    AstFieldDeclaration* field_decl = variable_symbol -> field_declaration ?
                        variable_symbol -> field_declaration -> FieldDeclarationCast() : NULL;
                    // Use the field's containing type's lex_stream (which may be different from current file)
                    if (field_decl && field_decl -> type &&
                        field_containing_type -> file_symbol &&
                        field_containing_type -> file_symbol -> lex_stream)
                    {
                        LexStream* field_lex_stream = field_containing_type -> file_symbol -> lex_stream;
                        AstTypeName* type_name_ast = field_decl -> type -> TypeNameCast();
                        if (type_name_ast && type_name_ast -> name && ! type_name_ast -> base_opt &&
                            ! type_name_ast -> type_arguments_opt)
                        {
                            // Simple type name - could be a type parameter
                            field_type_name_wide = field_lex_stream -> NameString(type_name_ast -> name -> identifier_token);
                        }
                    }

                    bool found_match = false;
                    if (field_type_param_name)
                    {
                        // Match using signature's type parameter name (narrow string)
                        // The name is from 'T' to ';' (exclusive)
                        for (unsigned i = 0; i < field_containing_type -> NumTypeParameters(); i++)
                        {
                            TypeParameterSymbol* type_param = field_containing_type -> TypeParameter(i);
                            if (type_param)
                            {
                                // Compare with type parameter's Utf8 name
                                const char* tp_utf8 = type_param -> Utf8Name();
                                if (tp_utf8)
                                {
                                    // Check if the names match (field_type_param_name ends at ';')
                                    unsigned tp_len = strlen(tp_utf8);
                                    if (strncmp(field_type_param_name, tp_utf8, tp_len) == 0 &&
                                        field_type_param_name[tp_len] == ';')
                                    {
                                        // Match! Substitute with the actual type argument
                                        if (i < param_super -> NumTypeArguments())
                                        {
                                            Type* type_arg = param_super -> TypeArgument(i);
                                            if (type_arg)
                                            {
                                                TypeSymbol* substituted = type_arg -> Erasure();
                                                // ErasedType() returns NULL for unbounded type parameters, use Object
                                                if (! substituted)
                                                    substituted = control.Object();
                                                if (substituted && substituted != control.no_type)
                                                {
                                                    name -> resolved_type = substituted;
                                                    found_match = true;
                                                }
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else if (field_type_name_wide)
                    {
                        // Match using source file wide string
                        for (unsigned i = 0; i < field_containing_type -> NumTypeParameters(); i++)
                        {
                            TypeParameterSymbol* type_param = field_containing_type -> TypeParameter(i);
                            if (type_param && wcscmp(field_type_name_wide, type_param -> Name()) == 0)
                            {
                                // Match! Substitute with the actual type argument
                                if (i < param_super -> NumTypeArguments())
                                {
                                    Type* type_arg = param_super -> TypeArgument(i);
                                    if (type_arg)
                                    {
                                        TypeSymbol* substituted = type_arg -> Erasure();
                                        // ErasedType() returns NULL for unbounded type parameters, use Object
                                        if (! substituted)
                                            substituted = control.Object();
                                        if (substituted && substituted != control.no_type)
                                        {
                                            name -> resolved_type = substituted;
                                            found_match = true;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                    if (!found_match)
                    {
                        // Fall back to erasure comparison for fields without signature info
                        for (unsigned i = 0; i < field_containing_type -> NumTypeParameters(); i++)
                        {
                            TypeParameterSymbol* type_param = field_containing_type -> TypeParameter(i);
                            // ErasedType() returns NULL for unbounded type parameters, meaning Object
                            TypeSymbol* erased = type_param ? type_param -> ErasedType() : NULL;
                            if (!erased)
                                erased = control.Object();  // Unbounded T erases to Object
                            if (type_param && erased == field_type)
                            {
                                // The field's type is this type parameter's erasure
                                // Substitute with the actual type argument
                                if (i < param_super -> NumTypeArguments())
                                {
                                    Type* type_arg = param_super -> TypeArgument(i);
                                    if (type_arg)
                                    {
                                        TypeSymbol* substituted = type_arg -> Erasure();
                                        if (substituted && substituted != control.no_type)
                                        {
                                            name -> resolved_type = substituted;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }

            CheckSimpleName(name, where_found);

            //
            // If the variable belongs to an outer type, add the proper
            // pointer dereferences (and method access in the case of a
            // private variable) necessary to  get to it.
            //
            if (where_found != state_stack.Top() &&
                variable_symbol -> owner -> TypeCast())
            {
                CreateAccessToScopedVariable(name, where_found -> Type());
            }
        }
        //
        // Java 5: Check for static imports before type lookup
        //
        else if (control.option.source >= JopaOption::SDK1_5)
        {
            NameSymbol* name_symbol = lex_stream -> NameSymbol(name -> identifier_token);
            Symbol* static_member = NULL;

            // First check single static imports (lazy lookup)
            for (unsigned i = 0; i < single_static_imports.Length(); i++)
            {
                StaticImportInfo* info = single_static_imports[i];
                if (!info || !info -> type || !info -> member_name)
                    continue;

                if (info -> member_name != name_symbol)
                    continue;

                // Lazy resolution: look up the member in the type now
                TypeSymbol* import_type = info -> type;

                // Skip if type is bad or not ready
                if (import_type -> Bad())
                    continue;

                // For types that are still being processed, skip them for now
                // They will be resolved in a later pass or we'll get a proper error
                if (import_type -> SourcePending())
                    continue;

                // Make sure the type has been processed enough to have members
                if (!import_type -> FieldMembersProcessed())
                    continue;

                // Compute field closure if needed
                if (!import_type -> expanded_field_table)
                {
                    ComputeFieldsClosure(import_type, name -> identifier_token);
                }

                // Double-check the table exists after computation
                if (!import_type -> expanded_field_table)
                    continue;

                VariableShadowSymbol* var_shadow =
                    import_type -> expanded_field_table -> FindVariableShadowSymbol(name_symbol);
                if (var_shadow && var_shadow -> variable_symbol)
                {
                    VariableSymbol* var = var_shadow -> variable_symbol;
                    if (var -> ACC_STATIC())
                    {
                        if (!var -> IsTyped())
                            var -> ProcessVariableSignature(this, name -> identifier_token);
                        static_member = var;
                        break;
                    }
                }

                // Also check for nested types (member types can be statically imported)
                if (!static_member)
                {
                    if (!import_type -> expanded_type_table)
                        ComputeTypesClosure(import_type, name -> identifier_token);

                    if (import_type -> expanded_type_table)
                    {
                        TypeShadowSymbol* type_shadow =
                            import_type -> expanded_type_table -> FindTypeShadowSymbol(name_symbol);
                        if (type_shadow && type_shadow -> type_symbol)
                        {
                            TypeSymbol* nested = type_shadow -> type_symbol;
                            if (nested -> ACC_STATIC() && TypeAccessCheck(nested))
                            {
                                static_member = nested;
                                break;
                            }
                        }
                    }
                }
            }

            // If not found in single imports, check static-on-demand imports
            if (!static_member)
            {
                for (unsigned i = 0; i < static_on_demand_imports.Length(); i++)
                {
                    TypeSymbol* import_type = static_on_demand_imports[i];

                    // Skip if type is bad or not ready
                    if (!import_type || import_type -> Bad() || import_type -> SourcePending())
                        continue;

                    // Make sure the type has been processed enough to have members
                    if (!import_type -> FieldMembersProcessed())
                        continue;

                    // Compute field closure if needed
                    if (!import_type -> expanded_field_table)
                        ComputeFieldsClosure(import_type, name -> identifier_token);

                    // Check for static fields
                    if (import_type -> expanded_field_table)
                    {
                        VariableShadowSymbol* var_shadow =
                            import_type -> expanded_field_table -> FindVariableShadowSymbol(name_symbol);
                        if (var_shadow && var_shadow -> variable_symbol)
                        {
                            VariableSymbol* var = var_shadow -> variable_symbol;
                            if (var -> ACC_STATIC())
                            {
                                if (static_member)
                                {
                                    // Check if it's the same field (inherited via different paths)
                                    VariableSymbol* prev_var = static_member -> VariableCast();
                                    if (prev_var != var)
                                    {
                                        // Ambiguous static import - different fields
                                        ReportSemError(SemanticError::AMBIGUOUS_FIELD,
                                                     name -> identifier_token,
                                                     name_symbol -> Name());
                                        name -> symbol = control.no_type;
                                        return;
                                    }
                                    // Same field via different import, skip
                                    continue;
                                }
                                if (!var -> IsTyped())
                                    var -> ProcessVariableSignature(this, name -> identifier_token);
                                static_member = var;
                            }
                        }
                    }

                    // Check for nested types (if no field found)
                    if (!static_member)
                    {
                        if (!import_type -> expanded_type_table)
                            ComputeTypesClosure(import_type, name -> identifier_token);

                        if (import_type -> expanded_type_table)
                        {
                            TypeShadowSymbol* type_shadow =
                                import_type -> expanded_type_table -> FindTypeShadowSymbol(name_symbol);
                            if (type_shadow && type_shadow -> type_symbol)
                            {
                                TypeSymbol* nested = type_shadow -> type_symbol;
                                if (nested -> ACC_STATIC() && TypeAccessCheck(nested))
                                {
                                    if (static_member)
                                    {
                                        // Check if it's the same type (inherited via different paths)
                                        TypeSymbol* prev_type = static_member -> TypeCast();
                                        if (prev_type != nested)
                                        {
                                            // Ambiguous static import - different types
                                            ReportSemError(SemanticError::AMBIGUOUS_TYPE,
                                                         name -> identifier_token,
                                                         name_symbol -> Name());
                                            name -> symbol = control.no_type;
                                            return;
                                        }
                                        // Same type via different import, skip
                                        continue;
                                    }
                                    static_member = nested;
                                }
                            }
                        }
                    }

                    // Check for static methods (if no field or type found)
                    if (!static_member)
                    {
                        if (!import_type -> MethodMembersProcessed())
                            continue;

                        if (!import_type -> expanded_method_table)
                            ComputeMethodsClosure(import_type, name -> identifier_token);

                        if (import_type -> expanded_method_table)
                        {
                            MethodShadowSymbol* method_shadow =
                                import_type -> expanded_method_table -> FindMethodShadowSymbol(name_symbol);
                            if (method_shadow && method_shadow -> method_symbol)
                            {
                                MethodSymbol* meth = method_shadow -> method_symbol;
                                if (meth -> ACC_STATIC())
                                {
                                    if (!meth -> IsTyped())
                                        meth -> ProcessMethodSignature(this, name -> identifier_token);
                                    static_member = meth;
                                }
                            }
                        }
                    }
                }
            }

            if (static_member)
            {
                name -> symbol = static_member;
                // For variables, check if we need to compute final value
                VariableSymbol* var = static_member -> VariableCast();
                if (var)
                {
                    if (var -> ACC_FINAL() && ! var -> IsInitialized())
                        ComputeFinalValue(var);
                    name -> value = var -> initial_value;
                }
                // Fall through to continue processing
            }
        }
        //
        // ...Otherwise, if a type of that name is declared in the compilation
        // unit (7.3) containing the Identifier, either by a
        // single-type-import declaration (7.5.1) or by a class or interface
        // type declaration (7.6), then the Ambiguous name is reclassified as
        // a TypeName...
        //
        // ...Otherwise, if a type of that name is declared in another
        // compilation unit (7.3) of the package (7.1) of the compilation unit
        // containing the Identifier, then the Ambiguous Name is reclassified
        // as a TypeName...
        //
        // ...Otherwise, if a type of that name is declared by exactly one
        // type-import-on-demand declaration (7.5.2) of the compilation unit
        // containing the Identifier, then the AmbiguousName is reclassified
        // as a TypeName
        //
        // ...Otherwise, if a type of that name is declared by more than one
        // type-import-on-demand declaration of the compilation unit
        // containing the Identifier, then a compile-time error results.
        //
        if (! name -> symbol)
        {
            if ((type = FindType(name -> identifier_token)))
            {
                name -> symbol = type;
                if (control.option.deprecation && type -> IsDeprecated() &&
                    ! InDeprecatedContext())
                {
                    ReportSemError(SemanticError::DEPRECATED_TYPE,
                                   name -> identifier_token,
                                   type -> ContainingPackageName(),
                                   type -> ExternalName());
                }
            }
            //
            // ...Otherwise, the Ambiguous name is reclassified as a PackageName.
            // While the JLS claims a later step determines whether or not
            // a package of that name actually exists, it is pointless to defer
            // the error that long, as a package cannot qualify a method or field
            // access, and a subpackage requires the base package to exist.
            //
            else
        {
            NameSymbol* name_symbol =
                lex_stream -> NameSymbol(name -> identifier_token);
            PackageSymbol* package =
                control.external_table.FindPackageSymbol(name_symbol);
            if (! package)
            {
                //
                // One last check in case the package was not imported.
                //
                package = control.external_table.InsertPackageSymbol(name_symbol,
                                                                     NULL);
                control.FindPathsToDirectory(package);
            }
            if (package -> directory.Length())
                name -> symbol = package;
            else
            {
                ReportVariableNotFound(name, this_type);
                name -> symbol = control.no_type;
            }
        }
        }
    }
    //
    // ...If the ambiguous name is a qualified name,...
    //
    else
    {
        //
        // ...First, classify the name or expression to the left of the '.'...
        //
        AstName* base = name -> base_opt;
        ProcessAmbiguousName(base);

        TypeSymbol* type = base -> Type();
        assert(type || base -> symbol -> PackageCast());

        if (type == control.no_type)
        {
            name -> symbol = control.no_type;
            return;
        }
        PackageSymbol* package = base -> symbol -> PackageCast();
        if (package)
        {
            //
            // ... If there is a package whose name is the name to the
            // left of the '.' and that package contains a declaration of
            // a type whose name is the same as the Identifier, then the
            // AmbiguousName is reclassified as a TypeName...
            //
            NameSymbol* name_symbol =
                lex_stream -> NameSymbol(name -> identifier_token);
            type = package -> FindTypeSymbol(name_symbol);

            if (type)
            {
                if (type -> SourcePending())
                    control.ProcessHeaders(type -> file_symbol);
                name -> symbol = type;
            }
            else
            {
                FileSymbol* file_symbol =
                    Control::GetFile(control, package, name_symbol);
                if (file_symbol)
                {
                    type = ReadType(file_symbol, package, name_symbol,
                                    name -> identifier_token);
                    name -> symbol = type;
                }
                //
                // ... Otherwise, this AmbiguousName is reclassified as a
                // PackageName. While the JLS claims a later step
                // determines whether or not a package of that name
                // actually exists, it is pointless to defer the error
                // that long, as a package cannot qualify a method or field
                // access, and a subpackage requires the base package to
                // exist.
                //
                else
                {
                    PackageSymbol* subpackage =
                        package -> FindPackageSymbol(name_symbol);
                    if (! subpackage)
                    {
                        //
                        // One last check in case the subpackage was not
                        // imported.
                        //
                        subpackage =
                            package -> InsertPackageSymbol(name_symbol);
                        control.FindPathsToDirectory(subpackage);
                    }
                    if (subpackage -> directory.Length())
                        name -> symbol = subpackage;
                    else
                    {
                        ReportSemError(SemanticError::UNKNOWN_AMBIGUOUS_NAME,
                                       name, name_symbol -> Name());
                        name -> symbol = control.no_type;
                    }
                }
            }
        }
        // ...Whether the qualifier is a type name, variable, or method
        // call, this is a regular field access
        //
        else
        {
            FindVariableMember(type, name);
            AddDependence(this_type, type, name -> IsConstant());

            //
            // Type substitution for generics: If the base has a parameterized type,
            // and the field's type is a type parameter, substitute it with the
            // corresponding type argument.
            //
            VariableSymbol* field = name -> symbol -> VariableCast();
            if (field && field -> field_declaration)
            {
                // Get the base's parameterized type (if any)
                ParameterizedType* base_param_type = NULL;
                VariableSymbol* base_var = base -> symbol -> VariableCast();
                if (base_var && base_var -> parameterized_type)
                {
                    base_param_type = base_var -> parameterized_type;
                }

                if (base_param_type)
                {
                    TypeSymbol* declaring_class = (TypeSymbol*) field -> owner;
                    // Only check type parameter substitution if the field is declared
                    // in a file we can read tokens from (same compilation unit)
                    if (declaring_class && declaring_class -> file_symbol == source_file_symbol)
                    {
                        AstFieldDeclaration* field_decl = field -> field_declaration -> FieldDeclarationCast();
                        if (field_decl && field_decl -> type)
                        {
                            AstTypeName* field_type_name = field_decl -> type -> TypeNameCast();
                            if (field_type_name && field_type_name -> name && ! field_type_name -> base_opt)
                            {
                                // It's a simple name (not qualified), could be a type parameter
                                // Match by name from the source code
                                const wchar_t* field_type_text = lex_stream -> NameString(field_type_name -> name -> identifier_token);

                                // Compare with class type parameter names
                                for (unsigned i = 0; i < declaring_class -> NumTypeParameters(); i++)
                                {
                                    TypeParameterSymbol* type_param = declaring_class -> TypeParameter(i);
                                    if (wcscmp(field_type_text, type_param -> Name()) == 0)
                                    {
                                        // Match! Substitute with the corresponding type argument
                                        if (i < base_param_type -> NumTypeArguments())
                                        {
                                            Type* type_arg = base_param_type -> TypeArgument(i);
                                            name -> resolved_type = type_arg -> Erasure();
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


void Semantic::ProcessFieldAccess(Ast* expr)
{
    AstFieldAccess* field_access = (AstFieldAccess*) expr;
    assert(! field_access -> base -> NameCast());
    ProcessExpressionOrStringConstant(field_access -> base);

    TypeSymbol* type = field_access -> base -> Type();
    assert(type);
    if (type == control.no_type)
    {
        field_access -> symbol = control.no_type;
        return;
    }
    FindVariableMember(type, field_access);
    AddDependence(ThisType(), type);

    if (field_access -> symbol != control.no_type)
    {
        PackageSymbol* package = field_access -> symbol -> PackageCast();
        if (package)
        {
            ReportSemError(SemanticError::UNKNOWN_AMBIGUOUS_NAME,
                           field_access, package -> PackageName());
            field_access -> symbol = control.no_type;
        }
        else if (field_access -> symbol -> TypeCast())
        {
            type = (TypeSymbol*) field_access -> symbol;
            ReportSemError(SemanticError::TYPE_NOT_FIELD,
                           field_access, type -> Name());
            field_access -> symbol = control.no_type;
        }
        else
        {
            //
            // Either it's not a variable (an error) or the signature of
            // the variable has been typed
            //
            assert(! field_access -> symbol -> VariableCast() ||
                   field_access -> symbol -> VariableCast() -> IsTyped());

            //
            // Type substitution for generics: If the receiver has a parameterized type,
            // and the field's type is a type parameter, substitute it with the
            // corresponding type argument.
            //
            // Note: This code path handles field access through explicit DOT expressions
            // (e.g., obj.field where obj is an expression, not a simple name).
            // Most field access goes through ProcessAmbiguousName instead.
            //
            VariableSymbol* field = field_access -> symbol -> VariableCast();
            if (field && field_access -> base)
            {
                // Get the receiver's parameterized type (if any)
                ParameterizedType* receiver_param_type = NULL;

                // Case 1: Base is a variable with parameterized_type
                VariableSymbol* var = field_access -> base -> symbol -> VariableCast();
                if (var && var -> parameterized_type)
                {
                    receiver_param_type = var -> parameterized_type;
                }
                // Case 2: Base is an expression with resolved_parameterized_type (e.g., method call)
                else if (field_access -> base -> resolved_parameterized_type)
                {
                    receiver_param_type = field_access -> base -> resolved_parameterized_type;
                }

                if (receiver_param_type)
                {
                    TypeSymbol* declaring_class = (TypeSymbol*) field -> owner;
                    if (declaring_class)
                    {
                        bool substituted = false;

                        // First try: Use generic_signature to identify type parameter fields
                        // This works for fields from class files (different compilation units)
                        const char* field_sig = field -> GenericSignatureString();
                        if (field_sig && field_sig[0] == 'T')
                        {
                            // Field signature is in format "T<name>;" - extract the type parameter name
                            int sig_len = strlen(field_sig);
                            if (sig_len > 2 && field_sig[sig_len - 1] == ';')
                            {
                                // Extract the type parameter name (between 'T' and ';')
                                char type_param_name[256];
                                int name_len = sig_len - 2;
                                if (name_len < 256)
                                {
                                    strncpy(type_param_name, field_sig + 1, name_len);
                                    type_param_name[name_len] = '\0';

                                    // Match with the declaring class's type parameters
                                    for (unsigned i = 0; i < declaring_class -> NumTypeParameters(); i++)
                                    {
                                        TypeParameterSymbol* type_param = declaring_class -> TypeParameter(i);
                                        // Compare UTF-8 name
                                        if (strcmp(type_param_name, type_param -> Utf8Name()) == 0)
                                        {
                                            // Match! Substitute with the corresponding type argument
                                            if (i < receiver_param_type -> NumTypeArguments())
                                            {
                                                Type* type_arg = receiver_param_type -> TypeArgument(i);
                                                field_access -> resolved_type = type_arg -> Erasure();
                                                substituted = true;
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        // Second try: Check source tokens if same compilation unit
                        // and not already substituted
                        if (! substituted && field -> field_declaration &&
                            declaring_class -> file_symbol == source_file_symbol)
                        {
                            AstFieldDeclaration* field_decl = field -> field_declaration -> FieldDeclarationCast();
                            if (field_decl && field_decl -> type)
                            {
                                AstTypeName* field_type_name = field_decl -> type -> TypeNameCast();
                                if (field_type_name && field_type_name -> name && ! field_type_name -> base_opt)
                                {
                                    // It's a simple name (not qualified), could be a type parameter
                                    // Match by name from the source code
                                    const wchar_t* field_type_text = lex_stream -> NameString(field_type_name -> name -> identifier_token);

                                    // Compare with class type parameter names
                                    for (unsigned i = 0; i < declaring_class -> NumTypeParameters(); i++)
                                    {
                                        TypeParameterSymbol* type_param = declaring_class -> TypeParameter(i);
                                        if (wcscmp(field_type_text, type_param -> Name()) == 0)
                                        {
                                            // Match! Substitute with the corresponding type argument
                                            if (i < receiver_param_type -> NumTypeArguments())
                                            {
                                                Type* type_arg = receiver_param_type -> TypeArgument(i);
                                                field_access -> resolved_type = type_arg -> Erasure();
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}



} // Close namespace Jopa block
