#include "paramtype.h"
#include "typeparam.h"
#include "symbol.h"
#include "control.h"
#include <cstring>

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

//
// ParameterizedType implementation
//

void ParameterizedType::GenerateSignature(char* buffer, unsigned& length)
{
    // Format: L<classname><TypeArguments>;
    // Example: Ljava/util/List<Ljava/lang/String;>;

    buffer[length++] = 'L';

    // Add the class name with slashes
    const char* class_name = generic_type -> Utf8Name();
    unsigned class_name_len = generic_type -> Utf8NameLength();

    for (unsigned i = 0; i < class_name_len; i++)
    {
        char c = class_name[i];
        buffer[length++] = (c == '.') ? '/' : c;
    }

    // Add type arguments if present
    if (type_arguments && type_arguments -> Length() > 0)
    {
        buffer[length++] = '<';

        for (unsigned i = 0; i < type_arguments -> Length(); i++)
        {
            Type* arg = (*type_arguments)[i];
            arg -> GenerateSignature(buffer, length);
        }

        buffer[length++] = '>';
    }

    buffer[length++] = ';';
}

//
// WildcardType implementation
//

TypeSymbol* WildcardType::UpperBound()
{
    if (bound_kind == EXTENDS && bound)
    {
        return bound -> Erasure();
    }

    // For UNBOUNDED and SUPER, upper bound is Object
    // This will be set properly during semantic analysis
    return NULL;  // Placeholder - will be replaced with Object
}

TypeSymbol* WildcardType::LowerBound()
{
    if (bound_kind == SUPER && bound)
    {
        return bound -> Erasure();
    }

    // No lower bound for EXTENDS and UNBOUNDED
    return NULL;
}

void WildcardType::GenerateSignature(char* buffer, unsigned& length)
{
    // Format:
    //   ? -> *
    //   ? extends Foo -> +<TypeSignature>
    //   ? super Bar -> -<TypeSignature>

    switch (bound_kind)
    {
        case UNBOUNDED:
            buffer[length++] = '*';
            break;

        case EXTENDS:
            buffer[length++] = '+';
            if (bound)
                bound -> GenerateSignature(buffer, length);
            break;

        case SUPER:
            buffer[length++] = '-';
            if (bound)
                bound -> GenerateSignature(buffer, length);
            break;
    }
}

//
// ArrayType implementation
//

TypeSymbol* ArrayType::Erasure()
{
    // Array erasure depends on component type erasure
    // T[] where T erases to Object -> Object[]
    // T[] where T extends Number -> Number[]
    // List<String>[] -> List[]

    // This will be implemented properly when we have access to Control
    // and can create array types
    return NULL;  // Placeholder
}

void ArrayType::GenerateSignature(char* buffer, unsigned& length)
{
    // Array signature: [<ComponentTypeSignature>
    buffer[length++] = '[';
    component_type -> GenerateSignature(buffer, length);
}

//
// Type implementation
//

TypeSymbol* Type::Erasure()
{
    switch (kind)
    {
        case SIMPLE_TYPE:
            return simple_type;

        case PARAMETERIZED_TYPE:
            return parameterized_type -> Erasure();

        case TYPE_PARAMETER:
            return type_parameter -> ErasedType();

        case WILDCARD_TYPE:
            return wildcard_type -> UpperBound();

        case ARRAY_TYPE:
            return array_type -> Erasure();

        default:
            return NULL;
    }
}

bool Type::IsGeneric()
{
    switch (kind)
    {
        case SIMPLE_TYPE:
            // Check if the simple type is itself a generic type (has type parameters)
            return simple_type && simple_type -> NumTypeParameters() > 0;

        case PARAMETERIZED_TYPE:
            return true;

        case TYPE_PARAMETER:
            return true;

        case WILDCARD_TYPE:
            return true;

        case ARRAY_TYPE:
            return array_type -> component_type -> IsGeneric();

        default:
            return false;
    }
}

bool Type::IsRawType()
{
    if (kind == SIMPLE_TYPE && simple_type)
    {
        // A raw type is a generic type used without type arguments
        return simple_type -> IsGeneric();
    }

    return false;
}

