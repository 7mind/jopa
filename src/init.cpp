#include "platform.h"
#include "semantic.h"
#include "control.h"
#include "stream.h"


namespace Jopa { // Open namespace Jopa block
void Semantic::ProcessVariableInitializer(AstVariableDeclarator* variable_declarator)
{
    VariableSymbol* symbol = variable_declarator -> symbol;

    if (! variable_declarator -> variable_initializer_opt)
    {
        symbol -> MarkInitialized();
        return;
    }

    TypeSymbol* field_type = symbol -> Type();
    AstExpression* init =
        (AstExpression*) variable_declarator -> variable_initializer_opt;
    AstArrayInitializer* array_initializer = init -> ArrayInitializerCast();
    if (array_initializer)
        ProcessArrayInitializer(array_initializer, field_type);
    else
    {
        ProcessExpressionOrStringConstant(init);

        // Target type inference: If the initializer is a method call with an
        // uninferred type parameter return type, use the target type (field_type).
        AstMethodInvocation* method_call = init -> MethodInvocationCast();
        if (method_call && method_call -> needs_target_type_inference &&
            field_type && field_type != control.no_type && field_type != control.Object())
        {
            // Get the method to verify it's a generic method
            MethodSymbol* method = method_call -> symbol ? method_call -> symbol -> MethodCast() : NULL;
            if (method && method -> method_return_type_param_index >= 0)
            {
                // The method's return type is a type parameter - use target type
                // Handle array return types
                TypeSymbol* method_return_type = method -> Type();
                unsigned return_dims = method_return_type ? method_return_type -> num_dimensions : 0;
                if (return_dims > 0)
                {
                    // Return type is T[] - target must be an array with matching dimensions
                    if (field_type -> num_dimensions >= return_dims)
                    {
                        method_call -> resolved_type = field_type;
                        method_call -> needs_target_type_inference = false;
                    }
                }
                else
                {
                    // Simple return type T - use target type directly
                    method_call -> resolved_type = field_type;
                    method_call -> needs_target_type_inference = false;
                }
            }
        }

        if (field_type != init -> Type() && init -> Type() != control.no_type)
        {
            if (CanAssignmentConvert(field_type, init))
            {
                init = ConvertToType(init, field_type);
                variable_declarator -> variable_initializer_opt = init;
            }
            else if (init -> IsConstant() &&
                     control.IsSimpleIntegerValueType(init -> Type()) &&
                     control.IsSimpleIntegerValueType(field_type))
            {
                if (field_type == control.byte_type)
                    ReportSemError(SemanticError::INVALID_BYTE_VALUE, init);
                else if (field_type == control.char_type)
                    ReportSemError(SemanticError::INVALID_CHARACTER_VALUE,
                                   init);
                else ReportSemError(SemanticError::INVALID_SHORT_VALUE, init);
                init -> value = NULL;
            }
            else
            {
                TypeSymbol* init_type = init -> Type();
                if (! init_type)
                    init_type = control.no_type;
                ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_ASSIGNMENT,
                               variable_declarator,
                               field_type -> ContainingPackageName(),
                               field_type -> ExternalName(),
                               init_type -> ContainingPackageName(),
                               init_type -> ExternalName());
                init -> value = NULL;
            }
        }

        if (symbol -> ACC_FINAL() &&
            (field_type -> Primitive() || field_type == control.String()))
        {
            if (init -> IsConstant())
            {
                symbol -> initial_value = init -> value;
            }
            else if (symbol -> ACC_STATIC() && ThisType() -> IsInner())
            {
                ReportSemError(SemanticError::STATIC_FIELD_IN_INNER_CLASS_NOT_CONSTANT,
                               variable_declarator,
                               lex_stream -> NameString(variable_declarator -> LeftToken()),
                               ThisType() -> Name(), ThisType() -> FileLoc());
            }
        }
    }

    //
    // A non-static final field initialized to a constant value wastes
    // space in each instance, so warn about it.
    //
    TypeSymbol* containing_type = symbol -> owner -> TypeCast();
    if (containing_type && ! containing_type -> ACC_INTERFACE() &&
        ! field_type -> IsArray() &&
        symbol -> ACC_FINAL() &&
        ! symbol -> ACC_STATIC() &&
        init && init -> IsConstant())
    {
        ReportSemError(SemanticError::NON_STATIC_FINAL_CONSTANT_FIELD,
                       variable_declarator,
                       lex_stream ->
                           NameString(variable_declarator -> LeftToken()));
    }

