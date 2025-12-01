// Semantic expression processing - literals and primary expressions
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

void Semantic::ProcessCharacterLiteral(Ast* expr)
{
    AstCharacterLiteral* char_literal = (AstCharacterLiteral*) expr;

    LiteralSymbol* literal =
        lex_stream -> LiteralSymbol(char_literal -> character_literal_token);

    if (! literal -> value)
        control.int_pool.FindOrInsertChar(literal);
    if (literal -> value == control.BadValue())
        char_literal -> symbol = control.no_type;
    else
    {
        char_literal -> value = literal -> value;
        char_literal -> symbol = control.char_type;
    }
}


void Semantic::ProcessIntegerLiteral(Ast* expr)
{
    AstIntegerLiteral* int_literal = (AstIntegerLiteral*) expr;

    LiteralSymbol* literal =
        lex_stream -> LiteralSymbol(int_literal -> integer_literal_token);

    if (! literal -> value)
        control.int_pool.FindOrInsertInt(literal);
    if (literal -> value == control.BadValue())
    {
        ReportSemError(SemanticError::INVALID_INT_VALUE, int_literal);
        int_literal -> symbol = control.no_type;
    }
    else
    {
        int_literal -> value = literal -> value;
        int_literal -> symbol = control.int_type;
    }
}


void Semantic::ProcessLongLiteral(Ast* expr)
{
    AstLongLiteral* long_literal = (AstLongLiteral*) expr;

    LiteralSymbol* literal =
        lex_stream -> LiteralSymbol(long_literal -> long_literal_token);

    if (! literal -> value)
        control.long_pool.FindOrInsertLong(literal);
    if (literal -> value == control.BadValue())
    {
        ReportSemError(SemanticError::INVALID_LONG_VALUE, long_literal);
        long_literal -> symbol = control.no_type;
    }
    else
    {
        long_literal -> value = literal -> value;
        long_literal -> symbol = control.long_type;
    }
}


void Semantic::ProcessFloatLiteral(Ast* expr)
{
    AstFloatLiteral* float_literal = (AstFloatLiteral*) expr;

    LiteralSymbol* literal =
        lex_stream -> LiteralSymbol(float_literal -> float_literal_token);

    if (! literal -> value)
        control.float_pool.FindOrInsertFloat(literal);
    if (control.option.source < JopaOption::SDK1_5 &&
        (literal -> Name()[1] == U_x || literal -> Name()[1] == U_X))
    {
        ReportSemError(SemanticError::HEX_FLOATING_POINT_UNSUPPORTED,
                       float_literal);
    }
    if (literal -> value == control.BadValue())
    {
        ReportSemError(SemanticError::INVALID_FLOAT_VALUE, float_literal);
        float_literal -> symbol = control.no_type;
    }
    else
    {
        float_literal -> value = literal -> value;
        float_literal -> symbol = control.float_type;
    }
}


void Semantic::ProcessDoubleLiteral(Ast* expr)
{
    AstDoubleLiteral* double_literal = (AstDoubleLiteral*) expr;

    LiteralSymbol* literal =
        lex_stream -> LiteralSymbol(double_literal -> double_literal_token);

    if (! literal -> value)
        control.double_pool.FindOrInsertDouble(literal);
    if (control.option.source < JopaOption::SDK1_5 &&
        (literal -> Name()[1] == U_x || literal -> Name()[1] == U_X))
    {
        ReportSemError(SemanticError::HEX_FLOATING_POINT_UNSUPPORTED,
                       double_literal);
    }
    if (literal -> value == control.BadValue())
    {
        ReportSemError(SemanticError::INVALID_DOUBLE_VALUE, double_literal);
        double_literal -> symbol = control.no_type;
    }
    else
    {
        double_literal -> value = literal -> value;
        double_literal -> symbol = control.double_type;
    }
}


void Semantic::ProcessTrueLiteral(Ast* expr)
{
    AstExpression* true_literal = (AstTrueLiteral*) expr;

    true_literal -> value = control.int_pool.FindOrInsert((int) 1);
    true_literal -> symbol = control.boolean_type;
}


void Semantic::ProcessFalseLiteral(Ast* expr)
{
    AstExpression* false_literal = (AstFalseLiteral*) expr;

    false_literal -> value = control.int_pool.FindOrInsert((int) 0);
    false_literal -> symbol = control.boolean_type;
}


void Semantic::ProcessStringLiteral(Ast* expr)
{
    AstStringLiteral* string_literal = (AstStringLiteral*) expr;

    LiteralSymbol* literal =
        lex_stream -> LiteralSymbol(string_literal -> string_literal_token);

    if (! literal -> value)
        control.Utf8_pool.FindOrInsertString(literal);
    if (literal -> value == control.BadValue())
        string_literal -> symbol = control.no_type;
    else
    {
        string_literal -> value = literal -> value;
        string_literal -> symbol = control.String();
    }
}


void Semantic::ProcessArrayAccess(Ast* expr)
{
    AstArrayAccess* array_access = (AstArrayAccess*) expr;

    ProcessExpression(array_access -> base);
    ProcessExpression(array_access -> expression);
    array_access -> expression =
        PromoteUnaryNumericExpression(array_access -> expression);
    if (array_access -> expression -> Type() != control.int_type)
    {
        TypeSymbol* type = array_access -> expression -> Type();
        if (array_access -> expression -> symbol != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_INTEGER,
                           array_access -> expression,
                           type -> ContainingPackageName(),
                           type -> ExternalName());
        array_access -> symbol = control.no_type;
    }

    TypeSymbol* array_type = array_access -> base -> Type();
    if (array_type -> IsArray())
    {
        if (! array_access -> symbol)
            array_access -> symbol = array_type -> ArraySubtype();
    }
    else
    {
        if (array_type != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_ARRAY,
                           array_access -> base,
                           array_type -> ContainingPackageName(),
                           array_type -> ExternalName());
        array_access -> symbol = control.no_type;
    }
}


MethodShadowSymbol* Semantic::FindMethodMember(TypeSymbol* type,
                                               AstMethodInvocation* method_call)
{
    AstExpression* base = method_call -> base_opt;
    TokenIndex id_token = method_call -> identifier_token;
    assert(base);
    //
    // TypeCast() returns true for super, this, and instance creation as
    // well as true type names, hence the extra check
    //
    bool base_is_type = base -> symbol -> TypeCast() && base -> NameCast();
    MethodShadowSymbol* shadow = NULL;

    if (type -> Bad())
    {
        //
        // If no error has been detected so far, report this as an error so
        // that we don't try to generate code later. On the other hand, if an
        // error had been detected prior to this, don't flood the user with
        // spurious messages.
        //
        if (NumErrors() == 0)
            ReportMethodNotFound(method_call, type);
        method_call -> symbol = control.no_type;
    }
    else if (type == control.null_type || type -> Primitive())
    {
        ReportSemError(SemanticError::TYPE_NOT_REFERENCE, base,
                       type -> Name());
        method_call -> symbol = control.no_type;
    }
    else
    {
        TypeSymbol* this_type = ThisType();
        if (! TypeAccessCheck(type))
        {
            ReportTypeInaccessible(base, type);
            method_call -> symbol = control.no_type;
            return shadow;
        }

        shadow = FindMethodInType(type, method_call);
        MethodSymbol* method = (shadow ? shadow -> method_symbol
                                : (MethodSymbol*) NULL);
        if (method)
        {
            assert(method -> IsTyped());

            if (base_is_type && ! method -> ACC_STATIC())
            {
                ReportSemError(SemanticError::METHOD_NOT_CLASS_METHOD,
                               method_call -> LeftToken(), id_token,
                               lex_stream -> NameString(id_token));
                method_call -> symbol = control.no_type;
                return NULL;
            }
            if (method -> ACC_STATIC() && ! base_is_type)
            {
                ReportSemError(SemanticError::CLASS_METHOD_INVOKED_VIA_INSTANCE,
                               method_call -> LeftToken(), id_token,
                               lex_stream -> NameString(id_token));
            }

            //
            // Apply method invocation conversion to the parameters
            //
            MethodInvocationConversion(method_call -> arguments, method);

            //
            // Access to a private or protected variable in or via an enclosing
            // type? If the base is a super expression, be sure to start from
            // the correct enclosing instance.
            //
            TypeSymbol* containing_type = method -> containing_type;
            TypeSymbol* target_type = containing_type;
            if (! method -> ACC_STATIC() && base -> SuperExpressionCast())
            {
                AstSuperExpression* super_expr = (AstSuperExpression*) base;
                if (super_expr -> base_opt)
                    target_type = super_expr -> base_opt -> symbol;
            }
            if (this_type != target_type &&
                (method -> ACC_PRIVATE() ||
                 (method -> ACC_PROTECTED() &&
                  ! ProtectedAccessCheck(containing_type)) ||
                 (target_type != containing_type &&
                  target_type != this_type)))
            {
                //
                // Find the right enclosing class to place the accessor method
                // in. For private methods, the containing type; for protected
                // methods or superclass methods, an enclosing class which is
                // related to the containing type.
                //
                TypeSymbol* environment_type = containing_type;
                if (! method -> ACC_PRIVATE())
                {
                    for (SemanticEnvironment* env = this_type -> semantic_environment;
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

                AstArguments* args = compilation_unit -> ast_pool ->
                    GenArguments(method_call -> arguments -> left_parenthesis_token,
                                 method_call -> arguments -> right_parenthesis_token);
                unsigned num_args = method_call -> arguments -> NumArguments();
                if (! method -> ACC_STATIC())
                {
                    args -> AllocateArguments(num_args + 1);
                    args -> AddArgument(base);
                }
                else args -> AllocateArguments(num_args);
                for (unsigned i = 0; i < num_args; i++)
                    args -> AddArgument(method_call -> arguments -> Argument(i));

                AstMethodInvocation* accessor = compilation_unit ->
                    ast_pool -> GenMethodInvocation(id_token);
                // TODO: WARNING: sharing of subtrees...
                accessor -> base_opt = base;
                accessor -> arguments = args;
                accessor -> symbol = environment_type ->
                    GetReadAccessMethod(method, base -> Type());

                method_call -> symbol = method;
                method_call -> resolution_opt = accessor;
            }
            else method_call -> symbol = method;
        }
        else
        {
            method_call -> symbol = control.no_type;
        }
    }
    return shadow;
}


static Type* Substitute(Type* type, ParameterizedType* context)
{
    if (type -> IsTypeParameter())
    {
        TypeParameterSymbol* param = type -> GetTypeParameter();
        if (param -> IsClassTypeParameter() && param -> ContainingType() == context -> generic_type)
        {
            unsigned pos = param -> position;
            if (context -> type_arguments && pos < context -> type_arguments -> Length())
            {
                return (*context -> type_arguments)[pos] -> Clone();
            }
        }
        return type -> Clone();
    }
    
    if (type -> IsParameterized())
    {
        ParameterizedType* pt = type -> GetParameterizedType();
        Tuple<Type*>* new_args = new Tuple<Type*>(pt -> type_arguments -> Length());
        for (unsigned i = 0; i < pt -> type_arguments -> Length(); i++)
        {
            new_args -> Next() = Substitute((*pt -> type_arguments)[i], context);
        }
        return new Type(new ParameterizedType(pt -> generic_type, new_args, pt -> enclosing_type), true);
    }
    
    if (type -> IsArray())
    {
        ArrayType* at = type -> GetArrayType();
        Type* new_comp = Substitute(at -> component_type, context);
        return new Type(new ArrayType(new_comp), true);
    }
    
    return type -> Clone();
}

static void ResolveGenericReturnType(AstMethodInvocation* call, MethodSymbol* method, AstExpression* base)
{
    ParameterizedType* base_pt = NULL;
    if (base && base -> resolved_parameterized_type)
        base_pt = base -> resolved_parameterized_type;

    if (method -> return_parameterized_type)
    {
        Type* ret_type = new Type(method -> return_parameterized_type, false); 
        Type* result = NULL;
        
        if (base_pt) {
            result = Substitute(ret_type, base_pt);
        } else {
            result = ret_type -> Clone();
        }
        
        if (result && result -> IsParameterized())
        {
            call -> resolved_parameterized_type = result -> GetParameterizedType();
            result -> owns_content = false;
        }
        delete result;
        delete ret_type;
    }
    else if (method -> return_type_param_index >= 0)
    {
        if (base_pt)
        {
             unsigned pos = (unsigned)method -> return_type_param_index;
             if (pos < base_pt -> type_arguments -> Length())
             {
                 Type* type_arg = (*base_pt -> type_arguments)[pos];
                 
                 if (type_arg -> IsParameterized())
                 {
                     // Clone it to avoid lifetime issues? 
                     // For now assuming type graphs are stable during compilation phase.
                     call -> resolved_parameterized_type = type_arg -> GetParameterizedType();
                 }
                 else if (type_arg -> kind == Type::SIMPLE_TYPE)
                 {
                     call -> resolved_type = type_arg -> GetTypeSymbol();
                 }
             }
        }
    }
}

void Semantic::ProcessMethodName(AstMethodInvocation* method_call)
{
    TypeSymbol* this_type = ThisType();
    AstExpression* base = method_call -> base_opt;
    TokenIndex id_token = method_call -> identifier_token;
    TypeSymbol* base_type;
    MethodShadowSymbol* method_shadow;
    if (! base)
    {
        SemanticEnvironment* where_found;
        method_shadow = FindMethodInEnvironment(where_found, method_call);
        if (! method_shadow)
        {
            method_call -> symbol = control.no_type;
            base_type = NULL;
        }
        else
        {
            // Java 5: where_found can be NULL for static imports
            base_type = where_found ? where_found -> Type() : method_shadow -> method_symbol -> containing_type;
            MethodSymbol* method = method_shadow -> method_symbol;
            assert(method -> IsTyped());

            if (! method -> ACC_STATIC())
            {
                if (ExplicitConstructorInvocation())
                {
                    if (where_found == state_stack.Top())
                    {
                        //
                        // If the method belongs to this type, including
                        // inherited from an enclosing type, it is not
                        // accessible.
                        //
                        ReportSemError(SemanticError::INSTANCE_METHOD_IN_EXPLICIT_CONSTRUCTOR,
                                       method_call, method -> Header(),
                                       method -> containing_type -> Name());
                        method_call -> symbol = control.no_type;
                        method_shadow = NULL;
                    }
                }
                else if (StaticRegion())
                {
                    ReportSemError(SemanticError::METHOD_NOT_CLASS_METHOD,
                                   method_call,
                                   lex_stream -> NameString(id_token));
                    method_call -> symbol = control.no_type;
                    method_shadow = NULL;
                }
            }

            //
            // Apply method invocation conversion to the parameters
            //
            MethodInvocationConversion(method_call -> arguments, method);
            method_call -> symbol = method;
            ResolveGenericReturnType(method_call, method, NULL);

            //
            // If the method is a private method belonging to an outer type,
            // give the ast simple_name access to its read_method.
            // Java 5: where_found is NULL for static imports, no accessor needed
            //
            if (where_found && where_found != state_stack.Top())
                CreateAccessToScopedMethod(method_call, where_found -> Type());
        }
    }
    else
    {
        //
        // ...First, classify the name or expression to the left of the '.'...
        // If there are more names to the left, we short-circuit
        // ProcessFieldAccess, since we already know what context the name
        // is in.
        //
        if (base -> NameCast())
            ProcessAmbiguousName((AstName*) base);
        else // The qualifier might be a complex String constant
            ProcessExpressionOrStringConstant(base);

        if (base -> symbol -> PackageCast())
        {
            ReportSemError(SemanticError::UNKNOWN_AMBIGUOUS_NAME, base,
                           base -> symbol -> PackageCast() -> PackageName());
            base -> symbol = control.no_type;
        }

        base_type = base -> Type();
        assert(base_type);

        if (base_type == control.no_type)
        {
            method_call -> symbol = control.no_type;
            method_shadow = NULL;
        }
        else
            method_shadow = FindMethodMember(base_type, method_call);
        if (base -> SuperExpressionCast())
        {
            //
            // JLS2 15.12.3 requires this test
            //
            MethodSymbol* method = method_call -> symbol -> MethodCast();
            if (method && method -> ACC_ABSTRACT())
            {
                ReportSemError(SemanticError::ABSTRACT_METHOD_INVOCATION,
                               method_call,
                               lex_stream -> NameString(id_token));
            }
        }
        else AddDependence(this_type, base_type);
    }

    //
    // If we found a candidate, proceed to check the throws clauses. If
    // base_type inherited multiple abstract methods, then this calling
    // environment must merge the throws clauses (although it may invoke an
    // arbitrary method from the set). Be careful of default and protected
    // abstract methods which are not accessible when doing this merge.
    //
    if (method_shadow)
    {
        MethodSymbol* method = (MethodSymbol*) method_call -> symbol;
        if (! MemberAccessCheck(base_type, method, base))
        {
            assert(method_shadow -> NumConflicts() > 0);
            method = method_shadow -> Conflict(0);
            method_call -> symbol = method;
        }
        ResolveGenericReturnType(method_call, method, base);

        SymbolSet exceptions(method -> NumThrows());
        int i, j;
        // First, the base set
        for (i = method -> NumThrows(); --i >= 0; )
            exceptions.AddElement(method -> Throws(i));
        // Next, add all subclasses thrown in method conflicts
        for (i = method_shadow -> NumConflicts(); --i >= 0; )
        {
            MethodSymbol* conflict = method_shadow -> Conflict(i);
            conflict -> ProcessMethodThrows(this,
                                            method_call -> identifier_token);
            for (j = conflict -> NumThrows(); --j >= 0; )
            {
                TypeSymbol* candidate = conflict -> Throws(j);
                for (TypeSymbol* ex = (TypeSymbol*) exceptions.FirstElement();
                     ex; ex = (TypeSymbol*) exceptions.NextElement())
                {
                    if (candidate -> IsSubclass(ex))
                    {
                        exceptions.AddElement(candidate);
                        break;
                    }
                }
            }
        }
        // Finally, prune all methods not thrown by all conflicts, and report
        // uncaught exceptions.
        TypeSymbol* ex = (TypeSymbol*) exceptions.FirstElement();
        while (ex)
        {
            bool remove = false;
            for (i = method_shadow -> NumConflicts(); --i >= 0; )
            {
                MethodSymbol* conflict = method_shadow -> Conflict(i);
                for (j = conflict -> NumThrows(); --j >= 0; )
                {
                    TypeSymbol* candidate = conflict -> Throws(j);
                    if (ex -> IsSubclass(candidate))
                        break;
                }
                if (j < 0)
                {
                    remove = true;
                    break;
                }
            }
            TypeSymbol* temp = (TypeSymbol*) exceptions.NextElement();
            if (remove)
                exceptions.RemoveElement(ex);
            else if (UncaughtException(ex))
                ReportSemError(SemanticError::UNCAUGHT_METHOD_EXCEPTION,
                               method_call, method -> Header(),
                               ex -> ContainingPackageName(),
                               ex -> ExternalName(),
                               UncaughtExceptionContext());
            ex = temp;
        }

        SymbolSet* exception_set = TryExceptionTableStack().Top();
        if (exception_set)
            exception_set -> Union(exceptions);
    }
    else
    {
        //
        // There was no candidate, so we have no idea what can be thrown in
        // a try block if it had been a valid method call.
        //
        SymbolSet* exception_set = TryExceptionTableStack().Top();
        if (exception_set)
            exception_set -> AddElement(control.no_type);
    }

    //
    // Type substitution for generics: If the method's return type is a type parameter
    // of its containing class, substitute it with the corresponding type argument
    // from the receiver's parameterized type.
    //
    MethodSymbol* method = method_call -> symbol -> MethodCast();
    if (method && method -> return_type_param_index >= 0)
    {
        unsigned param_index = (unsigned) method -> return_type_param_index;
        ParameterizedType* param_type = NULL;

        // Case 1: Check if the receiver variable has a parameterized type
        // Also check if the base expression has resolved_parameterized_type (for chained calls)
        if (base)
        {
            VariableSymbol* var = base -> symbol -> VariableCast();
            ParameterizedType* base_param_type = NULL;

            if (var && var -> parameterized_type)
            {
                base_param_type = var -> parameterized_type;
            }
            else if (base -> ExpressionCast() && base -> ExpressionCast() -> resolved_parameterized_type)
            {
                // For chained method calls like map.get(x).get(y)
                base_param_type = base -> ExpressionCast() -> resolved_parameterized_type;
            }

            if (base_param_type)
            {
                if (base_param_type -> generic_type == method -> containing_type)
                {
                    param_type = base_param_type;
                }
                else
                {
                    // The method is inherited from a superclass. Walk up the inheritance
                    // hierarchy and propagate type arguments.
                    // E.g., GenericExtender<Integer> extends Container<E>
                    // where get() is in Container<T>. Need to map Integer -> E -> T.
                    TypeSymbol* current_generic = base_param_type -> generic_type;
                    ParameterizedType* current_param = base_param_type;

                    while (current_generic && ! param_type)
                    {
                        if (current_generic -> HasParameterizedSuper())
                        {
                            ParameterizedType* super_param = current_generic -> GetParameterizedSuper();
                            if (super_param -> generic_type == method -> containing_type)
                            {
                                // Found the target. Now substitute type arguments.
                                // super_param's type arguments may reference current_generic's
                                // type parameters, which we need to substitute with current_param's
                                // type arguments.
                                // For now, handle the simple case where the super's type argument
                                // is directly a type parameter of current_generic.
                                if (super_param -> NumTypeArguments() > param_index)
                                {
                                    Type* super_type_arg = super_param -> TypeArgument(param_index);
                                    if (super_type_arg)
                                    {
                                        // Check if this type arg is a type parameter of current_generic
                                        TypeParameterSymbol* type_param_sym =
                                            super_type_arg -> IsTypeParameter()
                                                ? super_type_arg -> GetTypeParameter() : NULL;
                                        if (type_param_sym && current_param)
                                        {
                                            // Find which type parameter index this is
                                            for (unsigned k = 0; k < current_generic -> NumTypeParameters(); k++)
                                            {
                                                if (current_generic -> TypeParameter(k) == type_param_sym)
                                                {
                                                    // Substitute with current_param's type argument
                                                    if (k < current_param -> NumTypeArguments())
                                                    {
                                                        Type* substituted_arg =
                                                            current_param -> TypeArgument(k);
                                                        if (substituted_arg)
                                                        {
                                                            TypeSymbol* result = substituted_arg -> Erasure();
                                                            if (result && result -> fully_qualified_name &&
                                                                ! result -> Primitive())
                                                            {
                                                                method_call -> resolved_type = result;
                                                                // Also track parameterized type for chained calls
                                                                if (substituted_arg -> IsParameterized())
                                                                    method_call -> resolved_parameterized_type =
                                                                        substituted_arg -> GetParameterizedType();
                                                                param_type = super_param; // Mark as found
                                                            }
                                                        }
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            // Direct type argument (not a type parameter reference)
                                            param_type = super_param;
                                        }
                                    }
                                }
                                break;
                            }
                            current_param = super_param;
                            current_generic = super_param -> generic_type;
                        }
                        // Also check parameterized interfaces (for interface inheritance)
                        if (! param_type)
                        {
                            for (unsigned iface_idx = 0;
                                 iface_idx < current_generic -> NumInterfaces() && ! param_type;
                                 iface_idx++)
                            {
                                ParameterizedType* iface_param =
                                    current_generic -> ParameterizedInterface(iface_idx);
                                if (iface_param && iface_param -> generic_type == method -> containing_type)
                                {
                                    // Found the target interface. Substitute type arguments.
                                    if (iface_param -> NumTypeArguments() > param_index)
                                    {
                                        Type* iface_type_arg = iface_param -> TypeArgument(param_index);
                                        if (iface_type_arg)
                                        {
                                            TypeParameterSymbol* type_param_sym =
                                                iface_type_arg -> IsTypeParameter()
                                                    ? iface_type_arg -> GetTypeParameter() : NULL;
                                            if (type_param_sym && current_param)
                                            {
                                                for (unsigned k = 0; k < current_generic -> NumTypeParameters(); k++)
                                                {
                                                    if (current_generic -> TypeParameter(k) == type_param_sym)
                                                    {
                                                        if (k < current_param -> NumTypeArguments())
                                                        {
                                                            Type* substituted_arg =
                                                                current_param -> TypeArgument(k);
                                                            if (substituted_arg)
                                                            {
                                                                TypeSymbol* result = substituted_arg -> Erasure();
                                                                if (result && result -> fully_qualified_name &&
                                                                    ! result -> Primitive())
                                                                {
                                                                    method_call -> resolved_type = result;
                                                                    // Also track parameterized type for chained calls
                                                                    if (substituted_arg -> IsParameterized())
                                                                        method_call -> resolved_parameterized_type =
                                                                            substituted_arg -> GetParameterizedType();
                                                                    param_type = iface_param;
                                                                }
                                                            }
                                                        }
                                                        break;
                                                    }
                                                }
                                            }
                                            else if (! type_param_sym)
                                            {
                                                // Direct type argument (not a type parameter reference)
                                                param_type = iface_param;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (! param_type)
                        {
                            if (current_generic -> HasParameterizedSuper())
                            {
                                ParameterizedType* super_param = current_generic -> GetParameterizedSuper();
                                current_param = super_param;
                                current_generic = super_param -> generic_type;
                            }
                            else
                            {
                                current_generic = current_generic -> super;
                                current_param = NULL;
                            }
                        }
                    }
                }
            }
        }

        // Case 2: For super.method() calls, check if THIS type has a parameterized super
        // that matches the method's containing type
        if (! param_type && base && base -> SuperExpressionCast())
        {
            TypeSymbol* current_type = ThisType();
            if (current_type && current_type -> HasParameterizedSuper())
            {
                ParameterizedType* param_super = current_type -> GetParameterizedSuper();
                if (param_super -> generic_type == method -> containing_type)
                {
                    param_type = param_super;
                }
                else
                {
                    // The method might be from an ancestor of the direct super
                    // Walk up through parameterized supers
                    TypeSymbol* super_type = param_super -> generic_type;
                    while (super_type && ! param_type)
                    {
                        if (super_type -> HasParameterizedSuper())
                        {
                            ParameterizedType* ancestor_param = super_type -> GetParameterizedSuper();
                            if (ancestor_param -> generic_type == method -> containing_type)
                                param_type = ancestor_param;
                            super_type = ancestor_param -> generic_type;
                        }
                        else
                        {
                            super_type = super_type -> super;
                        }
                    }
                }
            }
        }

        // Case 3: Walk up the inheritance hierarchy from base_type
        // Track type argument substitutions through the chain
        if (! param_type && base_type)
        {
            // Start with the first parameterized super in the chain
            ParameterizedType* current_param_super = base_type -> HasParameterizedSuper()
                ? base_type -> GetParameterizedSuper() : NULL;
            TypeSymbol* current = base_type;

            while (current && ! param_type)
            {
                if (current -> HasParameterizedSuper())
                {
                    ParameterizedType* param_super = current -> GetParameterizedSuper();
                    if (param_super -> generic_type == method -> containing_type)
                    {
                        // Found the method's containing type. Check if we need to substitute.
                        if (param_super -> NumTypeArguments() > param_index)
                        {
                            Type* type_arg = param_super -> TypeArgument(param_index);
                            if (type_arg && type_arg -> IsTypeParameter())
                            {
                                // The type argument is a type parameter - need to substitute
                                // using the previous level's type arguments
                                TypeParameterSymbol* tps = type_arg -> GetTypeParameter();
                                if (current_param_super && tps)
                                {
                                    // Find this type parameter's position in current's type params
                                    for (unsigned k = 0; k < current -> NumTypeParameters(); k++)
                                    {
                                        if (current -> TypeParameter(k) == tps &&
                                            k < current_param_super -> NumTypeArguments())
                                        {
                                            Type* substituted_type = current_param_super -> TypeArgument(k);
                                            if (substituted_type)
                                            {
                                                TypeSymbol* result = substituted_type -> Erasure();
                                                if (result && result -> fully_qualified_name &&
                                                    ! result -> Primitive())
                                                {
                                                    method_call -> resolved_type = result;
                                                    // Also track parameterized type for chained calls
                                                    if (substituted_type -> IsParameterized())
                                                        method_call -> resolved_parameterized_type =
                                                            substituted_type -> GetParameterizedType();
                                                    // Don't set param_type here - we've already done the
                                                    // full substitution and don't want the code below
                                                    // to overwrite with the wrong type argument
                                                }
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // Concrete type argument - use it directly
                                param_type = param_super;
                            }
                        }
                        break;
                    }
                    // Update current_param_super for the next iteration
                    current_param_super = param_super;
                    current = param_super -> generic_type;
                }
                else
                {
                    current_param_super = NULL;
                    current = current -> super;
                }
            }
        }

        // Substitute if we found the parameterized type and have the type argument.
        // Skip this if resolved_type was already set by the cases above (which handle
        // complex type parameter chains through inheritance hierarchies).
        if (! method_call -> resolved_type && param_type && param_index < param_type -> NumTypeArguments())
        {
            Type* type_arg = param_type -> TypeArgument(param_index);
            if (type_arg)
            {
                TypeSymbol* substituted = type_arg -> Erasure();
                // Make sure we have a valid reference type (not primitive)
                // Primitives don't have fully_qualified_name and can't be type arguments
                if (substituted &&
                    substituted -> fully_qualified_name &&
                    ! substituted -> Primitive())
                {
                    // Handle array return types: T[] getEnumConstants()
                    // The method's erased return type includes the array dimensions
                    TypeSymbol* method_return_type = method -> Type();
                    unsigned return_dims = method_return_type ? method_return_type -> num_dimensions : 0;
                    if (return_dims > 0)
                    {
                        // Add array dimensions to the substituted type
                        substituted = substituted -> GetArrayType((Semantic*) this, return_dims);
                    }
                    method_call -> resolved_type = substituted;
                    // Also track parameterized type for chained calls
                    if (type_arg -> IsParameterized())
                        method_call -> resolved_parameterized_type =
                            type_arg -> GetParameterizedType();
                }
                else if (! substituted)
                {
                    // Unbounded type parameter erases to Object
                    TypeSymbol* result = control.Object();
                    // Handle array return types even for unbounded type parameters
                    TypeSymbol* method_return_type = method -> Type();
                    unsigned return_dims = method_return_type ? method_return_type -> num_dimensions : 0;
                    if (return_dims > 0)
                        result = result -> GetArrayType((Semantic*) this, return_dims);
                    method_call -> resolved_type = result;
                }
                // If substituted is primitive or lacks fully_qualified_name, keep original resolved_type
            }
        }
    }

    //
    // Type inference for generic methods: If the method's return type is one of its own
    // type parameters, infer the type from the actual arguments.
    //
    if (method && method -> method_return_type_param_index >= 0)
    {
        int return_type_param = method -> method_return_type_param_index;
        TypeSymbol* inferred_type = NULL;

        // Try to infer the type from actual arguments
        if (method -> param_type_param_indices)
        {
            unsigned num_args = method_call -> arguments -> NumArguments();
            unsigned num_params = method -> param_type_param_indices -> Length();
            unsigned limit = (num_args < num_params) ? num_args : num_params;

            for (unsigned i = 0; i < limit && ! inferred_type; i++)
            {
                int param_type_param = (*(method -> param_type_param_indices))[i];
                if (param_type_param == return_type_param)
                {
                    // This argument corresponds to the same type parameter as the return type
                    AstExpression* arg = method_call -> arguments -> Argument(i);

                    // Unwrap cast expressions to get the original type
                    while (arg -> kind == Ast::CAST)
                    {
                        AstCastExpression* cast = (AstCastExpression*) arg;
                        arg = cast -> expression;
                    }

                    TypeSymbol* arg_type = NULL;

                    // Handle class literals specially: for String.class, extract String
                    // This handles patterns like getInstance(Class<T> clazz)
                    if (arg -> kind == Ast::CLASS_LITERAL)
                    {
                        AstClassLiteral* class_literal = (AstClassLiteral*) arg;
                        if (class_literal -> type && class_literal -> type -> symbol)
                            arg_type = class_literal -> type -> symbol;
                    }
                    // Handle class creation expressions
                    // For doPrivileged(new PrivilegedAction<Provider>() {...}), extract Provider
                    // For doPrivileged(new StringAction(...)) where StringAction implements
                    // PrivilegedAction<String>, extract String from the implemented interface
                    else if (arg -> kind == Ast::CLASS_CREATION)
                    {
                        AstClassCreationExpression* class_creation =
                            (AstClassCreationExpression*) arg;
                        // Check if the class type has type arguments (parameterized type)
                        if (class_creation -> class_type &&
                            class_creation -> class_type -> parameterized_type &&
                            class_creation -> class_type -> parameterized_type -> NumTypeArguments() > 0)
                        {
                            // Extract the first type argument
                            // For PrivilegedAction<Provider>, get Provider
                            Type* type_arg = class_creation -> class_type ->
                                parameterized_type -> TypeArgument(0);
                            if (type_arg)
                                arg_type = type_arg -> Erasure();
                        }
                        // No explicit type arguments - check if the created class implements
                        // the parameter interface with type arguments
                        else if (class_creation -> class_type &&
                                 class_creation -> class_type -> symbol &&
                                 i < method -> NumFormalParameters())
                        {
                            TypeSymbol* created_type = class_creation -> class_type -> symbol;
                            VariableSymbol* formal_param = method -> FormalParameter(i);
                            TypeSymbol* param_erasure = formal_param -> Type();
                            if (created_type && param_erasure && param_erasure -> ACC_INTERFACE())
                            {
                                // Search for the interface in the created type's hierarchy
                                TypeSymbol* search_type = created_type;
                                while (search_type && ! arg_type)
                                {
                                    for (unsigned k = 0; k < search_type -> NumInterfaces(); k++)
                                    {
                                        if (search_type -> Interface(k) == param_erasure)
                                        {
                                            ParameterizedType* param_interf =
                                                search_type -> ParameterizedInterface(k);
                                            if (param_interf &&
                                                param_interf -> NumTypeArguments() > 0)
                                            {
                                                Type* type_arg = param_interf -> TypeArgument(0);
                                                if (type_arg)
                                                    arg_type = type_arg -> Erasure();
                                            }
                                            break;
                                        }
                                    }
                                    search_type = search_type -> super;
                                }
                            }
                        }
                        // For raw types (no type arguments and no parameterized interface),
                        // don't infer - let return type be erased type
                    }
                    else
                    {
                        arg_type = arg -> Type();
                        // For array parameters like T[], extract the element type from the argument
                        // For example, if argument is String[], we want String (not String[])
                        // so that the return type T[] becomes String[] (not String[][])
                        if (arg_type && arg_type -> num_dimensions > 0)
                        {
                            // Get the formal parameter to check if it's an array type
                            VariableSymbol* formal_param = (i < method -> NumFormalParameters())
                                ? method -> FormalParameter(i) : NULL;
                            // The parameter type (after erasure) is Object[] for T[]
                            // If the parameter is also an array, extract element type from argument
                            if (formal_param && formal_param -> Type() &&
                                formal_param -> Type() -> num_dimensions > 0)
                            {
                                // Strip only the formal parameter's array dimensions from argument
                                // For toArray(T[] a) called with String[][], T = String[]
                                unsigned formal_dims = formal_param -> Type() -> num_dimensions;
                                unsigned arg_dims = arg_type -> num_dimensions;
                                if (arg_dims > formal_dims)
                                {
                                    // Argument has more dimensions - get array type with remaining dims
                                    unsigned remaining_dims = arg_dims - formal_dims;
                                    arg_type = arg_type -> base_type -> GetArrayType(
                                        (Semantic*) this, remaining_dims);
                                }
                                else
                                {
                                    // Same or fewer dimensions - use base type
                                    arg_type = arg_type -> base_type;
                                }
                            }
                        }
                        // For parameterized interface inference:
                        // If arg_type implements a parameterized interface that matches the
                        // formal parameter type, extract the type argument from that interface.
                        // E.g., for doPrivileged(action) where action is StringAction
                        // implementing PrivilegedAction<String>, infer T=String.
                        if (arg_type && i < method -> NumFormalParameters())
                        {
                            VariableSymbol* formal_param = method -> FormalParameter(i);
                            TypeSymbol* param_erasure = formal_param -> Type();
                            if (param_erasure && param_erasure -> ACC_INTERFACE())
                            {
                                // Check if arg_type implements this interface with type args
                                TypeSymbol* search_type = arg_type;
                                TypeSymbol* inferred_from_interface = NULL;
                                while (search_type && ! inferred_from_interface)
                                {
                                    for (unsigned k = 0; k < search_type -> NumInterfaces(); k++)
                                    {
                                        if (search_type -> Interface(k) == param_erasure)
                                        {
                                            // Found the interface - check for type arguments
                                            ParameterizedType* param_interf =
                                                search_type -> ParameterizedInterface(k);
                                            if (param_interf &&
                                                param_interf -> NumTypeArguments() > 0)
                                            {
                                                Type* type_arg = param_interf -> TypeArgument(0);
                                                if (type_arg)
                                                    inferred_from_interface = type_arg -> Erasure();
                                            }
                                            break;
                                        }
                                    }
                                    // Also check superclass hierarchy
                                    search_type = search_type -> super;
                                }
                                if (inferred_from_interface)
                                    arg_type = inferred_from_interface;
                            }
                        }
                    }

                    // Only use arg_type for inference if it's meaningful.
                    // For wrapped type params like Class<T>, if arg is raw Class,
                    // arg_type equals the formal param erasure and gives no info.
                    if (arg_type && arg_type != control.no_type && arg_type != control.null_type)
                    {
                        VariableSymbol* formal_param = (i < method -> NumFormalParameters())
                            ? method -> FormalParameter(i) : NULL;
                        // Skip if arg_type is just the erasure of the formal param
                        // (e.g., raw Class for Class<T>), unless it's a class literal
                        if (formal_param && arg_type == formal_param -> Type() &&
                            arg -> kind != Ast::CLASS_LITERAL)
                        {
                            // Raw type matches param erasure - no useful inference
                            // Skip this arg for type inference
                        }
                        else
                        {
                            inferred_type = arg_type;
                        }
                    }
                }
            }
        }

        // Substitute the return type with the inferred type
        if (inferred_type)
        {
            // Handle array return types: <T> T[] toArray(T elem)
            // Get dimensions from the method's declared return type (after type erasure)
            TypeSymbol* method_return_type = method -> Type();
            unsigned return_dims = method_return_type ? method_return_type -> num_dimensions : 0;
            if (return_dims > 0)
            {
                // Return type is an array - ADD dimensions to the inferred type
                // For <T> T[] toArray(T[] a), if T = String[], return T[] = String[][]
                unsigned inferred_dims = inferred_type -> num_dimensions;
                unsigned total_dims = inferred_dims + return_dims;
                TypeSymbol* base = inferred_type -> num_dimensions > 0
                    ? inferred_type -> base_type : inferred_type;
                method_call -> resolved_type = base -> GetArrayType((Semantic*) this, total_dims);
            }
            else
            {
                method_call -> resolved_type = inferred_type;
            }
        }
        //
        // Secondary inference: if the return type parameter is only constrained
        // through another method type parameter (e.g., <I extends Interface<T>, T>),
        // try to pull the concrete argument from the parameterized interface/class
        // implemented by the actual argument.
        //
        if (! inferred_type)
        {
            for (unsigned i = 0; i < method_call -> arguments -> NumArguments() && ! inferred_type; i++)
            {
                AstExpression* arg = method_call -> arguments -> Argument(i);
                TypeSymbol* arg_type = arg -> Type();
                VariableSymbol* formal_param =
                    (i < method -> NumFormalParameters()) ? method -> FormalParameter(i) : NULL;
                TypeSymbol* formal_type = formal_param ? formal_param -> Type() : NULL;
                if (! arg_type || ! formal_type)
                    continue;

                // Walk the argument's supertype chain to find a matching interface/class
                for (TypeSymbol* search = arg_type; search && ! inferred_type; search = search -> super)
                {
                    // Check interfaces first
                    for (unsigned k = 0; k < search -> NumInterfaces() && ! inferred_type; k++)
                    {
                        if (search -> Interface(k) != formal_type)
                            continue;

                        ParameterizedType* pinterface = search -> ParameterizedInterface(k);
                        if (pinterface && pinterface -> NumTypeArguments() > 0)
                        {
                            Type* type_arg = pinterface -> TypeArgument(0);
                            TypeSymbol* erasure = type_arg ? type_arg -> Erasure() : NULL;
                            if (erasure && erasure != control.no_type && erasure -> fully_qualified_name)
                                inferred_type = erasure;
                        }
                    }

                    // Also handle parameterized superclass that matches the formal type
                    if (! inferred_type && search -> super == formal_type)
                    {
                        ParameterizedType* psuper = search -> GetParameterizedSuper();
                        if (psuper && psuper -> NumTypeArguments() > 0)
                        {
                            Type* type_arg = psuper -> TypeArgument(0);
                            TypeSymbol* erasure = type_arg ? type_arg -> Erasure() : NULL;
                            if (erasure && erasure != control.no_type && erasure -> fully_qualified_name)
                                inferred_type = erasure;
                        }
                    }
                }
            }

            if (inferred_type)
            {
                TypeSymbol* method_return_type = method -> Type();
                unsigned return_dims = method_return_type ? method_return_type -> num_dimensions : 0;
                if (return_dims > 0)
                {
                    TypeSymbol* base = inferred_type -> num_dimensions > 0
                        ? inferred_type -> base_type : inferred_type;
                    method_call -> resolved_type = base -> GetArrayType((Semantic*) this,
                                                                         inferred_type -> num_dimensions + return_dims);
                }
                else
                {
                    method_call -> resolved_type = inferred_type;
                }
            }
        }
    }

    //
    // Direct parameterized return type: If the method's return type is directly
    // parameterized (e.g., Map<String, Integer> getMap()), propagate the parameterized
    // type to the method call for chained calls like getMap().get(key).
    //
    if (method && method -> return_parameterized_type && ! method_call -> resolved_parameterized_type)
    {
        method_call -> resolved_parameterized_type = method -> return_parameterized_type;
    }

    //
    // Special case: array.clone() returns the array type, not Object.
    // JLS 10.7: Every array type T[] has a public method clone() with return type T[].
    //
    if (method && base_type && base_type -> IsArray() &&
        method -> name_symbol == control.clone_name_symbol)
    {
        method_call -> resolved_type = base_type;
    }
}


//
// Processes the argument list, returning true if the list contains an
// invalid expression.
//
bool Semantic::ProcessArguments(AstArguments* args)
{
    bool bad_argument = false;
    for (unsigned i = 0; i < args -> NumArguments(); i++)
    {
        AstExpression* expr = args -> Argument(i);
        ProcessExpressionOrStringConstant(expr);
        if (expr -> symbol == control.no_type)
            bad_argument = true;
        else if (expr -> Type() == control.void_type)
        {
            ReportSemError(SemanticError::TYPE_IS_VOID, expr,
                           expr -> Type() -> Name());
            bad_argument = true;
        }
    }
    return bad_argument;
}


void Semantic::ProcessMethodInvocation(Ast* expr)
{
    AstMethodInvocation* method_call = (AstMethodInvocation*) expr;

    if (method_call -> type_arguments_opt &&
        control.option.source < JopaOption::SDK1_5)
    {
        ReportSemError(SemanticError::EXPLICIT_TYPE_ARGUMENTS_UNSUPPORTED,
                       method_call -> type_arguments_opt);
    }
    bool bad_argument = ProcessArguments(method_call -> arguments);
    if (bad_argument)
        method_call -> symbol = control.no_type;
    else ProcessMethodName(method_call);
    assert(method_call -> symbol == control.no_type ||
           ((MethodSymbol*) method_call -> symbol) -> IsTyped());
}


void Semantic::ProcessNullLiteral(Ast* expr)
{
    //
    // Null is not a compile-time constant, so don't give it a value
    //
    AstNullLiteral* null_literal = (AstNullLiteral*) expr;
    null_literal -> symbol = control.null_type;
}


void Semantic::ProcessClassLiteral(Ast* expr)
{
    TypeSymbol* this_type = ThisType();
    AstClassLiteral* class_lit = (AstClassLiteral*) expr;
    //
    // In a clone, simply return control.no_type. We are in a clone only
    // when doing something like evaluating a forward reference to a final
    // field for its constant value, but a class literal has no constant
    // value. In such cases, this method will again be invoked when we
    // finally reach the field, and then it is finally appropriate to
    // resolve the reference.
    //
    if (error && error -> InClone())
    {
        class_lit -> symbol = control.no_type;
        return;
    }
    ProcessType(class_lit -> type);
    TypeSymbol* type = class_lit -> type -> symbol;
    AddDependence(this_type, type -> BoxedType(control));
    if (type == control.no_type)
        class_lit -> symbol = control.no_type;
    else if (type -> Primitive())
    {
        if (type == control.int_type)
            class_lit -> symbol = control.Integer_TYPE_Field();
        else if (type == control.double_type)
            class_lit -> symbol = control.Double_TYPE_Field();
        else if (type == control.char_type)
            class_lit -> symbol = control.Character_TYPE_Field();
        else if (type == control.long_type)
            class_lit -> symbol = control.Long_TYPE_Field();
        else if (type == control.float_type)
            class_lit -> symbol = control.Float_TYPE_Field();
        else if (type == control.byte_type)
            class_lit -> symbol = control.Byte_TYPE_Field();
        else if (type == control.short_type)
            class_lit -> symbol = control.Short_TYPE_Field();
        else if (type == control.boolean_type)
            class_lit -> symbol = control.Boolean_TYPE_Field();
        else
        {
            assert(type == control.void_type);
            class_lit -> symbol = control.Void_TYPE_Field();
        }
    }
    else if (control.option.target < JopaOption::SDK1_5)
    {
        //
        // We have already checked that the type is accessible. Older VMs
        // require a helper method to resolve the reference.
        //
        VariableSymbol* var = this_type -> FindOrInsertClassLiteral(type);
        AstName* name = compilation_unit -> ast_pool ->
            GenName(class_lit -> class_token);
        name -> symbol = var;
        class_lit -> symbol = var;
        class_lit -> resolution_opt = name;
    }
    else class_lit -> symbol = control.Class();
}


void Semantic::ProcessThisExpression(Ast* expr)
{
    TypeSymbol* this_type = ThisType();
    AstThisExpression* this_expression = (AstThisExpression*) expr;
    AstTypeName* base = this_expression -> base_opt;
    if (base)
    {
        ProcessType(base);
        TypeSymbol* enclosing_type = base -> symbol;
        if (enclosing_type == control.no_type)
            this_expression -> symbol = control.no_type;
        else if (! enclosing_type)
        {
            ReportSemError(SemanticError::NOT_A_TYPE, base);
            this_expression -> symbol = control.no_type;
        }
        else if (enclosing_type -> ACC_INTERFACE())
        {
            ReportSemError(SemanticError::NOT_A_CLASS, base,
                           enclosing_type -> ContainingPackageName(),
                           enclosing_type -> ExternalName());
            this_expression -> symbol = control.no_type;
        }
        else if (ExplicitConstructorInvocation() &&
                 enclosing_type == this_type)
        {
            ReportSemError(SemanticError::SELF_IN_EXPLICIT_CONSTRUCTOR,
                           base -> LeftToken(),
                           this_expression -> this_token,
                           StringConstant::US_this);
            this_expression -> symbol = control.no_type;
        }
        else if (! this_type -> IsNestedIn(enclosing_type))
        {
            ReportSemError(SemanticError::ILLEGAL_THIS_FIELD_ACCESS,
                           base -> LeftToken(),
                           this_expression -> this_token,
                           enclosing_type -> ContainingPackageName(),
                           enclosing_type -> ExternalName(),
                           this_package -> PackageName(),
                           this_type -> ExternalName());
            this_expression -> symbol = control.no_type;
        }
        else if (this_type == enclosing_type)
        {
            if (StaticRegion())
            {
                ReportSemError(SemanticError::ENCLOSING_INSTANCE_NOT_ACCESSIBLE,
                               base -> LeftToken(),
                               this_expression -> this_token,
                               enclosing_type -> ContainingPackageName(),
                               enclosing_type -> ExternalName());
                this_expression -> symbol = control.no_type;
            }
            else this_expression -> symbol = this_type;
        }
        else
        {
            this_expression -> resolution_opt =
                CreateAccessToType(this_expression, enclosing_type);
            this_expression -> symbol =
                this_expression -> resolution_opt -> symbol;
        }
    }
    else // unqualified
    {
        if (ExplicitConstructorInvocation())
        {
            ReportSemError(SemanticError::SELF_IN_EXPLICIT_CONSTRUCTOR,
                           this_expression -> this_token,
                           StringConstant::US_this);
            this_expression -> symbol = control.no_type;
        }
        else if (StaticRegion())
        {
            ReportSemError(SemanticError::MISPLACED_THIS_EXPRESSION,
                           this_expression -> this_token);
            this_expression -> symbol = control.no_type;
        }
        else this_expression -> symbol = this_type;
    }
}


void Semantic::ProcessSuperExpression(Ast* expr)
{
    TypeSymbol* this_type = ThisType();
    AstSuperExpression* super_expression = (AstSuperExpression*) expr;
    AstTypeName* base = super_expression -> base_opt;
    if (base)
    {
        ProcessType(base);
        TypeSymbol* enclosing_type = base -> symbol;
        if (enclosing_type == control.no_type)
            super_expression -> symbol = control.no_type;
        else if (! enclosing_type)
        {
            ReportSemError(SemanticError::NOT_A_TYPE, base);
            super_expression -> symbol = control.no_type;
        }
        else if (enclosing_type -> ACC_INTERFACE())
        {
            ReportSemError(SemanticError::NOT_A_CLASS, base,
                           enclosing_type -> ContainingPackageName(),
                           enclosing_type -> ExternalName());
            super_expression -> symbol = control.no_type;
        }
        else if (this_type == control.Object())
        {
            ReportSemError(SemanticError::OBJECT_HAS_NO_SUPER_TYPE,
                           base -> LeftToken(),
                           super_expression -> super_token);
            super_expression -> symbol = control.no_type;
        }
        else if (ExplicitConstructorInvocation() &&
                 enclosing_type == this_type)
        {
            ReportSemError(SemanticError::SELF_IN_EXPLICIT_CONSTRUCTOR,
                           base -> LeftToken(),
                           super_expression -> super_token,
                           StringConstant::US_super);
            super_expression -> symbol = control.no_type;
        }
        else if (! this_type -> IsNestedIn(enclosing_type))
        {
            ReportSemError(SemanticError::ILLEGAL_THIS_FIELD_ACCESS,
                           base -> LeftToken(),
                           super_expression -> super_token,
                           enclosing_type -> ContainingPackageName(),
                           enclosing_type -> ExternalName(),
                           this_package -> PackageName(),
                           this_type -> ExternalName());
            super_expression -> symbol = control.no_type;
        }
        else if (this_type == enclosing_type)
        {
            if (StaticRegion())
            {
                ReportSemError(SemanticError::ENCLOSING_INSTANCE_NOT_ACCESSIBLE,
                               base -> LeftToken(),
                               super_expression -> super_token,
                               enclosing_type -> ContainingPackageName(),
                               enclosing_type -> ExternalName());
                super_expression -> symbol = control.no_type;
            }
            else super_expression -> symbol = this_type -> super;
        }
        else
        {
            super_expression -> resolution_opt =
                CreateAccessToType(super_expression, enclosing_type);
            super_expression -> symbol =
                super_expression -> resolution_opt -> symbol;
        }
    }
    else // unqualified
    {
        if (ThisType() == control.Object())
        {
            ReportSemError(SemanticError::OBJECT_HAS_NO_SUPER_TYPE,
                           super_expression -> super_token);
            super_expression -> symbol = control.no_type;
        }
        else if (ExplicitConstructorInvocation())
        {
            ReportSemError(SemanticError::SELF_IN_EXPLICIT_CONSTRUCTOR,
                           super_expression -> super_token,
                           StringConstant::US_super);
            super_expression -> symbol = control.no_type;
        }
        else if (StaticRegion())
        {
            ReportSemError(SemanticError::MISPLACED_SUPER_EXPRESSION,
                           super_expression -> super_token);
            super_expression -> symbol = control.no_type;
        }
        else super_expression -> symbol = ThisType() -> super;
    }
}


void Semantic::ProcessParenthesizedExpression(Ast* expr)
{
    AstParenthesizedExpression* parenthesized =
        (AstParenthesizedExpression*) expr;

    //
    // Do not use ProcessExpressionOrStringConstant here, to avoid generating
    // intermediate Strings - see CheckConstantString in lookup.cpp
    //
    ProcessExpression(parenthesized -> expression);
    if (parenthesized -> expression -> Type() == control.void_type)
    {
        ReportSemError(SemanticError::TYPE_IS_VOID,
                       parenthesized -> expression,
                       control.void_type -> Name());
        parenthesized -> symbol = control.no_type;
    }
    else
    {
        parenthesized -> value = parenthesized -> expression -> value;
        parenthesized -> symbol = parenthesized -> expression -> symbol;
        // Also propagate resolved_type and resolved_parameterized_type for generic type substitution
        parenthesized -> resolved_type = parenthesized -> expression -> resolved_type;
        parenthesized -> resolved_parameterized_type =
            parenthesized -> expression -> resolved_parameterized_type;
    }
}


void Semantic::UpdateLocalConstructors(TypeSymbol* inner_type)
{
    assert(inner_type -> IsLocal() &&
           (! inner_type -> Anonymous() || ! inner_type -> EnclosingType()));

    //
    // Update the constructor signatures to account for local shadow
    // parameters.
    //
    inner_type -> MarkLocalClassProcessingCompleted();
    unsigned param_count = inner_type -> NumConstructorParameters();
    if (param_count)
    {
        MethodSymbol* ctor;
        for (ctor = inner_type -> FindMethodSymbol(control.init_name_symbol);
             ctor; ctor = ctor -> next_method)
        {
            ctor -> SetSignature(control);
        }
        for (unsigned j = 0;
             j < inner_type -> NumPrivateAccessConstructors(); j++)
        {
            inner_type -> PrivateAccessConstructor(j) ->
                SetSignature(control, (inner_type -> outermost_type ->
                                       GetPlaceholderType()));
        }
    }

    //
    // Update all constructor call contexts that were pending on this class.
    // These calls are necessarily located within the body of inner_type, and
    // are calling a constructor in inner_type.
    //
    for (unsigned i = 0;
         i < inner_type -> NumLocalConstructorCallEnvironments(); i++)
    {
        SemanticEnvironment* env =
            inner_type -> LocalConstructorCallEnvironment(i);
        state_stack.Push(env);
        AstArguments* args = env -> args;

        args -> AllocateLocalArguments(param_count);
        for (unsigned k = 0; k < param_count; k++)
        {
            AstName* name = compilation_unit ->
                ast_pool -> GenName(args -> right_parenthesis_token);
            VariableSymbol* accessor =
                FindLocalVariable(inner_type -> ConstructorParameter(k),
                                  ThisType());
            name -> symbol = accessor;
            TypeSymbol* owner = accessor -> ContainingType();
            if (owner != ThisType())
                CreateAccessToScopedVariable(name, owner);
            args -> AddLocalArgument(name);
        }
        if (ThisType() -> Anonymous() &&
            ! ThisType() -> LocalClassProcessingCompleted())
        {
            UpdateLocalConstructors(ThisType());
        }
        state_stack.Pop();
    }
}


//
// This creates the default constructor for an anonymous class, and sets
// the resolution_opt field of the original to a generated instance creation
// expression that has been adjusted for compilation purposes.
//
void Semantic::GetAnonymousConstructor(AstClassCreationExpression* class_creation,
                                       TypeSymbol* anonymous_type)
{
    TokenIndex left_loc = class_creation -> class_type -> LeftToken();
    TokenIndex right_loc =
        class_creation -> arguments -> right_parenthesis_token;

    state_stack.Push(anonymous_type -> semantic_environment);
    TypeSymbol* super_type = anonymous_type -> super;
    MethodSymbol* super_constructor = FindConstructor(super_type,
                                                      class_creation,
                                                      left_loc, right_loc);
    if (! super_constructor)
    {
        class_creation -> class_type -> symbol = control.no_type;
        state_stack.Pop();
        return;
    }
    assert(super_constructor -> IsTyped());

    //
    // Make replacement class instance creation expression.
    //
    AstArguments* resolution_args = compilation_unit -> ast_pool ->
        GenArguments(class_creation -> arguments -> left_parenthesis_token,
                     right_loc);

    AstClassCreationExpression* resolution =
        compilation_unit -> ast_pool -> GenClassCreationExpression();
    resolution -> new_token = class_creation -> new_token;
    // TODO: WARNING: sharing of subtrees...
    resolution -> class_type = class_creation -> class_type;
    resolution -> arguments = resolution_args;
    resolution -> symbol = anonymous_type;
    class_creation -> resolution_opt = resolution;

    //
    // Make constructor symbol. The associated symbol table will not contain
    // too many elements...
    //
    BlockSymbol* block_symbol =
        new BlockSymbol(super_constructor -> NumFormalParameters() + 3);
    block_symbol -> max_variable_index = 1; // A spot for "this".

    MethodSymbol* constructor =
        anonymous_type -> InsertMethodSymbol(control.init_name_symbol);
    constructor -> SetType(anonymous_type);
    constructor -> SetContainingType(anonymous_type);
    constructor -> SetBlockSymbol(block_symbol);

    //
    // Anonymous class constructors may throw any exception listed in the
    // superclass; but this list may be expanded later since the anonymous
    // constructor also throws anything possible in instance initializers.
    //
    for (unsigned i = 0; i < super_constructor -> NumThrows(); i++)
        constructor -> AddThrows(super_constructor -> Throws(i));

    //
    // If we are in a static region, the anonymous constructor does not need
    // a this$0 argument. Otherwise, a this$0 argument that points to an
    // instance of the immediately enclosing class is required.
    //
    if (anonymous_type -> EnclosingType())
    {
        VariableSymbol* this0_variable =
            block_symbol -> InsertVariableSymbol(control.this_name_symbol);
        this0_variable -> SetType(anonymous_type -> EnclosingType());
        this0_variable -> SetOwner(constructor);
        this0_variable -> SetFlags(AccessFlags::ACCESS_FINAL |
                                   AccessFlags::ACCESS_SYNTHETIC);
        this0_variable -> SetLocalVariableIndex(block_symbol ->
                                                max_variable_index++);
        this0_variable -> MarkComplete();
        AstThisExpression* this0_expression =
            compilation_unit -> ast_pool -> GenThisExpression(left_loc);
        this0_expression -> symbol = anonymous_type -> EnclosingType();
        resolution -> base_opt = this0_expression;
    }

    //
    // Create an explicit call to the superconstructor, passing any necessary
    // shadow variables or enclosing instances.
    //
    AstArguments* super_args = compilation_unit -> ast_pool ->
        GenArguments(class_creation -> arguments -> left_parenthesis_token,
                     right_loc);

    AstSuperCall* super_call = compilation_unit -> ast_pool -> GenSuperCall();
    if (super_constructor -> ACC_PRIVATE())
    {
        super_constructor =
            super_type -> GetReadAccessConstructor(super_constructor);
        super_args -> AddNullArgument();
    }

    // Use initial base_opt.
    super_call -> base_opt = class_creation -> base_opt;
    super_call -> super_token = class_creation -> new_token;
    super_call -> arguments = super_args;
    super_call -> semicolon_token = right_loc;
    super_call -> symbol = super_constructor;

    AstClassBody* class_body = class_creation -> class_body_opt;

    //
    // Construct the default constructor of the anonymous type.
    //
    AstMethodBody* constructor_block =
        compilation_unit -> ast_pool -> GenMethodBody();
    // This symbol table will be empty.
    constructor_block -> block_symbol =
        constructor -> block_symbol -> InsertBlockSymbol(0);
    constructor_block -> left_brace_token = class_body -> left_brace_token;
    constructor_block -> right_brace_token = class_body -> left_brace_token;
    constructor_block -> explicit_constructor_opt = super_call;
    constructor_block -> AllocateStatements(1); // for the generated return

    AstMethodDeclarator* method_declarator =
        compilation_unit -> ast_pool -> GenMethodDeclarator();
    method_declarator -> identifier_token = left_loc;
    method_declarator -> left_parenthesis_token =
        class_creation -> arguments -> left_parenthesis_token;
    method_declarator -> right_parenthesis_token = right_loc;

    AstConstructorDeclaration* constructor_declaration  =
        compilation_unit -> ast_pool -> GenConstructorDeclaration();
    constructor_declaration -> constructor_declarator = method_declarator;
    constructor_declaration -> constructor_body = constructor_block;
    constructor_declaration -> constructor_symbol = constructor;

    constructor -> declaration = constructor_declaration;
    class_body -> default_constructor = constructor_declaration;


    //
    // Update the enclosing instance of the supertype.
    //
    unsigned num_args = class_creation -> arguments -> NumArguments();
    if (class_creation -> base_opt)
    {
        VariableSymbol* super_this0_variable =
            block_symbol -> InsertVariableSymbol(control.MakeParameter(0));
        super_this0_variable -> SetACC_SYNTHETIC();
        super_this0_variable -> SetType(super_call -> base_opt -> Type());
        super_this0_variable -> SetOwner(constructor);
        super_this0_variable -> SetLocalVariableIndex(block_symbol ->
                                                      max_variable_index++);
        super_this0_variable -> MarkComplete();

        resolution_args -> AllocateArguments(num_args + 1);
        resolution_args -> AddArgument(class_creation -> base_opt);
        constructor -> AddFormalParameter(super_this0_variable);

        AstName* name = compilation_unit -> ast_pool ->
            GenName(class_creation -> new_token);
        name -> symbol = super_this0_variable;
        super_call -> base_opt = name;
    }
    else resolution_args -> AllocateArguments(num_args);
    super_args -> AllocateArguments(super_constructor ->
                                    NumFormalParameters());

    //
    // Next, simply pass all parameters through to the superclass.
    //
    for (unsigned j = 0; j < super_constructor -> NumFormalParameters(); j++)
    {
        VariableSymbol* param = super_constructor -> FormalParameter(j);
        VariableSymbol* symbol =
            block_symbol -> InsertVariableSymbol(param -> Identity());
        symbol -> SetType(param -> Type());
        symbol -> SetOwner(constructor);
        symbol -> SetLocalVariableIndex(block_symbol -> max_variable_index++);
        symbol -> MarkComplete();
        if (control.IsDoubleWordType(symbol -> Type()))
            block_symbol -> max_variable_index++;

        resolution_args -> AddArgument(class_creation -> arguments -> Argument(j));
        constructor -> AddFormalParameter(symbol);
        AstName* name = compilation_unit -> ast_pool ->
            GenName(class_creation -> new_token);
        name -> symbol = symbol;
        super_args -> AddArgument(name);
    }

    //
    // Worry about shadow variables in the super type
    //
    if (super_type -> IsLocal())
    {
        unsigned param_count = super_type -> NumConstructorParameters();
        if (super_type -> LocalClassProcessingCompleted() && param_count)
        {
            super_args -> AllocateLocalArguments(param_count);
            for (unsigned k = 0; k < param_count; k++)
            {
                //
                // We may need to create a shadow in the outermost
                // local class enclosing the variable.
                //
                AstName* name = compilation_unit ->
                    ast_pool -> GenName(super_call -> super_token);
                VariableSymbol* accessor =
                    FindLocalVariable(super_type -> ConstructorParameter(k),
                                      anonymous_type);
                name -> symbol = accessor;
                TypeSymbol* owner = accessor -> ContainingType();
                if (owner != anonymous_type)
                    CreateAccessToScopedVariable(name, owner);
                super_args -> AddLocalArgument(name);
            }
        }
        else
        {
            //
            // We are within body of super_type; save processing for
            // later, since not all shadows may be known yet. See
            // ProcessClassDeclaration.
            //
            super_type -> AddLocalConstructorCallEnvironment
                (GetEnvironment(super_call -> arguments));
        }
    }
    //
    // We set the signature of the constructor now, although it may be modified
    // later if this is in a local constructor call environment.
    //
    constructor -> SetSignature(control);
    state_stack.Pop();
}

//
// super_type is the type specified in the anonymous constructor,
// which is the supertype of the created anonymous type.
//
TypeSymbol* Semantic::GetAnonymousType(AstClassCreationExpression* class_creation,
                                       TypeSymbol* super_type)
{
    //
    // In a clone, simply return control.no_type. We are in a clone only when
    // doing something like evaluating a forward reference to a final field for
    // its constant value, but an anonymous class has no constant value. In
    // such cases, this method will again be invoked when we finally reach the
    // field, and then it is finally appropriate to create the class.
    //
    if (error && error -> InClone())
        return control.no_type;

    TypeSymbol* this_type = ThisType();
    AstClassBody* class_body = class_creation -> class_body_opt;
    assert(class_body);
    TypeSymbol* outermost_type = this_type -> outermost_type;

    //
    // Anonymous and local classes can clash if we don't use both when
    // determining the id number of this class.
    //
    IntToWstring value(this_type -> NumLocalTypes() +
                       this_type -> NumAnonymousTypes() + 1);

    int length = this_type -> ExternalNameLength() + 1 +
        value.Length(); // +1 for $
    wchar_t* anonymous_name = new wchar_t[length + 1]; // +1 for '\0'
    wcscpy(anonymous_name, this_type -> ExternalName());
    wcscat(anonymous_name, (control.option.target < JopaOption::SDK1_5
                            ? StringConstant::US_DS : StringConstant::US_MI));
    wcscat(anonymous_name, value.String());

    NameSymbol* name_symbol = control.FindOrInsertName(anonymous_name, length);
    delete [] anonymous_name;

    assert(! ThisMethod() || LocalSymbolTable().Top());

    TypeSymbol* anon_type =
        this_type -> InsertAnonymousTypeSymbol(name_symbol);
    anon_type -> MarkAnonymous();
    anon_type -> outermost_type = outermost_type;
    anon_type -> supertypes_closure = new SymbolSet;
    anon_type -> subtypes_closure = new SymbolSet;
    anon_type -> semantic_environment =
        new SemanticEnvironment(this, anon_type, state_stack.Top());
    anon_type -> declaration = class_body;
    anon_type -> declaration -> semantic_environment =
        anon_type -> semantic_environment;
    anon_type -> file_symbol = source_file_symbol;
    if (ThisMethod())
        anon_type -> SetOwner(ThisMethod());
    else if (ThisVariable())
    {
        //
        // Creating an anonymous class in a field initializer necessarily
        // requires non-trivial code, so the initializer method should
        // exist as the owner of this type.
        //
        assert(ThisVariable() -> ACC_STATIC()
               ? this_type -> static_initializer_method
               : (this_type -> FindMethodSymbol(control.
                                                block_init_name_symbol)));
        anon_type ->
            SetOwner(ThisVariable() -> ACC_STATIC()
                     ? this_type -> static_initializer_method
                     : (this_type ->
                        FindMethodSymbol(control.block_init_name_symbol)));
    }
    else
    {
        assert(class_creation -> generated);
        anon_type -> SetOwner(this_type);
    }

    //
    // Add 3 extra elements for padding. Need a default constructor and
    // other support elements.
    //
    anon_type -> SetSymbolTable(class_body -> NumClassBodyDeclarations() + 3);
    anon_type -> SetLocation();
    anon_type -> SetSignature(control);

    //
    // By JLS2 15.9.5, an anonymous class is implicitly final, but never
    // static. However, the anonymous class only needs access to its enclosing
    // instance if it is not in a static context.
    //
    anon_type -> SetACC_FINAL();
    if (! StaticRegion())
        anon_type -> InsertThis0();

    if (super_type -> ACC_INTERFACE())
    {
        anon_type -> AddInterface(super_type);
        anon_type -> super = control.Object();
        control.Object() -> subtypes -> AddElement(anon_type);
    }
    else anon_type -> super = super_type;

    // Store parameterized superclass for Signature attribute generation
    // (needed for TypeToken pattern and similar reflection-based code)
    AstTypeName* class_type_name = class_creation -> class_type;
    if (class_type_name -> parameterized_type)
    {
        anon_type -> SetParameterizedSuper(class_type_name -> parameterized_type);
    }

    AddDependence(anon_type, super_type);
    super_type -> subtypes -> AddElement(anon_type);
    if (super_type -> ACC_FINAL())
    {
         ReportSemError(SemanticError::SUPER_IS_FINAL,
                        class_creation -> class_type,
                        super_type -> ContainingPackageName(),
                        super_type -> ExternalName());
         anon_type -> MarkBad();
    }
    else if (super_type -> Bad())
        anon_type -> MarkBad();

    this_type -> AddAnonymousType(anon_type);

    //
    // Provide the default constructor. For now, we don't worry about accessors
    // to final local variables; those are inserted later when completing
    // the class instance creation processing. Also, the throws clause may
    // expand after processing instance initializer blocks. We keep on
    // processing, even if the constructor failed, to detect other semantic
    // errors in the anonymous class body.
    //
    GetAnonymousConstructor(class_creation, anon_type);

    //
    // Now process the body of the anonymous class !!!
    //
    CheckNestedMembers(anon_type, class_body);
    ProcessTypeHeaders(class_body, anon_type);

    //
    // If the class body has not yet been parsed, do so now.
    //
    if (class_body -> UnparsedClassBodyCast())
    {
        if (! control.parser -> InitializerParse(lex_stream, class_body))
             compilation_unit -> MarkBad();
        else
        {
            ProcessMembers(class_body);
            CompleteSymbolTable(class_body);
        }

        if (! control.parser -> BodyParse(lex_stream, class_body))
            compilation_unit -> MarkBad();
        else ProcessExecutableBodies(class_body);
    }
    else // The relevant bodies have already been parsed
    {
        ProcessMembers(class_body);
        CompleteSymbolTable(class_body);
        ProcessExecutableBodies(class_body);
    }

    //
    // If we failed to provide a default constructor, this is as far as
    // we can go.
    //
    if (class_creation -> class_type -> symbol == control.no_type)
        return control.no_type;

    //
    // Finally, mark the class complete, in order to add any shadow variable
    // parameters to the constructor.
    //
    if (! super_type -> IsLocal() ||
        super_type -> LocalClassProcessingCompleted() ||
        anon_type -> EnclosingType())
    {
        if (anon_type -> NumConstructorParameters() && ! anon_type -> Bad())
        {
            class_body -> default_constructor -> constructor_symbol ->
                SetSignature(control);
        }
        anon_type -> MarkLocalClassProcessingCompleted();
    }
    return anon_type;
}


void Semantic::ProcessClassCreationExpression(Ast* expr)
{
    AstClassCreationExpression* class_creation =
        (AstClassCreationExpression*) expr;
    unsigned i;

    //
    // For an anonymous type, the qualifier determines the enclosing instance
    // of the supertype; as the enclosing instance of the anonymous class (if
    // present) is the current class. We update actual_type after this.
    //
    AstName* actual_type = class_creation -> class_type -> name;
    TypeSymbol* type;
    if (class_creation -> base_opt)
    {
        ProcessExpression(class_creation -> base_opt);
        TypeSymbol* enclosing_type = class_creation -> base_opt -> Type();
        if (! enclosing_type -> IsSubclass(control.Object()))
        {
            if (enclosing_type != control.no_type)
                ReportSemError(SemanticError::TYPE_NOT_REFERENCE,
                               class_creation -> base_opt,
                               enclosing_type -> ExternalName());
            enclosing_type = control.no_type;
        }

        //
        // The grammar guarantees that the actual type is a simple name.
        //
        type = MustFindNestedType(enclosing_type, actual_type);
        if (type -> ACC_INTERFACE())
        {
            ReportSemError(SemanticError::INTERFACE_NOT_INNER_CLASS,
                           actual_type, type -> ContainingPackageName(),
                           type -> ExternalName());
            type = control.no_type;
        }
        else if (type -> ACC_STATIC())
        {
            ReportSemError(SemanticError::STATIC_NOT_INNER_CLASS,
                           actual_type, type -> ContainingPackageName(),
                           type -> ExternalName());
            type = control.no_type;
        }
    }
    else
    {
        ProcessType(class_creation -> class_type);
        type = class_creation -> class_type -> symbol;
        if (type -> EnclosingType())
        {
            AstThisExpression* this_expr = compilation_unit -> ast_pool ->
                GenThisExpression(class_creation -> new_token);
            this_expr -> resolution_opt =
                CreateAccessToType(class_creation, type -> EnclosingType());
            this_expr -> symbol = this_expr -> resolution_opt -> symbol;
            class_creation -> base_opt = this_expr;
        }
    }

    //
    // Check the arguments to the constructor.
    //
    if (class_creation -> type_arguments_opt &&
        control.option.source < JopaOption::SDK1_5)
    {
        ReportSemError(SemanticError::EXPLICIT_TYPE_ARGUMENTS_UNSUPPORTED,
                       class_creation -> type_arguments_opt);
    }
    ProcessArguments(class_creation -> arguments);

    //
    // Create the anonymous class now, if needed; then check that the type
    // can be constructed. A side effect of creating the anonymous class is
    // building a resolution constructor invocation that does not have a body;
    // this new constructor is necessary to call parameters in the correct
    // order, when the superclass of the anonymous class has an enclosing
    // instance.
    //
    if (type -> IsEnum())
    {
        ReportSemError(SemanticError::CANNOT_CONSTRUCT_ENUM, actual_type,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
        type = control.no_type;
    }
    else if (class_creation -> class_body_opt)
    {
        type = GetAnonymousType(class_creation, type);
        class_creation -> symbol = type;
        if (type != control.no_type)
            class_creation = class_creation -> resolution_opt;
    }
    else if (type -> ACC_INTERFACE())
    {
        ReportSemError(SemanticError::NOT_A_CLASS, actual_type,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
        type = control.no_type;
    }
    else if (type -> ACC_ABSTRACT())
    {
        ReportSemError(SemanticError::ABSTRACT_TYPE_CREATION, actual_type,
                       type -> ExternalName());
    }

    MethodSymbol* ctor =
        FindConstructor(type, class_creation, actual_type -> LeftToken(),
                        class_creation -> arguments -> right_parenthesis_token);
    //
    // Convert the arguments to the correct types.
    //
    if (ctor)
    {
        assert(ctor -> IsTyped());
        class_creation -> symbol = ctor;

        if (class_creation -> base_opt)
        {
            assert(CanAssignmentConvertReference(ctor -> containing_type -> EnclosingType(),
                                                 class_creation -> base_opt -> Type()));
            class_creation -> base_opt =
                ConvertToType(class_creation -> base_opt,
                              ctor -> containing_type -> EnclosingType());
        }
        MethodInvocationConversion(class_creation -> arguments, ctor);

        //
        // Process the throws clause.
        //
        SymbolSet* exception_set = TryExceptionTableStack().Top();
        for (i = 0; i < ctor -> NumThrows(); i++)
        {
            TypeSymbol* exception = ctor -> Throws(i);
            if (exception_set)
                exception_set -> AddElement(exception);

            if (UncaughtException(exception))
                ReportSemError((class_creation -> class_body_opt
                                ? SemanticError::UNCAUGHT_ANONYMOUS_CONSTRUCTOR_EXCEPTION
                                : SemanticError::UNCAUGHT_CONSTRUCTOR_EXCEPTION),
                               actual_type, type -> ExternalName(),
                               exception -> ContainingPackageName(),
                               exception -> ExternalName(),
                               UncaughtExceptionContext());
        }

        if (ctor -> ACC_PRIVATE() && ThisType() != type)
        {
            //
            // Add extra argument for read access constructor.
            //
            assert(ThisType() -> outermost_type == type -> outermost_type);
            ctor = type -> GetReadAccessConstructor(ctor);
            class_creation -> symbol = ctor;
            class_creation -> arguments -> AddNullArgument();
        }
    }
    else
    {
        //
        // No constructor was found (possibly because the type was not found),
        // so we don't know what exceptions could be thrown if the user fixes
        // the prior errors.
        //
        SymbolSet* exception_set = TryExceptionTableStack().Top();
        if (exception_set)
            exception_set -> AddElement(control.no_type);
        class_creation -> symbol = control.no_type;
    }

    //
    // A local type may use enclosed local variables. If so, we must add
    // the parameters which allow the local type to initialize its shadows.
    //
    if (type -> IsLocal())
    {
        if (type -> LocalClassProcessingCompleted())
        {
            unsigned param_count = type -> NumConstructorParameters();
            class_creation -> arguments -> AllocateLocalArguments(param_count);
            for (i = 0; i < param_count; i++)
            {
                //
                // Are we currently within the body of the method that
                // contains the local variable in question? If not, we may need
                // to create a shadow in the outermost local class enclosing
                // the variable.
                //
                AstName* name = compilation_unit ->
                    ast_pool -> GenName(class_creation -> new_token);
                VariableSymbol* accessor =
                    FindLocalVariable(type -> ConstructorParameter(i),
                                      ThisType());
                name -> symbol = accessor;
                TypeSymbol* owner = accessor -> ContainingType();
                if (owner != ThisType())
                    CreateAccessToScopedVariable(name, owner);
                class_creation -> arguments -> AddLocalArgument(name);
            }
        }
        else
        {
            //
            // We are within body of type; save processing for later, since
            // not all shadows may be known yet. See ProcessClassDeclaration
            // in body.cpp.
            //
            type -> AddLocalConstructorCallEnvironment
                (GetEnvironment(class_creation -> arguments));
        }
    }
}


void Semantic::ProcessArrayCreationExpression(Ast* expr)
{
    AstArrayCreationExpression* array_creation =
        (AstArrayCreationExpression*) expr;
    //
    // Either we have an initializer, or we have dimension expressions and
    // optional brackets.
    //
    assert(array_creation -> array_initializer_opt ?
           (! array_creation -> NumDimExprs() &&
            ! array_creation -> NumBrackets())
           : array_creation -> NumDimExprs());
    ProcessType(array_creation -> array_type);
    TypeSymbol* type = array_creation -> array_type -> symbol;
    unsigned dims = type -> num_dimensions +
        array_creation -> NumDimExprs() + array_creation -> NumBrackets();
    type = type -> GetArrayType(this, dims);
    array_creation -> symbol = type;

    for (unsigned i = 0; i < array_creation -> NumDimExprs(); i++)
    {
        AstDimExpr* dim_expr = array_creation -> DimExpr(i);
        ProcessExpression(dim_expr -> expression);
        AstExpression* expr =
            PromoteUnaryNumericExpression(dim_expr -> expression);
        if (expr -> Type() != control.int_type &&
            expr -> symbol != control.no_type)
        {
            ReportSemError(SemanticError::TYPE_NOT_INTEGER,
                           dim_expr -> expression,
                           expr -> Type() -> ContainingPackageName(),
                           expr -> Type() -> ExternalName());
            array_creation -> symbol = control.no_type;
        }
        dim_expr -> expression = expr;
        if (expr -> IsConstant() &&
            expr -> Type() == control.int_type &&
            (DYNAMIC_CAST<IntLiteralValue*> (expr -> value)) -> value < 0)
        {
            ReportSemError(SemanticError::NEGATIVE_ARRAY_SIZE,
                           dim_expr -> expression);
        }
    }

    if (array_creation -> array_initializer_opt)
        ProcessArrayInitializer(array_creation -> array_initializer_opt, type);
}



} // Close namespace Jopa block