void Type::GenerateSignature(char* buffer, unsigned& length)
{
    switch (kind)
    {
        case SIMPLE_TYPE:
        {
            // Simple type signature: L<classname>;
            // Or primitive: I, J, Z, etc.

            if (simple_type -> Primitive())
            {
                // Primitive type - single character
                // This is simplified - needs proper primitive signature mapping
                const char* name = simple_type -> Utf8Name();
                if (strcmp(name, "int") == 0)
                    buffer[length++] = 'I';
                else if (strcmp(name, "long") == 0)
                    buffer[length++] = 'J';
                else if (strcmp(name, "boolean") == 0)
                    buffer[length++] = 'Z';
                else if (strcmp(name, "byte") == 0)
                    buffer[length++] = 'B';
                else if (strcmp(name, "char") == 0)
                    buffer[length++] = 'C';
                else if (strcmp(name, "short") == 0)
                    buffer[length++] = 'S';
                else if (strcmp(name, "float") == 0)
                    buffer[length++] = 'F';
                else if (strcmp(name, "double") == 0)
                    buffer[length++] = 'D';
                else if (strcmp(name, "void") == 0)
                    buffer[length++] = 'V';
            }
            else
            {
                // Reference type
                buffer[length++] = 'L';

                const char* class_name = simple_type -> Utf8Name();
                unsigned class_name_len = simple_type -> Utf8NameLength();

                for (unsigned i = 0; i < class_name_len; i++)
                {
                    char c = class_name[i];
                    buffer[length++] = (c == '.') ? '/' : c;
                }

                buffer[length++] = ';';
            }
            break;
        }

        case PARAMETERIZED_TYPE:
            parameterized_type -> GenerateSignature(buffer, length);
            break;

        case TYPE_PARAMETER:
        {
            // Type variable reference: T<Identifier>;
            buffer[length++] = 'T';

            const char* name = type_parameter -> Utf8Name();
            unsigned name_len = type_parameter -> Utf8NameLength();

            for (unsigned i = 0; i < name_len; i++)
                buffer[length++] = name[i];

            buffer[length++] = ';';
            break;
        }

        case WILDCARD_TYPE:
            wildcard_type -> GenerateSignature(buffer, length);
            break;

        case ARRAY_TYPE:
            array_type -> GenerateSignature(buffer, length);
            break;
    }
}

Type* Type::Clone()
{
    switch (kind)
    {
        case SIMPLE_TYPE:
            return new Type(simple_type);

        case PARAMETERIZED_TYPE:
        {
            // Clone type arguments
            Tuple<Type*>* cloned_args = NULL;
            if (parameterized_type -> type_arguments)
            {
                cloned_args = new Tuple<Type*>(
                    parameterized_type -> type_arguments -> Length());
                for (unsigned i = 0; i < parameterized_type -> type_arguments -> Length(); i++)
                {
                    cloned_args -> Next() =
                        (*parameterized_type -> type_arguments)[i] -> Clone();
                }
            }

            ParameterizedType* cloned_enclosing = NULL;
            if (parameterized_type -> enclosing_type)
            {
                // Recursively clone enclosing type
                Type enclosing_type_wrapper(parameterized_type -> enclosing_type);
                Type* cloned_wrapper = enclosing_type_wrapper.Clone();
                cloned_enclosing = cloned_wrapper -> GetParameterizedType();
            }

            ParameterizedType* cloned_pt = new ParameterizedType(
                parameterized_type -> generic_type,
                cloned_args,
                cloned_enclosing
            );

            return new Type(cloned_pt);
        }

        case TYPE_PARAMETER:
            return new Type(type_parameter);

        case WILDCARD_TYPE:
        {
            Type* cloned_bound = wildcard_type -> bound
                ? wildcard_type -> bound -> Clone()
                : NULL;

            WildcardType* cloned_wt = new WildcardType(
                wildcard_type -> bound_kind,
                cloned_bound
            );

            return new Type(cloned_wt);
        }

        case ARRAY_TYPE:
        {
            Type* cloned_component = array_type -> component_type -> Clone();
            ArrayType* cloned_at = new ArrayType(cloned_component);
            return new Type(cloned_at);
        }

        default:
            return NULL;
    }
}

#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif
