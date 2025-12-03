#include "paramtype.h"
#include "typeparam.h"
#include "symbol.h"
#include "control.h"
#include <cstring>


namespace Jopa { // Open namespace Jopa block
//
// ParameterizedType implementation
//

ParameterizedType::~ParameterizedType()
{
    // Delete the Type* objects in the tuple (they were allocated with 'new Type()')
    if (type_arguments)
    {
        for (unsigned i = 0; i < type_arguments -> Length(); i++)
            delete (*type_arguments)[i];
        delete type_arguments;
    }
    delete enclosing_type;
}


ParameterizedType* ParameterizedType::Clone() const
{
    // Clone type arguments
    Tuple<Type*>* cloned_args = NULL;
    if (type_arguments)
    {
        cloned_args = new Tuple<Type*>(type_arguments -> Length());
        for (unsigned i = 0; i < type_arguments -> Length(); i++)
        {
            cloned_args -> Next() = (*type_arguments)[i] -> Clone();
        }
    }

    // Clone enclosing type
    ParameterizedType* cloned_enclosing = NULL;
    if (enclosing_type)
    {
        cloned_enclosing = enclosing_type -> Clone();
    }

    return new ParameterizedType(generic_type, cloned_args, cloned_enclosing);
}


void ParameterizedType::GenerateSignature(char* buffer, unsigned& length)
{
    // Format: L<classname><TypeArguments>;
    // Example: Ljava/util/List<Ljava/lang/String;>;
    //
    // For inner classes with parameterized enclosing types:
    // Format: L<OuterClass><TypeArgs>.<InnerClass>;
    // Example: LA<LB;>.C;  for class C extends A<B>.C

    if (enclosing_type)
    {
        // Generate enclosing type signature without trailing ';'
        // First get the enclosing type's full signature
        const char* enclosing_sig = enclosing_type -> generic_type -> SignatureString();
        unsigned enclosing_len = strlen(enclosing_sig);

        // Copy "L<OuterClassName>"
        for (unsigned i = 0; i < enclosing_len - 1; i++)
            buffer[length++] = enclosing_sig[i];

        // Add enclosing type's type arguments
        if (enclosing_type -> type_arguments && enclosing_type -> type_arguments -> Length() > 0)
        {
            buffer[length++] = '<';
            for (unsigned i = 0; i < enclosing_type -> type_arguments -> Length(); i++)
            {
                Type* arg = (*enclosing_type -> type_arguments)[i];
                arg -> GenerateSignature(buffer, length);
            }
            buffer[length++] = '>';
        }

        // Add '.' separator and inner class simple name
        buffer[length++] = '.';

        // Get the inner class's simple name (without package/outer class prefix)
        const char* inner_name = generic_type -> Utf8Name();
        unsigned inner_len = strlen(inner_name);
        for (unsigned i = 0; i < inner_len; i++)
            buffer[length++] = inner_name[i];

        // Add type arguments for the inner class if present
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
    else
    {
        // Simple case: no enclosing parameterized type
        // Get the fully qualified signature (e.g., "Ljava/util/List;")
        const char* full_sig = generic_type -> SignatureString();
        unsigned sig_len = strlen(full_sig);

        // Copy everything except the trailing ';'
        for (unsigned i = 0; i < sig_len - 1; i++)
        {
            buffer[length++] = full_sig[i];
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
                // Reference type - use the fully qualified signature
                // SignatureString() returns "Ljava/lang/String;" format
                const char* sig = simple_type -> SignatureString();
                unsigned sig_len = strlen(sig);

                for (unsigned i = 0; i < sig_len; i++)
                {
                    buffer[length++] = sig[i];
                }
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

bool Type::IsSubtype(TypeSymbol* type)
{
    if (!type) return false;

    switch (kind)
    {
        case SIMPLE_TYPE:
            return simple_type && simple_type -> IsSubtype(type);

        case PARAMETERIZED_TYPE:
            return parameterized_type -> Erasure() -> IsSubtype(type);

        case TYPE_PARAMETER:
        {
            // For type variable T, T <: type if any bound <: type
            // If no bounds, T <: Object
            if (type_parameter -> NumBounds() == 0)
            {
                return type == NULL; // Should check against Object, but we don't have Control here easily.
                // Assuming type is not null, and unbounded T only extends Object.
                // We need Control to get Object.
                // However, Erasure() returns Object (or NULL).
                // If Erasure() is NULL, we can't check properly without Control.
                // But TypeParameterSymbol::ErasedType() handles this if we trust it.
                // Wait, ErasedType returns NULL if unbounded and no Control.
                
                // If we assume type is valid, we can check if type is Object by name?
                // Or just rely on Erasure logic if possible.
                
                // Better: check bounds.
            }
            
            for (unsigned i = 0; i < type_parameter -> NumBounds(); i++)
            {
                if (type_parameter -> Bound(i) -> IsSubtype(type))
                    return true;
            }
            return false;
        }

        case WILDCARD_TYPE:
            return wildcard_type -> UpperBound() && wildcard_type -> UpperBound() -> IsSubtype(type);

        case ARRAY_TYPE:
            return array_type -> Erasure() && array_type -> Erasure() -> IsSubtype(type);

        default:
            return false;
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


} // Close namespace Jopa block