    symbol -> MarkInitialized();
}


void Semantic::ProcessArrayInitializer(AstArrayInitializer* array_initializer,
                                       TypeSymbol* type)
{
    if (! type -> IsArray())
    {
        ReportSemError(SemanticError::INIT_SCALAR_WITH_ARRAY,
                       array_initializer, type -> Name());
    }
    else
    {
        for (unsigned i = 0;
             i < array_initializer -> NumVariableInitializers(); i++)
        {
            AstArrayInitializer* sub_array_initializer = array_initializer ->
                VariableInitializer(i) -> ArrayInitializerCast();
            TypeSymbol* array_subtype = type -> ArraySubtype();
            if (sub_array_initializer)
                 ProcessArrayInitializer(sub_array_initializer, array_subtype);
            else
            {
                AstExpression* init = (AstExpression*) array_initializer ->
                    VariableInitializer(i);
                ProcessExpressionOrStringConstant(init);

                if (array_subtype != init -> Type())
                {
                    if (CanAssignmentConvert(array_subtype, init))
                        array_initializer -> VariableInitializer(i) =
                            ConvertToType(init, array_subtype);
                    else if (array_subtype -> IsArray() &&
                             init -> Type() -> Primitive())
                    {
                        ReportSemError(SemanticError::INIT_ARRAY_WITH_SCALAR,
                                       init, array_subtype -> Name());
                    }
                    else if (init -> IsConstant() &&
                             control.IsSimpleIntegerValueType(init -> Type()) &&
                             control.IsSimpleIntegerValueType(array_subtype))
                    {
                        if (array_subtype == control.byte_type)
                            ReportSemError(SemanticError::INVALID_BYTE_VALUE,
                                           init);
                        else if (array_subtype == control.char_type)
                            ReportSemError(SemanticError::INVALID_CHARACTER_VALUE,
                                           init);
                        else ReportSemError(SemanticError::INVALID_SHORT_VALUE,
                                            init);
                    }
                    else
                    {
                        ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_INITIALIZATION,
                                       init,
                                       array_subtype -> ContainingPackageName(),
                                       array_subtype -> ExternalName(),
                                       init -> Type() -> ContainingPackageName(),
                                       init -> Type() -> ExternalName());
                    }
                }
            }
        }
    }
}


void Semantic::ComputeFinalValue(VariableSymbol* variable)
{
    AstVariableDeclarator* variable_declarator = variable -> declarator;

    // Enum constants are final but don't have declarators
    if (variable -> ACC_ENUM())
    {
        variable -> MarkInitialized();
        return;
    }

    assert(variable_declarator && variable -> ACC_FINAL());
    if (! variable -> IsInitialized())
    {
        if (variable_declarator -> pending ||
            ! variable_declarator -> variable_initializer_opt)
        {
            //
            // Break loops, and ignore non-initialized fields.
            //
            variable -> MarkInitialized();
            return;
        }

        //
        // Create a clone and process that, to avoid triggering errors now.
        // Later, we will issue the errors for real when processing the field
        // initializer when we get to its source file.
        //
        TypeSymbol* type = variable -> ContainingType();
        Semantic* sem = type -> semantic_environment -> sem;

        if (! sem -> error)
            sem -> error =
                new SemanticError(control, sem -> source_file_symbol);
        sem -> error -> EnteringClone();
        sem -> state_stack.Push(type -> semantic_environment);
        MethodSymbol* calling_method = sem -> ThisMethod();
        VariableSymbol* calling_var = sem -> ThisVariable();
        sem -> ThisMethod() = NULL;
        sem -> ThisVariable() = variable;
        variable_declarator -> pending = true;

        StoragePool pool(variable_declarator -> RightToken() -
                         variable_declarator -> LeftToken());
        AstVariableDeclarator* clone = (AstVariableDeclarator*)
            variable_declarator -> Clone(&pool);
        clone -> symbol = variable;
        sem -> ProcessVariableInitializer(clone);
        assert(variable -> IsInitialized());

        variable_declarator -> pending = false;
        sem -> ThisMethod() = calling_method;
        sem -> ThisVariable() = calling_var;
        sem -> state_stack.Pop();
        sem -> error -> ExitingClone();
    }
}


} // Close namespace Jopa block
