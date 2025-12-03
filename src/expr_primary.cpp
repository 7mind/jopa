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

        // Check if base has a secondary type from intersection (wildcard capture).
        // If so, suppress error on first lookup so we can try secondary type.
        TypeSymbol* secondary_type = base -> secondary_resolved_type;
        bool has_secondary = secondary_type && secondary_type != type;
        shadow = FindMethodInType(type, method_call, NULL, has_secondary);
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
            // Method not found in primary type. Check secondary type from
            // intersection type (wildcard capture). For G1<? extends I2> where
            // G1<T extends I1>, the captured type has bound I1 & I2.
            if (has_secondary)
            {
                shadow = FindMethodInType(secondary_type, method_call);
                MethodSymbol* method = (shadow ? shadow -> method_symbol : NULL);
                if (method)
                {
                    MethodInvocationConversion(method_call -> arguments, method);
                    method_call -> symbol = method;
                    return shadow;
                }
                // Not found in either type - report error now
                ReportMethodNotFound(method_call, type);
            }
            method_call -> symbol = control.no_type;
        }
    }
    return shadow;
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

        SymbolSet exceptions(method -> NumThrows());
        int i, j;
        // First, the base set - with exception type parameter inference
        for (i = method -> NumThrows(); --i >= 0; )
        {
            TypeSymbol* ex_type = method -> Throws(i);

            // Check if this exception type could be a type parameter's bound (erased type).
            // For a method like <E extends Throwable> void m() throws E, the throws clause
            // contains Throwable (the erasure). We need to check if the method has a type
            // parameter with this bound and try to infer the actual type from arguments.

            if (method -> NumTypeParameters() > 0 &&
                ex_type && ex_type -> IsSubclass(control.Throwable()))
            {
                // Check each type parameter to find one that matches this exception type
                for (unsigned tp = 0; tp < method -> NumTypeParameters(); tp++)
                {
                    TypeParameterSymbol* type_param = method -> TypeParameter(tp);
                    if (! type_param)
                        continue;

                    // Check if this type parameter's bound matches the exception type
                    // ErasedType() returns the first bound, or NULL for unbounded (Object)
                    TypeSymbol* bound = type_param -> ErasedType();
                    if (! bound)
                        bound = control.Object();

                    if (bound != ex_type)
                        continue;

                    // Found a matching type parameter. Try to infer actual type from arguments.
                    // Scan arguments to find parameterized types containing this type parameter.
                    if (method_call -> arguments)
                    {
                        unsigned num_args = method_call -> arguments -> NumArguments();
                        for (unsigned arg_idx = 0; arg_idx < num_args && ex_type == method -> Throws(i); arg_idx++)
                        {
                            AstExpression* arg = method_call -> arguments -> Argument(arg_idx);
                            if (! arg)
                                continue;

                            // Unwrap cast expressions to find the original expression
                            // (MethodInvocationConversion wraps arguments in casts)
                            AstExpression* unwrapped = arg;
                            while (unwrapped -> CastExpressionCast())
                            {
                                unwrapped = unwrapped -> CastExpressionCast() -> expression;
                            }

                            TypeSymbol* arg_type = unwrapped -> Type();
                            if (! arg_type || arg_type == control.no_type)
                                continue;

                            // Check parameterized super (for anonymous classes)
                            ParameterizedType* param_type = arg_type -> GetParameterizedSuper();
                            if (param_type)
                            {
                                // Search for the type parameter in this parameterized type
                                TypeSymbol* generic = param_type -> generic_type;
                                if (generic)
                                {
                                    for (unsigned k = 0; k < generic -> NumTypeParameters(); k++)
                                    {
                                        TypeParameterSymbol* gtp = generic -> TypeParameter(k);
                                        // Match by name since type parameters are separate instances
                                        if (gtp && type_param -> name_symbol == gtp -> name_symbol &&
                                            k < param_type -> NumTypeArguments())
                                        {
                                            Type* inferred = param_type -> TypeArgument(k);
                                            if (inferred)
                                            {
                                                TypeSymbol* inferred_type = inferred -> Erasure();
                                                if (inferred_type && inferred_type -> IsSubclass(control.Throwable()))
                                                    ex_type = inferred_type;
                                            }
                                            break;
                                        }
                                    }
                                }
                            }

                            // Check parameterized interfaces
                            if (ex_type == method -> Throws(i))
                            {
                                for (unsigned iface_idx = 0; iface_idx < arg_type -> NumInterfaces(); iface_idx++)
                                {
                                    ParameterizedType* iface_param = arg_type -> ParameterizedInterface(iface_idx);
                                    if (! iface_param)
                                        continue;

                                    TypeSymbol* iface_generic = iface_param -> generic_type;
                                    if (! iface_generic)
                                        continue;

                                    for (unsigned k = 0; k < iface_generic -> NumTypeParameters(); k++)
                                    {
                                        TypeParameterSymbol* gtp = iface_generic -> TypeParameter(k);
                                        if (gtp && type_param -> name_symbol == gtp -> name_symbol &&
                                            k < iface_param -> NumTypeArguments())
                                        {
                                            Type* inferred = iface_param -> TypeArgument(k);
                                            if (inferred)
                                            {
                                                TypeSymbol* inferred_type = inferred -> Erasure();
                                                if (inferred_type && inferred_type -> IsSubclass(control.Throwable()))
                                                    ex_type = inferred_type;
                                            }
                                            break;
                                        }
                                    }

                                    if (ex_type != method -> Throws(i))
                                        break;
                                }
                            }
                        }
                    }
                    break; // Found the type parameter for this exception
                }
            }

            exceptions.AddElement(ex_type);
        }
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

    //
    // Special handling for Object.getClass()
    // JLS 4.3.2: The type of a method invocation expression of getClass is Class<? extends |T|>,
    // where T is the class or interface that was searched for getClass.
    //
    if (method == control.Object_getClassMethod())
    {
        TypeSymbol* receiver_type = base_type;
        if (! receiver_type && ! base)
            receiver_type = ThisType();

        if (receiver_type && receiver_type != control.no_type)
        {
            // Construct Class<? extends |receiver_type|>
            // Note: receiver_type is already the erasure (TypeSymbol) of the receiver expression's type
            WildcardType* wildcard = new WildcardType(WildcardType::EXTENDS, new Type(receiver_type));
            Type* type_arg = new Type(wildcard);
            Tuple<Type*>* type_args = new Tuple<Type*>(1);
            type_args -> Next() = type_arg;
            
            method_call -> resolved_parameterized_type = new ParameterizedType(control.Class(), type_args);
        }
    }

    if (method && method -> return_type_param_index >= 0)
    {
        unsigned param_index = (unsigned) method -> return_type_param_index;
        ParameterizedType* param_type = NULL;

        // Case 1: Check if the receiver variable has a parameterized type
        // Also check if the base expression has resolved_parameterized_type (for chained calls)
        if (base)
        {
            VariableSymbol* var = base -> symbol ? base -> symbol -> VariableCast() : NULL;
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

            //
            // Case 1b: Check if the base type is a method type parameter with a parameterized bound.
            // For <T extends FooList<? super Bar>> void m(T t), when calling t.get(0),
            // the variable t has type T, which erases to FooList. We need to find the
            // parameterized bound FooList<? super Bar> from the type parameter.
            //
            if (! base_param_type && base_type && ThisMethod())
            {
                MethodSymbol* this_method = ThisMethod();
                for (unsigned i = 0; i < this_method -> NumTypeParameters() && ! base_param_type; i++)
                {
                    TypeParameterSymbol* type_param = this_method -> TypeParameter(i);
                    if (type_param && type_param -> parameterized_bounds &&
                        type_param -> parameterized_bounds -> Length() > 0)
                    {
                        // Check if base_type matches this type parameter's erasure
                        TypeSymbol* erased_bound = type_param -> ErasedType();
                        if (erased_bound && erased_bound == base_type)
                        {
                            // Found a matching type parameter - use its parameterized bound
                            Type* param_bound = type_param -> ParameterizedBound(0);
                            if (param_bound && param_bound -> IsParameterized())
                            {
                                base_param_type = param_bound -> GetParameterizedType();
                            }
                        }
                    }
                }
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

                                                            // Wildcard capture: For ? super X or unbounded ?,
                                                            // use the type parameter's bound instead of Object
                                                            if (substituted_arg -> IsWildcard())
                                                            {
                                                                WildcardType* wildcard = substituted_arg -> GetWildcard();
                                                                if (wildcard -> IsSuper() || wildcard -> IsUnbounded())
                                                                {
                                                                    // Get the bound from current_generic's type parameter
                                                                    if (k < current_generic -> NumTypeParameters())
                                                                    {
                                                                        TypeParameterSymbol* tp = current_generic -> TypeParameter(k);
                                                                        if (tp)
                                                                        {
                                                                            TypeSymbol* bound = tp -> ErasedType();
                                                                            if (bound)
                                                                                result = bound;
                                                                            else
                                                                                result = control.Object();
                                                                        }
                                                                    }
                                                                }
                                                            }

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

                                                                // Wildcard capture: For ? super X or unbounded ?,
                                                                // use the type parameter's bound instead of Object
                                                                if (substituted_arg -> IsWildcard())
                                                                {
                                                                    WildcardType* wildcard = substituted_arg -> GetWildcard();
                                                                    if (wildcard -> IsSuper() || wildcard -> IsUnbounded())
                                                                    {
                                                                        if (k < current_generic -> NumTypeParameters())
                                                                        {
                                                                            TypeParameterSymbol* tp = current_generic -> TypeParameter(k);
                                                                            if (tp)
                                                                            {
                                                                                TypeSymbol* bound = tp -> ErasedType();
                                                                                if (bound)
                                                                                    result = bound;
                                                                                else
                                                                                    result = control.Object();
                                                                            }
                                                                        }
                                                                    }
                                                                }

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

                //
                // Wildcard capture: For wildcards, the correct upper bound should
                // come from the corresponding type parameter's bound (JLS 5.1.10).
                //
                // For ? super X or unbounded ?, use the type parameter's bound.
                // For ? extends X, use the intersection of X and the type parameter's
                // bound. Since we can't represent intersections, we use the type
                // parameter's bound to ensure its contract is honored.
                //
                // Example: G1<T extends I1> and G1<? extends I2>
                // The captured type should have upper bound glb(I1, I2) = I1 & I2.
                // We use I1 (type param bound) to make T's methods available.
                //
                if (type_arg -> IsWildcard())
                {
                    WildcardType* wildcard = type_arg -> GetWildcard();
                    TypeSymbol* generic_type = param_type -> generic_type;
                    if (generic_type && param_index < generic_type -> NumTypeParameters())
                    {
                        TypeParameterSymbol* type_param = generic_type -> TypeParameter(param_index);
                        if (type_param)
                        {
                            TypeSymbol* type_param_bound = type_param -> ErasedType();
                            if (wildcard -> IsSuper() || wildcard -> IsUnbounded())
                            {
                                // Use type parameter's bound directly
                                if (type_param_bound)
                                    substituted = type_param_bound;
                                else
                                    substituted = control.Object();
                            }
                            else if (wildcard -> IsExtends() && type_param_bound)
                            {
                                // For ? extends X, captured type has bound glb(X, type_param_bound).
                                // We use type_param_bound as primary to ensure type parameter's
                                // contract methods are available. The wildcard's explicit bound X
                                // is stored as secondary to enable intersection type method lookup.
                                TypeSymbol* wildcard_bound = substituted; // Original erasure = wildcard bound
                                substituted = type_param_bound;
                                // Store the wildcard bound for secondary method lookup
                                if (wildcard_bound && wildcard_bound != type_param_bound)
                                    method_call -> secondary_resolved_type = wildcard_bound;
                            }
                        }
                    }
                }

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
        Type* inferred_full_type = NULL;  // Track full Type* for parameterized return types

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

                        // For parameterized type arguments like Iterable<G>, extract the type argument
                        // to infer the method's type parameter. Check multiple sources for parameterized type.
                        ParameterizedType* arg_param_type = NULL;

                        // Check if arg is a variable with a parameterized type
                        if (arg -> symbol)
                        {
                            VariableSymbol* var = arg -> symbol -> VariableCast();
                            if (var && var -> parameterized_type)
                                arg_param_type = var -> parameterized_type;
                        }
                        // Check if arg expression has resolved_parameterized_type
                        if (! arg_param_type && arg -> resolved_parameterized_type)
                            arg_param_type = arg -> resolved_parameterized_type;

                        // If we have a parameterized type with type arguments, extract the first one
                        // For Iterable<G>, extract G as the inferred type
                        if (arg_param_type && arg_param_type -> NumTypeArguments() > 0)
                        {
                            Type* first_type_arg = arg_param_type -> TypeArgument(0);
                            if (first_type_arg && ! first_type_arg -> IsWildcard())
                            {
                                TypeSymbol* type_arg_erasure = first_type_arg -> Erasure();
                                if (type_arg_erasure && type_arg_erasure != control.Object())
                                {
                                    arg_type = type_arg_erasure;
                                    // Track the full Type* for constructing resolved_parameterized_type
                                    // This is important when first_type_arg is a type parameter
                                    inferred_full_type = first_type_arg;
                                }
                            }
                        }

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

            // Construct resolved_parameterized_type for chained method calls
            // For <T> Set<T> getSet(Class<T> c) called with Class<U> where U is a type parameter,
            // we need resolved_parameterized_type = Set<U> for set.iterator().next() to work
            if (inferred_full_type && method -> return_parameterized_type &&
                ! method_call -> resolved_parameterized_type)
            {
                ParameterizedType* ret_ptype = method -> return_parameterized_type;
                // Substitute the method's type parameter with the inferred type
                // The return type's type arguments reference the method's type parameters
                Tuple<Type*>* new_type_args = new Tuple<Type*>(ret_ptype -> NumTypeArguments());
                for (unsigned j = 0; j < ret_ptype -> NumTypeArguments(); j++)
                {
                    Type* type_arg = ret_ptype -> TypeArgument(j);
                    if (type_arg && type_arg -> IsTypeParameter())
                    {
                        TypeParameterSymbol* tparam = type_arg -> GetTypeParameter();
                        // Check if this type parameter belongs to the method
                        bool is_method_type_param = false;
                        for (unsigned k = 0; k < method -> NumTypeParameters(); k++)
                        {
                            if (method -> TypeParameter(k) == tparam)
                            {
                                is_method_type_param = true;
                                // If return type param index matches, use inferred_full_type
                                if ((int)k == return_type_param)
                                {
                                    new_type_args -> Next() = inferred_full_type;
                                }
                                else
                                {
                                    // Different method type param - keep as-is or use erasure
                                    new_type_args -> Next() = new Type(inferred_type);
                                }
                                break;
                            }
                        }
                        if (!is_method_type_param)
                        {
                            // Not a method type param (class type param?) - keep as-is
                            new_type_args -> Next() = type_arg;
                        }
                    }
                    else
                    {
                        new_type_args -> Next() = type_arg ? type_arg : new Type(control.Object());
                    }
                }
                ParameterizedType* substituted = new ParameterizedType(ret_ptype -> generic_type, new_type_args);
                method_call -> resolved_parameterized_type = substituted;
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

                // Unwrap cast expressions to get the original expression's type
                // This is needed because method argument type coercion wraps the expression
                AstExpression* original_arg = arg;
                while (original_arg -> kind == Ast::CAST)
                {
                    AstCastExpression* cast = (AstCastExpression*) original_arg;
                    original_arg = cast -> expression;
                }
                // Use the original expression's type for inference, not the cast target type
                TypeSymbol* original_type = original_arg -> Type();
                if (original_type && original_type != arg_type)
                {
                    arg_type = original_type;
                }

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
            else
            {
                // No inference from arguments succeeded - mark for target type inference
                // The return type will be inferred from the assignment context if available
                method_call -> needs_target_type_inference = true;
            }
        }
    }

    //
    // Check for generic methods (methods with type parameters) whose return type
    // is itself generic. These may need target type inference if we haven't already
    // resolved the concrete return type.
    // Examples: <T> Container<T> of(Class<T> clazz), <T> List<T> emptyList()
    // Skip if we already resolved the type from arguments - that takes precedence.
    // Also skip if we have param_type_param_indices - the return_parameterized_type
    // processing can infer from arguments.
    //
    if (method && method -> NumTypeParameters() > 0 &&
        ! method_call -> resolved_type &&
        ! method_call -> resolved_parameterized_type &&
        ! method_call -> needs_target_type_inference &&
        ! method -> param_type_param_indices)
    {
        TypeSymbol* return_type = method -> Type();
        // If the return type is a generic class (has type parameters), mark for inference
        if (return_type && return_type -> NumTypeParameters() > 0)
        {
            method_call -> needs_target_type_inference = true;
        }
    }

    //
    // Direct parameterized return type: If the method's return type is directly
    // parameterized (e.g., Map<String, Integer> getMap()), propagate the parameterized
    // type to the method call for chained calls like getMap().get(key).
    // Skip if needs_target_type_inference is set - those need special handling.
    //
    // When the return type contains type parameter references (e.g., Set<Map.Entry<K,V>>),
    // substitute them with the actual type arguments from the receiver expression.
    //
    if (method && method -> return_parameterized_type &&
        ! method_call -> resolved_parameterized_type &&
        ! method_call -> needs_target_type_inference)
    {
        ParameterizedType* ret_ptype = method -> return_parameterized_type;
        // Get the receiver's parameterized type for substitution
        ParameterizedType* base_param_type = NULL;
        if (method_call -> base_opt)
        {
            AstExpression* base_expr = method_call -> base_opt -> ExpressionCast();
            if (base_expr)
            {
                VariableSymbol* var = base_expr -> symbol ?
                    base_expr -> symbol -> VariableCast() : NULL;
                if (var && var -> parameterized_type)
                    base_param_type = var -> parameterized_type;
                else if (base_expr -> resolved_parameterized_type)
                    base_param_type = base_expr -> resolved_parameterized_type;
            }
        }

        // Check if substitution is needed: the return type has type arguments that
        // might reference type parameters from the containing class
        bool needs_substitution = false;
        if (base_param_type && ret_ptype -> NumTypeArguments() > 0)
        {
            for (unsigned i = 0; i < ret_ptype -> NumTypeArguments() && !needs_substitution; i++)
            {
                Type* type_arg = ret_ptype -> TypeArgument(i);
                if (type_arg)
                {
                    if (type_arg -> IsTypeParameter())
                        needs_substitution = true;
                    else if (type_arg -> IsParameterized())
                    {
                        // Check nested parameterized type for type params
                        ParameterizedType* nested = type_arg -> GetParameterizedType();
                        if (nested)
                        {
                            for (unsigned j = 0; j < nested -> NumTypeArguments(); j++)
                            {
                                Type* nested_arg = nested -> TypeArgument(j);
                                if (nested_arg && nested_arg -> IsTypeParameter())
                                {
                                    needs_substitution = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (needs_substitution)
        {
            // Perform substitution: create new type arguments with substituted types
            TypeSymbol* containing = method -> containing_type;

            // First, find the correct type argument mapping from base_param_type to containing type
            // The receiver may be a subtype of the containing type (e.g., Set<T> calling Collection.iterator())
            // We need to walk up the hierarchy to find the type arguments for the containing type
            ParameterizedType* containing_param_type = NULL;
            if (base_param_type -> generic_type == containing)
            {
                containing_param_type = base_param_type;
            }
            else
            {
                // BFS to find the containing type in the interface hierarchy
                TypeSymbol* receiver_type = base_param_type -> generic_type;
                if (receiver_type)
                {
                    const int MAX_DEPTH = 10;
                    struct SearchNode { TypeSymbol* type; ParameterizedType* param_type; };
                    SearchNode queue[MAX_DEPTH * 4];
                    int queue_start = 0, queue_end = 0;

                    // Start with receiver's interfaces
                    for (unsigned i = 0; i < receiver_type -> NumInterfaces() && queue_end < MAX_DEPTH * 4; i++)
                    {
                        ParameterizedType* iface_param = receiver_type -> ParameterizedInterface(i);
                        if (iface_param)
                        {
                            Tuple<Type*>* subst_args = new Tuple<Type*>(iface_param -> NumTypeArguments());
                            for (unsigned j = 0; j < iface_param -> NumTypeArguments(); j++)
                            {
                                Type* type_arg = iface_param -> TypeArgument(j);
                                Type* subst_arg = type_arg;
                                if (type_arg && type_arg -> IsTypeParameter())
                                {
                                    TypeParameterSymbol* tp = type_arg -> GetTypeParameter();
                                    for (unsigned k = 0; k < receiver_type -> NumTypeParameters(); k++)
                                    {
                                        if (receiver_type -> TypeParameter(k) == tp &&
                                            k < base_param_type -> NumTypeArguments())
                                        {
                                            subst_arg = base_param_type -> TypeArgument(k);
                                            break;
                                        }
                                    }
                                }
                                subst_args -> Next() = subst_arg;
                            }
                            ParameterizedType* subst_iface = new ParameterizedType(iface_param -> generic_type, subst_args);
                            queue[queue_end].type = iface_param -> generic_type;
                            queue[queue_end].param_type = subst_iface;
                            queue_end++;
                        }
                    }

                    while (queue_start < queue_end && ! containing_param_type)
                    {
                        TypeSymbol* cur_type = queue[queue_start].type;
                        ParameterizedType* cur_param = queue[queue_start].param_type;
                        queue_start++;

                        if (cur_type == containing)
                        {
                            containing_param_type = cur_param;
                            break;
                        }

                        for (unsigned i = 0; i < cur_type -> NumInterfaces() && queue_end < MAX_DEPTH * 4; i++)
                        {
                            ParameterizedType* super_iface = cur_type -> ParameterizedInterface(i);
                            if (super_iface)
                            {
                                Tuple<Type*>* new_args = new Tuple<Type*>(super_iface -> NumTypeArguments());
                                for (unsigned j = 0; j < super_iface -> NumTypeArguments(); j++)
                                {
                                    Type* type_arg = super_iface -> TypeArgument(j);
                                    Type* subst_arg = type_arg;
                                    if (type_arg && type_arg -> IsTypeParameter())
                                    {
                                        TypeParameterSymbol* tp = type_arg -> GetTypeParameter();
                                        for (unsigned k = 0; k < cur_type -> NumTypeParameters(); k++)
                                        {
                                            if (cur_type -> TypeParameter(k) == tp &&
                                                cur_param && k < cur_param -> NumTypeArguments())
                                            {
                                                subst_arg = cur_param -> TypeArgument(k);
                                                break;
                                            }
                                        }
                                    }
                                    new_args -> Next() = subst_arg;
                                }
                                ParameterizedType* new_param = new ParameterizedType(super_iface -> generic_type, new_args);
                                queue[queue_end].type = super_iface -> generic_type;
                                queue[queue_end].param_type = new_param;
                                queue_end++;
                            }
                        }
                    }
                }
            }

            // Use containing_param_type for substitution (fallback to base_param_type)
            ParameterizedType* subst_source = containing_param_type ? containing_param_type : base_param_type;
            TypeSymbol* subst_type = subst_source -> generic_type;

            Tuple<Type*>* new_type_args = new Tuple<Type*>(ret_ptype -> NumTypeArguments());

            for (unsigned i = 0; i < ret_ptype -> NumTypeArguments(); i++)
            {
                Type* type_arg = ret_ptype -> TypeArgument(i);
                Type* new_arg = NULL;

                if (type_arg -> IsTypeParameter())
                {
                    // Simple type parameter - substitute directly
                    TypeParameterSymbol* tparam = type_arg -> GetTypeParameter();
                    for (unsigned k = 0; subst_type && k < subst_type -> NumTypeParameters(); k++)
                    {
                        if (subst_type -> TypeParameter(k) == tparam &&
                            k < subst_source -> NumTypeArguments())
                        {
                            // Use the resolved type argument
                            new_arg = subst_source -> TypeArgument(k);
                            break;
                        }
                    }
                    if (!new_arg)
                        new_arg = new Type(control.Object());
                }
                else if (type_arg -> IsParameterized())
                {
                    // Nested parameterized type (e.g., Map.Entry<K,V>) - substitute its type args
                    ParameterizedType* nested = type_arg -> GetParameterizedType();
                    bool has_type_params = false;
                    for (unsigned j = 0; j < nested -> NumTypeArguments(); j++)
                    {
                        Type* nested_arg = nested -> TypeArgument(j);
                        if (nested_arg && nested_arg -> IsTypeParameter())
                        {
                            has_type_params = true;
                            break;
                        }
                    }

                    if (has_type_params)
                    {
                        // Create substituted nested parameterized type
                        Tuple<Type*>* nested_args = new Tuple<Type*>(nested -> NumTypeArguments());
                        for (unsigned j = 0; j < nested -> NumTypeArguments(); j++)
                        {
                            Type* nested_arg = nested -> TypeArgument(j);
                            Type* subst_arg = NULL;

                            if (nested_arg && nested_arg -> IsTypeParameter())
                            {
                                TypeParameterSymbol* tparam = nested_arg -> GetTypeParameter();
                                for (unsigned k = 0; subst_type && k < subst_type -> NumTypeParameters(); k++)
                                {
                                    if (subst_type -> TypeParameter(k) == tparam &&
                                        k < subst_source -> NumTypeArguments())
                                    {
                                        subst_arg = subst_source -> TypeArgument(k);
                                        break;
                                    }
                                }
                            }
                            if (!subst_arg)
                                subst_arg = nested_arg ? nested_arg : new Type(control.Object());
                            nested_args -> Next() = subst_arg;
                        }
                        ParameterizedType* new_nested = new ParameterizedType(nested -> generic_type, nested_args);
                        new_arg = new Type(new_nested);
                    }
                    else
                    {
                        new_arg = type_arg;
                    }
                }
                else
                {
                    new_arg = type_arg;
                }

                new_type_args -> Next() = new_arg;
            }

            ParameterizedType* substituted = new ParameterizedType(ret_ptype -> generic_type, new_type_args);
            method_call -> resolved_parameterized_type = substituted;
        }
        else
        {
            // Check if the return type references method type parameters
            // that need to be inferred from arguments (for generic methods)
            bool has_method_type_params = false;

            if (method -> NumTypeParameters() > 0 && ret_ptype -> NumTypeArguments() > 0)
            {
                for (unsigned i = 0; i < ret_ptype -> NumTypeArguments() && !has_method_type_params; i++)
                {
                    Type* type_arg = ret_ptype -> TypeArgument(i);
                    if (type_arg && type_arg -> IsTypeParameter())
                    {
                        TypeParameterSymbol* tparam = type_arg -> GetTypeParameter();
                        for (unsigned k = 0; k < method -> NumTypeParameters(); k++)
                        {
                            if (method -> TypeParameter(k) == tparam)
                            {
                                has_method_type_params = true;
                                break;
                            }
                        }
                    }
                }
            }

            if (has_method_type_params && method -> param_type_param_indices)
            {
                // Infer method type parameters from arguments
                Tuple<Type*>* inferred_types = new Tuple<Type*>(method -> NumTypeParameters());
                for (unsigned tp = 0; tp < method -> NumTypeParameters(); tp++)
                    inferred_types -> Next() = NULL;

                unsigned num_args = method_call -> arguments -> NumArguments();
                unsigned num_params = method -> param_type_param_indices -> Length();
                unsigned limit = (num_args < num_params) ? num_args : num_params;

                for (unsigned i = 0; i < limit; i++)
                {
                    int param_type_param = (*(method -> param_type_param_indices))[i];
                    if (param_type_param >= 0 && (unsigned)param_type_param < method -> NumTypeParameters())
                    {
                        AstExpression* arg = method_call -> arguments -> Argument(i);

                        // Get parameterized type from argument
                        ParameterizedType* arg_param_type = NULL;
                        if (arg -> symbol)
                        {
                            VariableSymbol* var = arg -> symbol -> VariableCast();
                            if (var && var -> parameterized_type)
                                arg_param_type = var -> parameterized_type;
                        }
                        if (! arg_param_type && arg -> resolved_parameterized_type)
                            arg_param_type = arg -> resolved_parameterized_type;

                        if (arg_param_type && arg_param_type -> NumTypeArguments() > 0)
                        {
                            Type* first_type_arg = arg_param_type -> TypeArgument(0);
                            if (first_type_arg && ! first_type_arg -> IsWildcard())
                            {
                                (*inferred_types)[param_type_param] = first_type_arg;
                            }
                        }
                    }
                }

                // Substitute method type parameters in return type
                Tuple<Type*>* new_type_args = new Tuple<Type*>(ret_ptype -> NumTypeArguments());
                for (unsigned j = 0; j < ret_ptype -> NumTypeArguments(); j++)
                {
                    Type* type_arg = ret_ptype -> TypeArgument(j);
                    Type* new_arg = NULL;

                    if (type_arg && type_arg -> IsTypeParameter())
                    {
                        TypeParameterSymbol* tparam = type_arg -> GetTypeParameter();
                        for (unsigned k = 0; k < method -> NumTypeParameters(); k++)
                        {
                            if (method -> TypeParameter(k) == tparam)
                            {
                                new_arg = (*inferred_types)[k];
                                break;
                            }
                        }
                    }
                    if (!new_arg)
                        new_arg = type_arg ? type_arg : new Type(control.Object());
                    new_type_args -> Next() = new_arg;
                }

                ParameterizedType* substituted = new ParameterizedType(ret_ptype -> generic_type, new_type_args);
                method_call -> resolved_parameterized_type = substituted;
                delete inferred_types;
            }
            else
            {
                method_call -> resolved_parameterized_type = ret_ptype;
            }
        }
    }

    //
    // Fallback: If method returns a generic type but return_parameterized_type is NULL
    // (e.g., when loaded from class file), construct it from the return type and
    // the receiver's parameterized type arguments.
    // Example: set.iterator() where set is Set<T> and iterator() returns Iterator<E>
    // (with E being Set's type param) - construct Iterator<T>
    //
    if (method && ! method_call -> resolved_parameterized_type && base)
    {
        TypeSymbol* return_type = method -> Type();
        if (return_type && return_type -> NumTypeParameters() > 0 &&
            method -> containing_type && method -> containing_type -> NumTypeParameters() > 0)
        {
            // Get receiver's parameterized type
            ParameterizedType* base_param_type = NULL;
            VariableSymbol* var = base -> symbol ? base -> symbol -> VariableCast() : NULL;
            if (var && var -> parameterized_type)
                base_param_type = var -> parameterized_type;
            else if (base -> ExpressionCast() && base -> ExpressionCast() -> resolved_parameterized_type)
                base_param_type = base -> ExpressionCast() -> resolved_parameterized_type;

            // If receiver has parameterized type, find type arguments for method's containing type
            // The method may be declared in a supertype (e.g., iterator() in Iterable, called on Set)
            if (base_param_type)
            {
                TypeSymbol* containing = method -> containing_type;
                ParameterizedType* containing_param_type = NULL;

                // Check if the receiver's type directly matches the containing type
                if (base_param_type -> generic_type == containing)
                {
                    containing_param_type = base_param_type;
                }
                else
                {
                    // Walk up the inheritance hierarchy to find how the receiver implements
                    // the method's containing type. E.g., Set<T> -> Collection<T> -> Iterable<T>
                    // We need to recursively search through interfaces since the containing type
                    // may be a super-interface (e.g., Iterable is a super-interface of Collection)

                    // Use a simple BFS-like approach to search the interface hierarchy
                    // We keep track of the current type arguments at each level
                    TypeSymbol* receiver_type = base_param_type -> generic_type;
                    if (receiver_type)
                    {
                        // Stack of (type, parameterized_type_or_args) to process
                        // We use a simple linear search with limited depth
                        const int MAX_DEPTH = 10;
                        struct SearchNode {
                            TypeSymbol* type;
                            ParameterizedType* param_type;  // Current substituted type args
                        };
                        SearchNode queue[MAX_DEPTH * 4];
                        int queue_start = 0;
                        int queue_end = 0;

                        // Start with the receiver's interfaces
                        for (unsigned i = 0; i < receiver_type -> NumInterfaces() && queue_end < MAX_DEPTH * 4; i++)
                        {
                            ParameterizedType* iface_param = receiver_type -> ParameterizedInterface(i);
                            if (iface_param)
                            {
                                // Substitute type arguments using receiver's type args
                                Tuple<Type*>* subst_args = new Tuple<Type*>(iface_param -> NumTypeArguments());
                                for (unsigned j = 0; j < iface_param -> NumTypeArguments(); j++)
                                {
                                    Type* type_arg = iface_param -> TypeArgument(j);
                                    Type* subst_arg = type_arg;
                                    if (type_arg && type_arg -> IsTypeParameter())
                                    {
                                        TypeParameterSymbol* tparam = type_arg -> GetTypeParameter();
                                        for (unsigned k = 0; k < receiver_type -> NumTypeParameters(); k++)
                                        {
                                            if (receiver_type -> TypeParameter(k) == tparam &&
                                                k < base_param_type -> NumTypeArguments())
                                            {
                                                subst_arg = base_param_type -> TypeArgument(k);
                                                break;
                                            }
                                        }
                                    }
                                    subst_args -> Next() = subst_arg;
                                }
                                ParameterizedType* subst_iface = new ParameterizedType(iface_param -> generic_type, subst_args);
                                queue[queue_end].type = iface_param -> generic_type;
                                queue[queue_end].param_type = subst_iface;
                                queue_end++;
                            }
                        }

                        // Process the queue
                        while (queue_start < queue_end && ! containing_param_type)
                        {
                            TypeSymbol* cur_type = queue[queue_start].type;
                            ParameterizedType* cur_param = queue[queue_start].param_type;
                            queue_start++;

                            // Check if this is the containing type
                            if (cur_type == containing)
                            {
                                containing_param_type = cur_param;
                                break;
                            }

                            // Add this type's interfaces to the queue
                            for (unsigned i = 0; i < cur_type -> NumInterfaces() && queue_end < MAX_DEPTH * 4; i++)
                            {
                                ParameterizedType* super_iface = cur_type -> ParameterizedInterface(i);
                                if (super_iface)
                                {
                                    // Substitute type arguments using current type's args
                                    Tuple<Type*>* new_args = new Tuple<Type*>(super_iface -> NumTypeArguments());
                                    for (unsigned j = 0; j < super_iface -> NumTypeArguments(); j++)
                                    {
                                        Type* type_arg = super_iface -> TypeArgument(j);
                                        Type* subst_arg = type_arg;
                                        if (type_arg && type_arg -> IsTypeParameter())
                                        {
                                            TypeParameterSymbol* tparam = type_arg -> GetTypeParameter();
                                            for (unsigned k = 0; k < cur_type -> NumTypeParameters(); k++)
                                            {
                                                if (cur_type -> TypeParameter(k) == tparam &&
                                                    cur_param && k < cur_param -> NumTypeArguments())
                                                {
                                                    subst_arg = cur_param -> TypeArgument(k);
                                                    break;
                                                }
                                            }
                                        }
                                        new_args -> Next() = subst_arg;
                                    }
                                    ParameterizedType* new_param = new ParameterizedType(super_iface -> generic_type, new_args);
                                    queue[queue_end].type = super_iface -> generic_type;
                                    queue[queue_end].param_type = new_param;
                                    queue_end++;
                                }
                            }
                        }
                    }
                }

                // Construct parameterized return type using the resolved containing type's arguments
                if (containing_param_type)
                {
                    unsigned num_return_params = return_type -> NumTypeParameters();
                    unsigned num_containing_args = containing_param_type -> NumTypeArguments();
                    if (num_return_params <= num_containing_args)
                    {
                        Tuple<Type*>* new_type_args = new Tuple<Type*>(num_return_params);
                        for (unsigned i = 0; i < num_return_params; i++)
                        {
                            new_type_args -> Next() = containing_param_type -> TypeArgument(i);
                        }
                        ParameterizedType* result = new ParameterizedType(return_type, new_type_args);
                        method_call -> resolved_parameterized_type = result;
                    }
                }
            }
        }
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
    else
    {
        class_lit -> symbol = control.Class();
        //
        // For generics support: Provider.class has type Class<Provider>.
        // Create a parameterized type for the class literal so that
        // method type parameter inference works correctly.
        // E.g., for ServiceLoader.load(Class<S> service), passing Provider.class
        // should infer S = Provider.
        //
        Tuple<Type*>* type_args = new Tuple<Type*>(1);
        type_args -> Next() = new Type(type);  // The type argument is the class type
        class_lit -> resolved_parameterized_type = new ParameterizedType(control.Class(), type_args);
    }
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
    //
    // Next, simply pass all parameters through to the superclass.
    // For varargs constructors, num_args may be less than num_formals.
    //
    bool is_varargs = super_constructor -> ACC_VARARGS();
    unsigned num_formals = super_constructor -> NumFormalParameters();

    // For varargs, we pass actual arguments through; the varargs handling
    // (packing into array) is done by MethodInvocationConversion later.
    // The super call will get the actual arguments, and for empty varargs,
    // an empty array will be created during conversion.
    super_args -> AllocateArguments(is_varargs ? num_args : num_formals);

    // For varargs, get the component type of the varargs array
    TypeSymbol* varargs_component_type = NULL;
    if (is_varargs && num_formals > 0)
    {
        TypeSymbol* varargs_type = super_constructor -> FormalParameter(num_formals - 1) -> Type();
        if (varargs_type -> IsArray())
            varargs_component_type = varargs_type -> ArraySubtype();
    }

    for (unsigned j = 0; j < num_args; j++)
    {
        // For varargs, use actual argument type; for non-varargs, use formal parameter type
        TypeSymbol* param_type;
        if (is_varargs && j >= num_formals - 1)
        {
            // This is a varargs argument
            TypeSymbol* arg_type = class_creation -> arguments -> Argument(j) -> Type();
            // If the argument is null or doesn't have a signature, use the varargs component type
            if (arg_type == control.null_type || !arg_type -> signature)
            {
                param_type = varargs_component_type ? varargs_component_type : control.Object();
            }
            else
            {
                param_type = arg_type;
            }
        }
        else
        {
            VariableSymbol* param = super_constructor -> FormalParameter(j);
            param_type = param -> Type();
        }

        VariableSymbol* symbol =
            block_symbol -> InsertVariableSymbol(control.MakeParameter(j));
        symbol -> SetType(param_type);
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
