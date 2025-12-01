// Semantic expression processing - method and constructor lookup
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

namespace Jopa {

bool Semantic::IsIntValueRepresentableInType(AstExpression* expr,
                                             const TypeSymbol* type)
{
    if (! expr -> IsConstant() ||
        ! control.IsSimpleIntegerValueType(expr -> Type()))
    {
        return false;
    }

    IntLiteralValue* literal = DYNAMIC_CAST<IntLiteralValue*> (expr -> value);
    return type == control.int_type || type == control.no_type ||
        (type == control.char_type && (literal -> value >= 0) &&
         (literal -> value <= 65535)) ||
        (type == control.byte_type && (literal -> value >= -128) &&
         (literal -> value <= 127)) ||
        (type == control.short_type && (literal -> value >= -32768) &&
         (literal -> value <= 32767));
}


bool Semantic::IsConstantTrue(AstExpression* expr)
{
    return expr -> IsConstant() && expr -> Type() == control.boolean_type &&
        DYNAMIC_CAST<IntLiteralValue*> (expr -> value) -> value;
}


bool Semantic::IsConstantFalse(AstExpression* expr)
{
    return expr -> IsConstant() && expr -> Type() == control.boolean_type &&
        ! DYNAMIC_CAST<IntLiteralValue*> (expr -> value) -> value;
}


//
// Returns true if source_type can be converted to target_type via subtyping only.
// This does NOT include boxing or unboxing conversions, which is required for
// method specificity comparison according to JLS 15.12.2.5.
//
inline bool Semantic::CanSubtypeConvert(const TypeSymbol* target_type,
                                        const TypeSymbol* source_type)
{
    if (target_type == control.no_type)
        return false;
    if (source_type == control.no_type)
        return true;

    // Primitives: only widening primitive conversion
    if (source_type -> Primitive())
    {
        if (target_type -> Primitive())
            return target_type == source_type ||
                CanWideningPrimitiveConvert(target_type, source_type);
        return false;  // No boxing for specificity
    }

    // References: only subtyping
    if (target_type -> Primitive())
        return false;  // No unboxing for specificity
    return source_type == control.null_type ||
        source_type -> IsSubtype(target_type);
}

inline TypeSymbol* Semantic::EffectiveParameterType(MethodSymbol* method,
                                                    unsigned arg_index,
                                                    unsigned num_arguments)
{
    unsigned num_formals = method -> NumFormalParameters();
    (void) num_arguments;

    if (! method -> ACC_VARARGS())
    {
        if (arg_index >= num_formals)
            return NULL;
        VariableSymbol* param = method -> FormalParameter(arg_index);
        return param ? param -> Type() : NULL;
    }

    if (num_formals == 0)
        return NULL;

    if (arg_index < num_formals - 1)
    {
        VariableSymbol* param = method -> FormalParameter(arg_index);
        return param ? param -> Type() : NULL;
    }

    VariableSymbol* varargs_param = method -> FormalParameter(num_formals - 1);
    if (! varargs_param)
        return NULL;

    TypeSymbol* varargs_type = varargs_param -> Type();
    if (! varargs_type)
        return NULL;

    return varargs_type -> IsArray() ? varargs_type -> ArraySubtype()
                                     : varargs_type;
}

// Returns true if source_method is more specific than target_method, which
// is defined as the type that declared the method, as well as all method
// parameter types, being equal or more specific in the source_method.
// JLS 15.12.2.5: Specificity is determined using subtyping only, NOT
// boxing/unboxing conversions.
//
inline bool Semantic::MoreSpecific(MethodSymbol* source_method,
                                   MethodSymbol* target_method,
                                   unsigned num_arguments)
{
    //
    // Sun bug 4761586: the declaration type is no longer considered when
    // looking for the most specific method.
    //
//      if (! CanMethodInvocationConvert(target_method -> containing_type,
//                                       source_method -> containing_type))
//      {
//          return false;
//      }
    for (unsigned k = 0; k < num_arguments; k++)
    {
        TypeSymbol* target_type = EffectiveParameterType(target_method, k,
                                                         num_arguments);
        TypeSymbol* source_type = EffectiveParameterType(source_method, k,
                                                         num_arguments);
        // Error recovery: parameters might be invalid if there are semantic errors
        if (! target_type || ! source_type)
            return false;
        // Use subtyping only for specificity, not boxing/unboxing
        if (! CanSubtypeConvert(target_type, source_type))
        {
            return false;
        }
    }
    return true;
}


//
// Returns true if a method is more specific than the current set of maximally
// specific methods.
//
inline bool Semantic::MoreSpecific(MethodSymbol* method,
                                   Tuple<MethodSymbol*>& maximally_specific_method,
                                   unsigned num_arguments)
{
    for (unsigned i = 0; i < maximally_specific_method.Length(); i++)
    {
        if (! MoreSpecific(method, maximally_specific_method[i], num_arguments))
            return false;
    }
    return true;
}


//
// Returns true if no method in the current set of maximally specific methods
// is more specific than the given method, meaning that the given method should
// be added to the set.
//
inline bool Semantic::NoMethodMoreSpecific(Tuple<MethodSymbol*>& maximally_specific_method,
                                           MethodSymbol* method,
                                           unsigned num_arguments)
{
    for (unsigned i = 0; i < maximally_specific_method.Length(); i++)
    {
        if (MoreSpecific(maximally_specific_method[i], method, num_arguments))
            return false;
    }
    return true;
}


//
// Returns true if a method is more specific than the current set of maximally
// specific methods.
//
inline bool Semantic::MoreSpecific(MethodSymbol* method,
                                   Tuple<MethodShadowSymbol*>& maximally_specific_method,
                                   unsigned num_arguments)
{
    for (unsigned i = 0; i < maximally_specific_method.Length(); i++)
    {
        if (! MoreSpecific(method,
                           maximally_specific_method[i] -> method_symbol,
                           num_arguments))
            return false;
    }
    return true;
}


//
// Returns true if no method in the current set of maximally specific methods
// is more specific than the given method, meaning that the given method should
// be added to the set.
//
inline bool Semantic::NoMethodMoreSpecific(Tuple<MethodShadowSymbol*>& maximally_specific_method,
                                           MethodSymbol* method,
                                           unsigned num_arguments)
{
    for (unsigned i = 0; i < maximally_specific_method.Length(); i++)
    {
        if (MoreSpecific(maximally_specific_method[i] -> method_symbol, method,
                         num_arguments))
            return false;
    }
    return true;
}


//
// Returns true if a method is applicable by arity (parameter count) for the
// given number of arguments. For varargs methods, the argument count must be
// at least (num_formals - 1), otherwise it must be exactly num_formals.
//
inline bool Semantic::MethodApplicableByArity(MethodSymbol* method,
                                               unsigned num_arguments)
{
    unsigned num_formals = method -> NumFormalParameters();
    if (method -> ACC_VARARGS())
    {
        // Varargs: need at least (num_formals - 1) arguments
        // e.g., method(String... args) has 1 formal, accepts 0+ arguments
        return (num_formals == 0) || (num_arguments >= num_formals - 1);
    }
    else
    {
        // Non-varargs: need exact match
        return num_arguments == num_formals;
    }
}


//
// Creates a new wchar_t[] containing the type of the method or constructor
// overload for printing in Report*NotFound. Caller is responsible for
// calling delete[] on the result.
//
wchar_t* Semantic::Header(const NameSymbol* name, AstArguments* args)
{
    unsigned num_arguments = args -> NumArguments();
    int length = name -> NameLength();
    for (unsigned i = 0; i < num_arguments; i++)
    {
        TypeSymbol* arg_type = args -> Argument(i) -> Type();
        // '.' after package_name; ',' and ' ' to separate this argument
        // from the next one
        length += arg_type -> ContainingPackage() -> PackageNameLength() +
            arg_type -> ExternalNameLength() + 3;
    }

    // +1 for (, +1 for ), +1 for '\0'
    wchar_t* header = new wchar_t[length + 3];
    wchar_t* s = header;
    const wchar_t* s2;

    for (s2 = name -> Name(); *s2; s2++)
        *s++ = *s2;
    *s++ = U_LEFT_PARENTHESIS;
    if (num_arguments > 0)
    {
        for (unsigned i = 0; i < num_arguments; i++)
        {
            TypeSymbol* arg_type = args -> Argument(i) -> Type();

            PackageSymbol* package = arg_type -> ContainingPackage();
            wchar_t* package_name = package -> PackageName();
            if (package -> PackageNameLength() > 0 &&
                package_name[0] != U_DOT)
            {
                while (*package_name)
                {
                    *s++ = (*package_name == U_SLASH ? (wchar_t) U_DOT
                            : *package_name);
                    package_name++;
                }
                *s++ = U_DOT;
            }

            for (s2 = arg_type -> ExternalName(); *s2; s2++)
                *s++ = (*s2 == U_DOLLAR ? (wchar_t) U_DOT : *s2);
            *s++ = U_COMMA;
            *s++ = U_SPACE;
        }

        s -= 2; // remove the last ',' and ' '
    }
    *s++ = U_RIGHT_PARENTHESIS;
    *s = U_NULL;
    return header;
}


//
// Called when no accessible method was found. This checks in order: a hidden
// exact match in an enclosing class (for simple names only); an accessible
// method of the same name but different parameter types, favoring methods with
// the same parameter count; an accessible field by the same name (for no-arg
// call only); an inaccessible method in a superclass; a misspelled method
// name; a type by the same name; and finally no method was found. The
// parameter type should be NULL only if method_call represents a simple name.
//
void Semantic::ReportMethodNotFound(AstMethodInvocation* method_call,
                                    TypeSymbol* type)
{
    AstExpression* base = method_call -> base_opt;
    SemanticEnvironment* env;
    SemanticEnvironment* top_env = state_stack.Top();
    assert((base == NULL) == (type == NULL));

    TokenIndex id_token = method_call -> identifier_token;
    NameSymbol* name_symbol = lex_stream -> NameSymbol(id_token);
    MethodShadowSymbol* method_shadow;

    //
    // First, for simple names, search for a hidden method match in an
    // enclosing class.
    //
    for (env = top_env -> previous; ! base && env; env = env -> previous)
    {
        Tuple<MethodShadowSymbol*> others(2);
        SemanticEnvironment* found_other;
        FindMethodInEnvironment(others, found_other, env, method_call);
        if (others.Length() > 0)
        {
            ReportSemError(SemanticError::HIDDEN_METHOD_IN_ENCLOSING_CLASS,
                           method_call, others[0] -> method_symbol -> Header(),
                           others[0] -> method_symbol -> containing_type -> ContainingPackageName(),
                           others[0] -> method_symbol -> containing_type -> ExternalName());
            return;
        }
    }

    //
    // Search for an accessible method with different arguments. Favor the
    // earliest method found with the smallest difference in parameter count.
    // Since the JVMS limits methods to 255 parameters, we initialize our
    // difference detection with 255.
    //
    MethodSymbol* best_match = NULL;
    for (env = top_env; env;
         env = (base ? (SemanticEnvironment*) NULL : env -> previous))
    {
        if (! base)
            type = env -> Type();
        if (! type -> expanded_method_table)
            ComputeMethodsClosure(type, id_token);
        int difference = 255;
        for (method_shadow = type -> expanded_method_table ->
                 FindMethodShadowSymbol(name_symbol);
             method_shadow; method_shadow = method_shadow -> next_method)
        {
            MethodSymbol* method = method_shadow -> method_symbol;

            if (! method -> IsTyped())
                method -> ProcessMethodSignature(this, id_token);
            if (MemberAccessCheck(type, method, base) ||
                method_shadow -> NumConflicts() > 0)
            {
                int diff = (int)method_call -> arguments -> NumArguments() -
                    (int)method -> NumFormalParameters();
                if (diff < 0)
                    diff = - diff;
                if (diff < difference)
                {
                    best_match = method;
                    difference = diff;
                }
            }
        }
        if (best_match)
        {
            wchar_t* header = Header(name_symbol, method_call -> arguments);
            ReportSemError(SemanticError::METHOD_OVERLOAD_NOT_FOUND,
                           method_call, header,
                           best_match -> containing_type -> ContainingPackageName(),
                           best_match -> containing_type -> ExternalName(),
                           best_match -> Header());
            delete [] header;
            return;
        }
    }

    //
    // For a no-arg method, search for an accessible field of the same name.
    //
    if (method_call -> arguments -> NumArguments() == 0)
    {
        for (env = top_env; env;
             env = (base ? (SemanticEnvironment*) NULL : env -> previous))
        {
            if (! base)
                type = env -> Type();
            if (! type -> expanded_field_table)
                ComputeFieldsClosure(type, id_token);
            VariableShadowSymbol* variable_shadow = type ->
                expanded_field_table -> FindVariableShadowSymbol(name_symbol);
            if (variable_shadow)
            {
                VariableSymbol* variable = variable_shadow -> variable_symbol;
                if (MemberAccessCheck(type, variable))
                {
                    TypeSymbol* enclosing_type =
                        variable -> owner -> TypeCast();
                    assert(enclosing_type);
                    ReportSemError(SemanticError::FIELD_NOT_METHOD,
                                   method_call, variable -> Name(),
                                   enclosing_type -> ContainingPackageName(),
                                   enclosing_type -> ExternalName());
                    return;
                }
            }
        }
    }

    //
    // Check if the method is inaccessible.
    //
    for (TypeSymbol* super_type = type;
         super_type; super_type = super_type -> super)
    {
        for (method_shadow = super_type -> expanded_method_table ->
                 FindMethodShadowSymbol(name_symbol);
             method_shadow; method_shadow = method_shadow -> next_method)
        {
            MethodSymbol* method = method_shadow -> method_symbol;
            if (! method -> IsTyped())
                method -> ProcessMethodSignature(this, id_token);

            if (method_call -> arguments -> NumArguments() ==
                method -> NumFormalParameters())
            {
                unsigned i;
                for (i = 0;
                     i < method_call -> arguments -> NumArguments(); i++)
                {
                    AstExpression* expr =
                        method_call -> arguments -> Argument(i);
                    if (! CanMethodInvocationConvert(method -> FormalParameter(i) -> Type(),
                                                     expr -> Type()))
                    {
                        break;
                    }
                }
                if (i == method_call -> arguments -> NumArguments())
                {
                    //
                    // JLS 9.2: Interfaces do not have protected members,
                    // even though jikes treats interfaces as subtypes of
                    // Object.
                    //
                    if (base && method -> ACC_PROTECTED() &&
                        base -> Type() -> ACC_INTERFACE())
                    {
                        assert(method -> containing_type == control.Object());
                        ReportSemError(SemanticError::PROTECTED_INTERFACE_METHOD_NOT_ACCESSIBLE,
                                       method_call, method -> Header());
                    }
                    //
                    // A protected instance method in the superclass is
                    // inaccessible if the base expression is the wrong type.
                    //
                    else if (method -> ACC_PROTECTED() &&
                             ! method -> ACC_STATIC() &&
                             ThisType() -> HasProtectedAccessTo(method -> containing_type))
                    {
                        assert(base);
                        ReportSemError(SemanticError::PROTECTED_INSTANCE_METHOD_NOT_ACCESSIBLE,
                                       method_call, method -> Header(),
                                       method -> containing_type -> ContainingPackageName(),
                                       method -> containing_type -> ExternalName(),
                                       ThisType() -> ContainingPackageName(),
                                       ThisType() -> ExternalName());
                    }
                    else
                    {
                        ReportSemError(SemanticError::METHOD_NOT_ACCESSIBLE,
                                       method_call, method -> Header(),
                                       method -> containing_type -> ContainingPackageName(),
                                       method -> containing_type -> ExternalName(),
                                       method -> AccessString());
                    }
                    return;
                }
            }
        }
    }

    //
    // Search for a misspelled method name.
    //
    for (env = top_env; env;
         env = (base ? (SemanticEnvironment*) NULL : env -> previous))
    {
        if (! base)
            type = env -> Type();
        best_match = FindMisspelledMethodName(type, method_call, name_symbol);
        if (best_match)
        {
            ReportSemError(SemanticError::METHOD_NAME_MISSPELLED,
                           method_call, name_symbol -> Name(),
                           type -> ContainingPackageName(),
                           type -> ExternalName(), best_match -> Name());
            return;
        }
    }
    //
    // Search for a type of the same name.
    //
    if (FindType(id_token))
        ReportSemError(SemanticError::TYPE_NOT_METHOD, method_call,
                       name_symbol -> Name());
    //
    // Give up. We didn't find it.
    //
    else
    {
        if (! base)
            type = ThisType();
        wchar_t* header = Header(name_symbol, method_call -> arguments);
        ReportSemError(SemanticError::METHOD_NOT_FOUND, method_call,
                       header, type -> ContainingPackageName(),
                       type -> ExternalName());
        delete [] header;
    }
}


//
// Called when no accessible constructor was found. This checks in order: an
// accessible method of the same name but different parameters, favoring
// constructors with the same parameter count; an inaccessible constructor;
// an accessible method with the same name as the type; and finally no
// constructor was found.
//
void Semantic::ReportConstructorNotFound(Ast* ast, TypeSymbol* type)
{
    AstClassCreationExpression* class_creation =
        ast -> ClassCreationExpressionCast();
    AstSuperCall* super_call = ast -> SuperCallCast();
    AstArguments* args;
    TokenIndex left_tok;

    if (class_creation)
    {
        args = class_creation -> arguments;
        left_tok = class_creation -> new_token;
        if (class_creation -> class_body_opt)
            class_creation = NULL;
    }
    else if (super_call)
    {
        args = super_call -> arguments;
        left_tok = super_call -> super_token;
    }
    else
    {
        AstThisCall* this_call = ast -> ThisCallCast();
        assert(this_call);
        args = this_call -> arguments;
        left_tok = this_call -> this_token;
    }
    unsigned num_arguments = args -> NumArguments();
    TokenIndex right_tok = args -> right_parenthesis_token;

    //
    // Search for an accessible constructor with different arguments. Favor
    // the earliest ctor found with the smallest difference in parameter count.
    // Since the JVMS limits methods to 255 parameters, we initialize our
    // difference detection with 255.
    //
    MethodSymbol* best_match = NULL;
    MethodSymbol* ctor;
    int difference = 255;
    for (ctor = type -> FindMethodSymbol(control.init_name_symbol);
         ctor; ctor = ctor -> next_method)
    {
        if (ConstructorAccessCheck(ctor, ! class_creation))
        {
            int diff = (int)num_arguments - (int)ctor -> NumFormalParameters();
            if (diff < 0)
                diff = - diff;
            if (diff < difference)
            {
                best_match = ctor;
                difference = diff;
            }
        }
    }
    if (best_match)
    {
        wchar_t* header = Header(type -> Identity(), args);
        ReportSemError(SemanticError::CONSTRUCTOR_OVERLOAD_NOT_FOUND, ast,
                       header, type -> ContainingPackageName(),
                       type -> ExternalName(), best_match -> Header());
        delete [] header;
        return;
    }

    //
    // Check if the constructor is inaccessible.
    //
    for (ctor = type -> FindMethodSymbol(control.init_name_symbol);
         ctor; ctor = ctor -> next_method)
    {
        if (num_arguments == ctor -> NumFormalParameters())
        {
            unsigned i;
            for (i = 0; i < num_arguments; i++)
            {
                AstExpression* expr = args -> Argument(i);
                if (! CanMethodInvocationConvert(ctor -> FormalParameter(i) -> Type(),
                                                 expr -> Type()))
                {
                    break;
                }
            }
            if (i == num_arguments) // found a match?
            {
                ReportSemError(SemanticError::CONSTRUCTOR_NOT_ACCESSIBLE, ast,
                               ctor -> Header(),
                               type -> ContainingPackageName(),
                               type -> ExternalName(), ctor -> AccessString());
                return;
            }
        }
    }

    //
    // Search for an accessible method with the same name as the type.
    //
    MethodSymbol* method;
    for (method = type -> FindMethodSymbol(type -> Identity());
         method; method = method -> next_method)
    {
        if (! method -> IsTyped())
            method -> ProcessMethodSignature(this, right_tok);

        if (num_arguments == method -> NumFormalParameters())
        {
            unsigned i;
            for (i = 0; i < num_arguments; i++)
            {
                if (! CanMethodInvocationConvert(method -> FormalParameter(i) -> Type(),
                                                 args -> Argument(i) -> Type()))
                {
                    break;
                }
            }
            if (i == num_arguments)
                break;
        }
    }
    if (method)
    {
        if (method -> declaration)
        {
            AstMethodDeclaration* method_declaration =
                (AstMethodDeclaration*) method -> declaration;
            FileLocation loc((method -> containing_type ->
                              semantic_environment -> sem -> lex_stream),
                             (method_declaration -> method_declarator ->
                              identifier_token));
            ReportSemError(SemanticError::METHOD_FOUND_FOR_CONSTRUCTOR,
                           left_tok, right_tok, type -> Name(),
                           loc.location);
        }
        else
        {
            ReportSemError(SemanticError::METHOD_FOUND_FOR_CONSTRUCTOR,
                           left_tok, right_tok, type -> Name(),
                           method -> containing_type -> file_location -> location);
        }
        return;
    }

    //
    // Give up. We didn't find it.
    //
    wchar_t* header = Header(type -> Identity(), args);
    ReportSemError(SemanticError::CONSTRUCTOR_NOT_FOUND, ast, header,
                   type -> ContainingPackageName(), type -> ExternalName());
    delete [] header;
}


MethodSymbol* Semantic::FindConstructor(TypeSymbol* containing_type, Ast* ast,
                                        TokenIndex left_tok,
                                        TokenIndex right_tok)
{
    if (containing_type == control.no_type)
        return NULL;

    //
    // If this type is anonymous, we have just generated the constructor,
    // so we know it is the right one.
    //
    if (containing_type -> Anonymous())
    {
        return containing_type -> declaration -> default_constructor ->
            constructor_symbol;
    }

    AstArguments* args;
    Tuple<MethodSymbol*> constructor_set(2); // Stores constructor overloads.

    AstClassCreationExpression* class_creation =
        ast -> ClassCreationExpressionCast();
    AstSuperCall* super_call = ast -> SuperCallCast();

    if (class_creation)
    {
        args = class_creation -> arguments;
        if (class_creation -> class_body_opt)
            class_creation = NULL;
    }
    else if (super_call)
        args = super_call -> arguments;
    else
    {
        AstThisCall* this_call = ast -> ThisCallCast();
        assert(this_call);
        args = this_call -> arguments;
    }

    unsigned num_arguments = args -> NumArguments();
    assert(containing_type -> ConstructorMembersProcessed());
    MethodSymbol* ctor;

    // JLS 15.12.2: Two-phase constructor resolution
    // Phase 1: Find applicable constructors using subtyping only (no boxing/unboxing)
    for (ctor = containing_type -> FindMethodSymbol(control.init_name_symbol);
         ctor; ctor = ctor -> next_method)
    {
        if (! ctor -> IsTyped())
            ctor -> ProcessMethodSignature(this, right_tok);

        if (num_arguments == ctor -> NumFormalParameters() &&
            ConstructorAccessCheck(ctor, ! class_creation))
        {
            unsigned i;
            for (i = 0; i < num_arguments; i++)
            {
                // Phase 1 uses subtyping only
                if (! CanSubtypeConvert(ctor -> FormalParameter(i) -> Type(),
                                        args -> Argument(i) -> Type()))
                {
                    break;
                }
            }
            if (i == num_arguments)
            {
                if (MoreSpecific(ctor, constructor_set, num_arguments))
                {
                    constructor_set.Reset();
                    constructor_set.Next() = ctor;
                }
                else if (NoMethodMoreSpecific(constructor_set, ctor, num_arguments))
                    constructor_set.Next() = ctor;
            }
        }
    }

    // Phase 2: If Phase 1 found nothing, try with boxing/unboxing
    if (constructor_set.Length() == 0 && control.option.source >= JopaOption::SDK1_5)
    {
        for (ctor = containing_type -> FindMethodSymbol(control.init_name_symbol);
             ctor; ctor = ctor -> next_method)
        {
            if (! ctor -> IsTyped())
                ctor -> ProcessMethodSignature(this, right_tok);

            if (num_arguments == ctor -> NumFormalParameters() &&
                ConstructorAccessCheck(ctor, ! class_creation))
            {
                unsigned i;
                for (i = 0; i < num_arguments; i++)
                {
                    // Phase 2 allows boxing/unboxing
                    if (! CanMethodInvocationConvert(ctor -> FormalParameter(i) -> Type(),
                                                     args -> Argument(i) -> Type()))
                    {
                        break;
                    }
                }
                if (i == num_arguments)
                {
                    if (MoreSpecific(ctor, constructor_set, num_arguments))
                    {
                        constructor_set.Reset();
                        constructor_set.Next() = ctor;
                    }
                    else if (NoMethodMoreSpecific(constructor_set, ctor, num_arguments))
                        constructor_set.Next() = ctor;
                }
            }
        }
    }

    // Phase 3: Varargs constructors
    if (constructor_set.Length() == 0 && control.option.source >= JopaOption::SDK1_5)
    {
        for (ctor = containing_type -> FindMethodSymbol(control.init_name_symbol);
             ctor; ctor = ctor -> next_method)
        {
            if (! ctor -> IsTyped())
                ctor -> ProcessMethodSignature(this, right_tok);

            bool is_varargs = ctor -> ACC_VARARGS();
            if (! is_varargs)
                continue;

            unsigned num_formals = ctor -> NumFormalParameters();
            if (num_formals == 0 || ! ConstructorAccessCheck(ctor, ! class_creation))
                continue;

            // Check that we have at least num_formals - 1 args (the fixed params)
            unsigned num_fixed = num_formals - 1;
            if (num_arguments < num_fixed)
                continue;

            unsigned i;
            // Check fixed parameters
            for (i = 0; i < num_fixed; i++)
            {
                if (! CanMethodInvocationConvert(ctor -> FormalParameter(i) -> Type(),
                                                 args -> Argument(i) -> Type()))
                {
                    break;
                }
            }
            if (i < num_fixed)
                continue;

            // Check varargs parameters
            TypeSymbol* varargs_type = ctor -> FormalParameter(num_formals - 1) -> Type();
            TypeSymbol* component_type = varargs_type -> IsArray() ?
                varargs_type -> ArraySubtype() : varargs_type;

            // Special case: when num_arguments == num_formals, the last argument
            // can also be assignable to the varargs array type itself
            if (num_arguments == num_formals && i == num_fixed)
            {
                AstExpression* last_arg = args -> Argument(i);
                if (CanMethodInvocationConvert(varargs_type, last_arg -> Type()))
                {
                    i++; // Accept this argument as array
                }
            }

            // Check remaining varargs arguments against component type
            for ( ; i < num_arguments; i++)
            {
                AstExpression* expr = args -> Argument(i);
                if (! CanMethodInvocationConvert(component_type, expr -> Type()))
                {
                    break;
                }
            }

            if (i == num_arguments)
            {
                if (MoreSpecific(ctor, constructor_set, num_arguments))
                {
                    constructor_set.Reset();
                    constructor_set.Next() = ctor;
                }
                else if (NoMethodMoreSpecific(constructor_set, ctor, num_arguments))
                    constructor_set.Next() = ctor;
            }
        }
    }

    if (constructor_set.Length() == 0)
    {
        if (! containing_type -> Bad() || NumErrors() == 0)
            ReportConstructorNotFound(ast, containing_type);
        return NULL;
    }
    if (constructor_set.Length() > 1)
    {
        // Don't report ambiguity if any argument has no_type - there's already
        // an error upstream that caused this spurious ambiguity
        bool has_bad_arg = false;
        for (unsigned i = 0; i < num_arguments && !has_bad_arg; i++)
        {
            if (args -> Argument(i) -> Type() == control.no_type)
                has_bad_arg = true;
        }
        if (!has_bad_arg)
        {
            ReportSemError(SemanticError::AMBIGUOUS_CONSTRUCTOR_INVOCATION,
                           left_tok, right_tok, containing_type -> Name(),
                           constructor_set[0] -> Header(),
                           constructor_set[1] -> Header());
        }
    }

    ctor = constructor_set[0];
    if (ctor -> ACC_SYNTHETIC())
    {
        ReportSemError(SemanticError::SYNTHETIC_CONSTRUCTOR_INVOCATION,
                       left_tok, right_tok, ctor -> Header(),
                       containing_type -> ContainingPackageName(),
                       containing_type -> ExternalName());
    }

    //
    // If this constructor came from a class file, make sure that its throws
    // clause has been processed.
    //
    ctor -> ProcessMethodThrows(this, right_tok);

    if (control.option.deprecation && ctor -> IsDeprecated() &&
        ! InDeprecatedContext())
    {
        ReportSemError(SemanticError::DEPRECATED_CONSTRUCTOR,
                       left_tok, right_tok, ctor -> Header(),
                       ctor -> containing_type -> ContainingPackageName(),
                       ctor -> containing_type -> ExternalName());
    }
    return ctor;
}


//
//
//
VariableSymbol* Semantic::FindMisspelledVariableName(TypeSymbol* type,
                                                     AstExpression* expr)
{
    AstFieldAccess* field_access = expr -> FieldAccessCast();
    AstName* field_name = expr -> NameCast();
    AstExpression* base =
        field_name ? field_name -> base_opt : field_access -> base;
    VariableSymbol* misspelled_variable = NULL;
    int index = 0;
    TokenIndex identifier_token = expr -> RightToken();
    const wchar_t* name = lex_stream -> NameString(identifier_token);

    for (unsigned k = 0;
         k < type -> expanded_field_table -> symbol_pool.Length(); k++)
    {
        VariableShadowSymbol* variable_shadow =
            type -> expanded_field_table -> symbol_pool[k];
        VariableSymbol* variable = variable_shadow -> variable_symbol;
        if (! variable -> IsTyped())
            variable -> ProcessVariableSignature(this, identifier_token);
        if (! MemberAccessCheck(type, variable, base))
            variable = NULL;
        for (unsigned i = 0;
             ! variable && i < variable_shadow -> NumConflicts(); i++)
        {
            variable = variable_shadow -> Conflict(i);
            if (! variable -> IsTyped())
                variable -> ProcessVariableSignature(this,
                                                     identifier_token);
            if (! MemberAccessCheck(type, variable, base))
                variable = NULL;
        }

        if (variable)
        {
            int new_index = Spell::Index(name, variable -> Name());
            if (new_index > index)
            {
                misspelled_variable = variable;
                index = new_index;
            }
        }
    }

    int length = wcslen(name);
    return (length == 3 && index >= 5) ||
        (length == 4 && index >= 6) ||
        (length >= 5 && index >= 7)
        ? misspelled_variable : (VariableSymbol*) NULL;
}

//
//
//
MethodSymbol* Semantic::FindMisspelledMethodName(TypeSymbol* type,
                                                 AstMethodInvocation* method_call,
                                                 NameSymbol* name_symbol)
{
    AstExpression* base = method_call -> base_opt;
    MethodSymbol* misspelled_method = NULL;
    int index = 0;
    TokenIndex identifier_token = method_call -> identifier_token;

    for (unsigned k = 0;
         k < type -> expanded_method_table -> symbol_pool.Length(); k++)
    {
        MethodShadowSymbol* method_shadow =
            type -> expanded_method_table -> symbol_pool[k];
        MethodSymbol* method = method_shadow -> method_symbol;

        if (! method -> IsTyped())
            method -> ProcessMethodSignature(this, identifier_token);

        if ((method_call -> arguments -> NumArguments() ==
             method -> NumFormalParameters()) &&
            (MemberAccessCheck(type, method, base) ||
             method_shadow -> NumConflicts() > 0))
        {
            unsigned i;
            for (i = 0; i < method_call -> arguments -> NumArguments(); i++)
            {
                AstExpression* expr = method_call -> arguments -> Argument(i);
                if (! CanMethodInvocationConvert(method -> FormalParameter(i) -> Type(),
                                                 expr -> Type()))
                {
                    break;
                }
            }
            if (i == method_call -> arguments -> NumArguments())
            {
                int new_index = Spell::Index(name_symbol -> Name(),
                                             method -> Name());
                if (new_index > index)
                {
                    misspelled_method = method;
                    index = new_index;
                }
            }
        }
    }

    int length = name_symbol -> NameLength();
    int num_args = method_call -> arguments -> NumArguments();

    //
    // If we have a name of length 2, accept >= 30% probality if the function
    // takes at least one argument. If we have a name of length 3,
    // accept >= 50% probality if the function takes at least one argument.
    // Otherwise, if the length of the name is > 3, accept >= 60% probability.
    //
    return index < 3 ? (MethodSymbol*) NULL
        : ((length == 2 && (index >= 3 || num_args > 0)) ||
           (length == 3 && (index >= 5 || num_args > 0)) ||
           (length  > 3 && (index >= 6 || (index >= 5 && num_args > 0))))
        ? misspelled_method : (MethodSymbol*) NULL;
}


//
// Search the type in question for a method. Note that name_symbol is an
// optional argument. If it was not passed to this function then its default
// value is NULL (see semantic.h) and we assume that the name to search for
// is the name specified in the field_access of the method_call.
//
MethodShadowSymbol* Semantic::FindMethodInType(TypeSymbol* type,
                                               AstMethodInvocation* method_call,
                                               NameSymbol* name_symbol)
{
    Tuple<MethodShadowSymbol*> method_set(2); // Stores method overloads.
    AstExpression* base = method_call -> base_opt;
    TokenIndex id_token = method_call -> identifier_token;
    assert(base);
    if (! name_symbol)
        name_symbol = lex_stream -> NameSymbol(id_token);
    if (! type -> expanded_method_table)
        ComputeMethodsClosure(type, id_token);

    //
    // Here, we ignore any conflicts in a method declaration. If there are
    // conflicts, they are necessarily abstract methods inherited from
    // interfaces, so either the original method implements them all, or it
    // is also abstract and we are free to choose which one to use.
    //

    // JLS 15.12.2: Three-phase method resolution
    // Phase 1: Non-varargs methods using subtyping only (no boxing/unboxing)
    for (MethodShadowSymbol* method_shadow = type -> expanded_method_table ->
             FindMethodShadowSymbol(name_symbol);
         method_shadow; method_shadow = method_shadow -> next_method)
    {
        MethodSymbol* method = method_shadow -> method_symbol;

        if (! method -> IsTyped())
            method -> ProcessMethodSignature(this, id_token);

        unsigned num_args = method_call -> arguments -> NumArguments();
        unsigned num_formals = method -> NumFormalParameters();
        bool is_varargs = method -> ACC_VARARGS();

        // Phase 1 skips varargs methods
        if (is_varargs)
            continue;

        if (num_args == num_formals &&
            (MemberAccessCheck(type, method, base) ||
             method_shadow -> NumConflicts() > 0))
        {
            unsigned i;
            for (i = 0; i < num_formals; i++)
            {
                AstExpression* expr = method_call -> arguments -> Argument(i);
                // Phase 1 uses subtyping only
                if (! CanSubtypeConvert(method -> FormalParameter(i) -> Type(),
                                        expr -> Type()))
                {
                    break;
                }
            }

            if (i == num_formals)
            {
                if (MoreSpecific(method, method_set, num_args))
                {
                    method_set.Reset();
                    method_set.Next() = method_shadow;
                }
                else if (NoMethodMoreSpecific(method_set, method, num_args))
                    method_set.Next() = method_shadow;
            }
        }
    }

    // Phase 2: Non-varargs methods with boxing/unboxing
    if (method_set.Length() == 0 && control.option.source >= JopaOption::SDK1_5)
    {
        for (MethodShadowSymbol* method_shadow = type -> expanded_method_table ->
                 FindMethodShadowSymbol(name_symbol);
             method_shadow; method_shadow = method_shadow -> next_method)
        {
            MethodSymbol* method = method_shadow -> method_symbol;

            if (! method -> IsTyped())
                method -> ProcessMethodSignature(this, id_token);

            unsigned num_args = method_call -> arguments -> NumArguments();
            unsigned num_formals = method -> NumFormalParameters();
            bool is_varargs = method -> ACC_VARARGS();

            // Phase 2 skips varargs methods
            if (is_varargs)
                continue;

            if (num_args == num_formals &&
                (MemberAccessCheck(type, method, base) ||
                 method_shadow -> NumConflicts() > 0))
            {
                unsigned i;
                for (i = 0; i < num_formals; i++)
                {
                    AstExpression* expr = method_call -> arguments -> Argument(i);
                    // Phase 2 allows boxing/unboxing
                    if (! CanMethodInvocationConvert(method -> FormalParameter(i) -> Type(),
                                                     expr -> Type()))
                    {
                        break;
                    }
                }

                if (i == num_formals)
                {
                    if (MoreSpecific(method, method_set, num_args))
                    {
                        method_set.Reset();
                        method_set.Next() = method_shadow;
                    }
                    else if (NoMethodMoreSpecific(method_set, method, num_args))
                        method_set.Next() = method_shadow;
                }
            }
        }
    }

    // Phase 3: Varargs methods with boxing/unboxing
    if (method_set.Length() == 0)
    {
        for (MethodShadowSymbol* method_shadow = type -> expanded_method_table ->
                 FindMethodShadowSymbol(name_symbol);
             method_shadow; method_shadow = method_shadow -> next_method)
        {
            MethodSymbol* method = method_shadow -> method_symbol;

            if (! method -> IsTyped())
                method -> ProcessMethodSignature(this, id_token);

            unsigned num_args = method_call -> arguments -> NumArguments();
            if (MethodApplicableByArity(method, num_args) &&
                (MemberAccessCheck(type, method, base) ||
                 method_shadow -> NumConflicts() > 0))
            {
                unsigned i;
                unsigned num_formals = method -> NumFormalParameters();
                bool is_varargs = method -> ACC_VARARGS();

                // Phase 3 only considers varargs methods
                if (! is_varargs)
                    continue;

                // Check fixed parameters
                unsigned num_fixed = num_formals - 1;
                for (i = 0; i < num_fixed && i < num_args; i++)
                {
                    AstExpression* expr = method_call -> arguments -> Argument(i);
                    if (! CanMethodInvocationConvert(method -> FormalParameter(i) -> Type(),
                                                     expr -> Type()))
                    {
                        break;
                    }
                }

                // Check varargs parameters against component type (or array type)
                if (i == num_fixed && num_formals > 0)
                {
                    TypeSymbol* varargs_type = method -> FormalParameter(num_formals - 1) -> Type();
                    TypeSymbol* component_type = varargs_type -> IsArray() ?
                        varargs_type -> ArraySubtype() : varargs_type;

                    // Special case: when num_args == num_formals, the last argument
                    // can also be assignable to the varargs array type itself
                    // (JLS 15.12.2.4 - no wrapping needed in that case)
                    if (num_args == num_formals && i == num_fixed)
                    {
                        AstExpression* last_arg = method_call -> arguments -> Argument(i);
                        if (CanMethodInvocationConvert(varargs_type, last_arg -> Type()))
                        {
                            i++; // Accept this argument as array
                        }
                    }

                    // Check remaining varargs arguments against component type
                    for ( ; i < num_args; i++)
                    {
                        AstExpression* expr = method_call -> arguments -> Argument(i);
                        if (! CanMethodInvocationConvert(component_type, expr -> Type()))
                        {
                            break;
                        }
                    }
                }

                if (i == num_args)
                {
                    if (MoreSpecific(method, method_set, num_args))
                    {
                        method_set.Reset();
                        method_set.Next() = method_shadow;
                    }
                    else if (NoMethodMoreSpecific(method_set, method, num_args))
                        method_set.Next() = method_shadow;
                }
            }
        }
    }

    if (method_set.Length() == 0)
    {
        ReportMethodNotFound(method_call, type);
        return NULL;
    }
    else if (method_set.Length() > 1)
    {
        ReportSemError(SemanticError::AMBIGUOUS_METHOD_INVOCATION,
                       method_call, name_symbol -> Name(),
                       method_set[0] -> method_symbol -> Header(),
                       method_set[0] -> method_symbol -> containing_type -> ContainingPackageName(),
                       method_set[0] -> method_symbol -> containing_type -> ExternalName(),
                       method_set[1] -> method_symbol -> Header(),
                       method_set[1] -> method_symbol -> containing_type -> ContainingPackageName(),
                       method_set[1] -> method_symbol -> containing_type -> ExternalName());
    }

    MethodSymbol* method = method_set[0] -> method_symbol;
    if (method -> ACC_SYNTHETIC() && ! method -> ACC_BRIDGE())
    {
        // Allow enum synthetic methods (values, valueOf) to be called
        bool is_enum_synthetic = method -> containing_type -> IsEnum() &&
            (wcscmp(method -> Name(), L"values") == 0 ||
             wcscmp(method -> Name(), L"valueOf") == 0);

        if (!is_enum_synthetic)
        {
            ReportSemError(SemanticError::SYNTHETIC_METHOD_INVOCATION,
                           method_call, method -> Header(),
                           method -> containing_type -> ContainingPackageName(),
                           method -> containing_type -> ExternalName());
        }
    }

    //
    // If this method came from a class file, make sure that its throws clause
    // has been processed.
    //
    method -> ProcessMethodThrows(this, id_token);

    if (control.option.deprecation && method -> IsDeprecated() &&
        ! InDeprecatedContext())
    {
        ReportSemError(SemanticError::DEPRECATED_METHOD, method_call,
                       method -> Header(),
                       method -> containing_type -> ContainingPackageName(),
                       method -> containing_type -> ExternalName());
    }
    return method_set[0];
}


void Semantic::FindMethodInEnvironment(Tuple<MethodShadowSymbol*>& methods_found,
                                       SemanticEnvironment*& where_found,
                                       SemanticEnvironment* envstack,
                                       AstMethodInvocation* method_call)
{
    assert(! method_call -> base_opt);
    TokenIndex id_token = method_call -> identifier_token;
    NameSymbol* name_symbol = lex_stream -> NameSymbol(id_token);

    for (SemanticEnvironment* env = envstack; env; env = env -> previous)
    {
        TypeSymbol* type = env -> Type();
        if (! type -> expanded_method_table)
            ComputeMethodsClosure(type, id_token);

        methods_found.Reset();
        where_found = NULL;

        //
        // If this environment contained a method with the right name, the
        // search stops:
        //
        //    "Class scoping does not influence overloading: if the inner
        //     class has one print method, the simple method name 'print'
        //     refers to that method, not any of the ten 'print' methods in
        //     the enclosing class."
        //
        MethodShadowSymbol* method_shadow = type -> expanded_method_table ->
            FindMethodShadowSymbol(name_symbol);
        if (method_shadow)
        {
            // JLS 15.12.2: Three-phase method resolution
            // Phase 1: Non-varargs methods using subtyping only (no boxing/unboxing)
            for (MethodShadowSymbol* shadow = method_shadow; shadow;
                  shadow = shadow -> next_method)
            {
                MethodSymbol* method = shadow -> method_symbol;

                if (! method -> IsTyped())
                    method -> ProcessMethodSignature(this, id_token);

                unsigned num_args = method_call -> arguments -> NumArguments();
                unsigned num_formals = method -> NumFormalParameters();
                bool is_varargs = method -> ACC_VARARGS();

                // Phase 1 skips varargs methods
                if (is_varargs)
                    continue;

                if (num_args == num_formals)
                {
                    unsigned i;
                    for (i = 0; i < num_formals; i++)
                    {
                        AstExpression* expr = method_call -> arguments -> Argument(i);
                        // Phase 1 uses subtyping only
                        if (! CanSubtypeConvert(method -> FormalParameter(i) -> Type(),
                                                expr -> Type()))
                        {
                            break;
                        }
                    }

                    if (i == num_formals)
                    {
                        if (MoreSpecific(method, methods_found, num_args))
                        {
                            methods_found.Reset();
                            methods_found.Next() = shadow;
                        }
                        else if (NoMethodMoreSpecific(methods_found, method, num_args))
                            methods_found.Next() = shadow;
                    }
                }
            }

            // Phase 2: Non-varargs methods with boxing/unboxing
            if (methods_found.Length() == 0 && control.option.source >= JopaOption::SDK1_5)
            {
                for (MethodShadowSymbol* shadow = method_shadow; shadow;
                      shadow = shadow -> next_method)
                {
                    MethodSymbol* method = shadow -> method_symbol;

                    if (! method -> IsTyped())
                        method -> ProcessMethodSignature(this, id_token);

                    unsigned num_args = method_call -> arguments -> NumArguments();
                    unsigned num_formals = method -> NumFormalParameters();
                    bool is_varargs = method -> ACC_VARARGS();

                    // Phase 2 skips varargs methods
                    if (is_varargs)
                        continue;

                    if (num_args == num_formals)
                    {
                        unsigned i;
                        for (i = 0; i < num_formals; i++)
                        {
                            AstExpression* expr = method_call -> arguments -> Argument(i);
                            // Phase 2 allows boxing/unboxing
                            if (! CanMethodInvocationConvert(method -> FormalParameter(i) -> Type(),
                                                             expr -> Type()))
                            {
                                break;
                            }
                        }

                        if (i == num_formals)
                        {
                            if (MoreSpecific(method, methods_found, num_args))
                            {
                                methods_found.Reset();
                                methods_found.Next() = shadow;
                            }
                            else if (NoMethodMoreSpecific(methods_found, method, num_args))
                                methods_found.Next() = shadow;
                        }
                    }
                }
            }

            // Phase 3: Varargs methods with boxing/unboxing
            if (methods_found.Length() == 0)
            {
                for (MethodShadowSymbol* shadow = method_shadow; shadow;
                      shadow = shadow -> next_method)
                {
                    MethodSymbol* method = shadow -> method_symbol;

                    if (! method -> IsTyped())
                        method -> ProcessMethodSignature(this, id_token);

                    unsigned num_args = method_call -> arguments -> NumArguments();
                    unsigned num_formals = method -> NumFormalParameters();
                    bool is_varargs = method -> ACC_VARARGS();

                    // Phase 3 only considers varargs methods
                    if (! is_varargs)
                        continue;

                    if (MethodApplicableByArity(method, num_args))
                    {
                        unsigned i;

                        // Check fixed parameters
                        unsigned num_fixed = num_formals - 1;
                        for (i = 0; i < num_fixed && i < num_args; i++)
                        {
                            AstExpression* expr = method_call -> arguments -> Argument(i);
                            if (! CanMethodInvocationConvert(method -> FormalParameter(i) -> Type(),
                                                             expr -> Type()))
                            {
                                break;
                            }
                        }

                        // Check varargs parameters against component type (or array type)
                        if (i == num_fixed && num_formals > 0)
                        {
                            TypeSymbol* varargs_type = method -> FormalParameter(num_formals - 1) -> Type();
                            TypeSymbol* component_type = varargs_type -> IsArray() ?
                                varargs_type -> ArraySubtype() : varargs_type;

                            // Special case: when num_args == num_formals, the last argument
                            // can also be assignable to the varargs array type itself
                            // (JLS 15.12.2.4 - no wrapping needed in that case)
                            if (num_args == num_formals && i == num_fixed)
                            {
                                AstExpression* last_arg = method_call -> arguments -> Argument(i);
                                if (CanMethodInvocationConvert(varargs_type, last_arg -> Type()))
                                {
                                    i++; // Accept this argument as array
                                }
                            }

                            // Check remaining varargs arguments against component type
                            for ( ; i < num_args; i++)
                            {
                                AstExpression* expr = method_call -> arguments -> Argument(i);
                                if (! CanMethodInvocationConvert(component_type, expr -> Type()))
                                {
                                    break;
                                }
                            }
                        }

                        if (i == num_args)
                        {
                            if (MoreSpecific(method, methods_found, num_args))
                            {
                                methods_found.Reset();
                                methods_found.Next() = shadow;
                            }
                            else if (NoMethodMoreSpecific(methods_found, method, num_args))
                                methods_found.Next() = shadow;
                        }
                    }
                }
            }

            //
            // If a match was found, save the environment
            //
            where_found = (methods_found.Length() > 0 ? env
                           : (SemanticEnvironment*) NULL);
            break;
        }
    }

    // Java 5: If not found in environment, check single static imports
    if (methods_found.Length() == 0 &&
        control.option.source >= JopaOption::SDK1_5)
    {
        for (unsigned i = 0; i < single_static_imports.Length(); i++)
        {
            StaticImportInfo* info = single_static_imports[i];
            if (!info || !info -> type || !info -> member_name)
                continue;

            if (info -> member_name != name_symbol)
                continue;

            // Lazy resolution: look up the method in the type now
            TypeSymbol* import_type = info -> type;

            // Skip if type is bad or not ready
            if (import_type -> Bad() || import_type -> SourcePending())
                continue;

            // Make sure the type has been processed enough to have members
            if (!import_type -> MethodMembersProcessed())
                continue;

            // Compute method closure if needed
            if (!import_type -> expanded_method_table)
                ComputeMethodsClosure(import_type, id_token);

            // Double-check the table exists
            if (!import_type -> expanded_method_table)
                continue;

            MethodShadowSymbol* method_shadow =
                import_type -> expanded_method_table -> FindMethodShadowSymbol(name_symbol);
            if (!method_shadow)
                continue;

            // Check each method in the shadow (handle overloading)
            for (MethodShadowSymbol* shadow = method_shadow; shadow; shadow = shadow -> next_method)
            {
                MethodSymbol* method = shadow -> method_symbol;
                if (!method || !method -> ACC_STATIC())
                    continue;

                // Process method signature if needed
                if (!method -> IsTyped())
                    method -> ProcessMethodSignature(this, id_token);

                // Check if method is applicable to the arguments
                unsigned num_args = method_call -> arguments -> NumArguments();
                if (!MethodApplicableByArity(method, num_args))
                    continue;

                methods_found.Next() = shadow;
                // where_found remains NULL for static imports
                break; // Found one matching method from this import
            }
        }

        // If not found in single imports, check static-on-demand imports
        if (methods_found.Length() == 0)
        {
            for (unsigned i = 0; i < static_on_demand_imports.Length(); i++)
            {
                TypeSymbol* import_type = static_on_demand_imports[i];

                // Skip if type is bad or not ready
                if (!import_type || import_type -> Bad() || import_type -> SourcePending())
                    continue;

                // Make sure the type has been processed enough to have members
                if (!import_type -> MethodMembersProcessed())
                    continue;

                // Compute method closure if needed
                if (!import_type -> expanded_method_table)
                    ComputeMethodsClosure(import_type, id_token);

                // Double-check the table exists
                if (!import_type -> expanded_method_table)
                    continue;

                MethodShadowSymbol* method_shadow =
                    import_type -> expanded_method_table -> FindMethodShadowSymbol(name_symbol);
                if (!method_shadow)
                    continue;

                // Check each method in the shadow (handle overloading)
                for (MethodShadowSymbol* shadow = method_shadow; shadow; shadow = shadow -> next_method)
                {
                    MethodSymbol* method = shadow -> method_symbol;
                    if (!method || !method -> ACC_STATIC())
                        continue;

                    // Process method signature if needed
                    if (!method -> IsTyped())
                        method -> ProcessMethodSignature(this, id_token);

                    // Check if method is applicable to the arguments
                    unsigned num_args = method_call -> arguments -> NumArguments();
                    if (!MethodApplicableByArity(method, num_args))
                        continue;

                    methods_found.Next() = shadow;
                    // where_found remains NULL for static imports
                    break; // Found one matching method from this import
                }
            }
        }
    }
}


MethodShadowSymbol* Semantic::FindMethodInEnvironment(SemanticEnvironment*& where_found,
                                                      AstMethodInvocation* method_call)
{
    Tuple<MethodShadowSymbol*> methods_found(2);
    FindMethodInEnvironment(methods_found, where_found, state_stack.Top(),
                            method_call);
    if (methods_found.Length() == 0)
    {
        ReportMethodNotFound(method_call, NULL);
        return NULL;
    }
    MethodSymbol* method_symbol =  methods_found[0] -> method_symbol;
    for (unsigned i = 1; i < methods_found.Length(); i++)
    {
        ReportSemError(SemanticError::AMBIGUOUS_METHOD_INVOCATION,
                       method_call, method_symbol -> Name(),
                       methods_found[0] -> method_symbol -> Header(),
                       method_symbol -> containing_type -> ContainingPackageName(),
                       method_symbol -> containing_type -> ExternalName(),
                       methods_found[i] -> method_symbol -> Header(),
                       methods_found[i] -> method_symbol -> containing_type -> ContainingPackageName(),
                       methods_found[i] -> method_symbol -> containing_type -> ExternalName());
    }

    // Java 5: where_found can be NULL for static imports
    if (where_found && method_symbol -> containing_type != where_found -> Type())
    {
        //
        // The method was inherited.
        //
        if (method_symbol -> ACC_SYNTHETIC() && ! method_symbol -> ACC_BRIDGE())
        {
            // Allow enum synthetic methods (values, valueOf) to be called
            bool is_enum_synthetic = method_symbol -> containing_type -> IsEnum() &&
                (wcscmp(method_symbol -> Name(), L"values") == 0 ||
                 wcscmp(method_symbol -> Name(), L"valueOf") == 0);

            if (!is_enum_synthetic)
            {
                ReportSemError(SemanticError::SYNTHETIC_METHOD_INVOCATION,
                               method_call, method_symbol -> Header(),
                               method_symbol -> containing_type -> ContainingPackageName(),
                               method_symbol -> containing_type -> ExternalName());
            }
        }
        else if (control.option.pedantic)
        {
            //
            // Give a pedantic warning if the inherited method shadowed
            // a method of the same name within an enclosing lexical scope.
            //
            Tuple<MethodShadowSymbol*> others(2);
            SemanticEnvironment* found_other;
            SemanticEnvironment* previous_env = where_found -> previous;
            FindMethodInEnvironment(others, found_other, previous_env,
                                    method_call);

            if (others.Length() > 0 &&
                where_found -> Type() != found_other -> Type())
            {
                for (unsigned i = 0; i < others.Length();  i++)
                {
                    if (others[i] -> method_symbol != method_symbol &&
                        (others[i] -> method_symbol -> containing_type ==
                         found_other -> Type()))
                    {
                        ReportSemError(SemanticError::INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_MEMBER,
                                       method_call,
                                       method_symbol -> Name(),
                                       method_symbol -> containing_type -> ContainingPackageName(),
                                       method_symbol -> containing_type -> ExternalName(),
                                       found_other -> Type() -> ContainingPackageName(),
                                       found_other -> Type() -> ExternalName());
                        break; // emit only one error message
                    }
                }
            }
        }
    }

    //
    // If this method came from a class file, make sure that its throws
    // clause has been processed.
    //
    method_symbol -> ProcessMethodThrows(this,
                                         method_call -> identifier_token);
    if (control.option.deprecation && method_symbol -> IsDeprecated() &&
        ! InDeprecatedContext())
    {
        ReportSemError(SemanticError::DEPRECATED_METHOD,
                       method_call, method_symbol -> Header(),
                       method_symbol -> containing_type -> ContainingPackageName(),
                       method_symbol -> containing_type -> ExternalName());
    }
    return methods_found[0];
}



//
// Search the type in question for a variable. Note that name_symbol is an
// optional argument. If it was not passed to this function then its default
// value is NULL (see semantic.h) and we assume that the name to search for
// is the last identifier specified in the field_access. Error reporting if
// the field is not found is up to the callee, since for qualified names,
// the name may successfully resolve to a nested type.
//

} // Close namespace Jopa block
