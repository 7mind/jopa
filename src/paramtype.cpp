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


void ParameterizedType::GenerateSignature(char* buffer, unsigned& length)
{
    // Format: L<classname><TypeArguments>;
    // Example: Ljava/util/List<Ljava/lang/String;>;

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
                Type enclosing_type_wrapper(parameterized_type -> enclosing_type, false);
                Type* cloned_wrapper = enclosing_type_wrapper.Clone();
                cloned_enclosing = cloned_wrapper -> GetParameterizedType();
                // cloned_wrapper owns the cloned parameterized type, but GetParameterizedType returns ptr
                // We need to extract it and delete wrapper?
                // Actually Clone() returns a new Type which owns a new ParameterizedType.
                // We need the ParameterizedType* from it, then we can delete the Type wrapper but 
                // we must prevent it from deleting the ParameterizedType.
                cloned_wrapper -> owns_content = false; 
                delete cloned_wrapper;
            }

            ParameterizedType* cloned_pt = new ParameterizedType(
                parameterized_type -> generic_type,
                cloned_args,
                cloned_enclosing
            );

            return new Type(cloned_pt, true);
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

            return new Type(cloned_wt, true);
        }

        case ARRAY_TYPE:
        {
            Type* cloned_component = array_type -> component_type -> Clone();
            ArrayType* cloned_at = new ArrayType(cloned_component);
            return new Type(cloned_at, true);
        }

        default:
            return NULL;
    }
}

bool Type::IsSubtype(Type* other)
{
    if (this == other) return true;
    if (!other) return false;

    // Fast path: Erasure check
    TypeSymbol* this_erased = Erasure();
    TypeSymbol* other_erased = other->Erasure();

    if (!this_erased || !other_erased) return false;

    // Check erased subtype relationship first
    if (!this_erased->IsSubtype(other_erased)) return false;

    // If either is raw, we allow the conversion (unchecked)
    if (this->IsRawType() || other->IsRawType())
        return true;

    // If both are parameterized with same generic type
    if (kind == PARAMETERIZED_TYPE && other->kind == PARAMETERIZED_TYPE)
    {
        if (parameterized_type->generic_type == other->parameterized_type->generic_type)
        {
            // Check arguments for identity (invariant)
            // Full containment check requires more logic
            // For now: just check strict equality of args to be safe
            unsigned n = parameterized_type->type_arguments ? parameterized_type->type_arguments->Length() : 0;
            unsigned m = other->parameterized_type->type_arguments ? other->parameterized_type->type_arguments->Length() : 0;
            if (n != m) return false;

            for (unsigned i = 0; i < n; i++)
            {
                Type* t1 = (*parameterized_type->type_arguments)[i];
                Type* t2 = (*other->parameterized_type->type_arguments)[i];
                // Recursive equality check? Or just erasure for now?
                // Use erasure equality for loose check
                if (t1->Erasure() != t2->Erasure()) return false;
            }
            return true;
        }
    }

    return true; // Default to trusting erasure for now
}


} // Close namespace Jopa block
