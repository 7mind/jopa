#include "typeparam.h"
#include "symbol.h"
#include "lookup.h"
#include "control.h"
#include "paramtype.h"


namespace Jopa { // Open namespace Jopa block

TypeParameterSymbol::~TypeParameterSymbol()
{
    delete bounds;
    if (parameterized_bounds)
    {
        for (unsigned i = 0; i < parameterized_bounds -> Length(); i++)
            delete (*parameterized_bounds)[i];
        delete parameterized_bounds;
    }
    delete [] signature_string;
}

Type* TypeParameterSymbol::ParameterizedBound(unsigned i) const
{
    if (parameterized_bounds && i < parameterized_bounds -> Length())
        return (*parameterized_bounds)[i];
    return NULL;
}

void TypeParameterSymbol::AddParameterizedBound(Type* bound)
{
    if (! parameterized_bounds)
        parameterized_bounds = new Tuple<Type*>(2);
    parameterized_bounds -> Next() = bound;
}

//
// Get the erased type for this type parameter
//
TypeSymbol* TypeParameterSymbol::ErasedType()
{
    // If there are explicit bounds, erase to the first bound
    if (IsBounded())
        return FirstBound();

    // If there's an upper bound from capture conversion, use that
    if (upper_bound)
        return upper_bound;

    // Otherwise, erase to Object
    // This will be filled in during semantic analysis when Control is available
    return NULL;  // Will be replaced with Object during processing
}

//
// Get the name as a wide string
//
const wchar_t* TypeParameterSymbol::Name() const
{
    return name_symbol -> Name();
}

//
// Get the name length
//
unsigned TypeParameterSymbol::NameLength() const
{
    return name_symbol -> NameLength();
}

//
// Get the UTF-8 name
//
const char* TypeParameterSymbol::Utf8Name() const
{
    return name_symbol -> Utf8_literal
        ? name_symbol -> Utf8_literal -> value
        : (char*) NULL;
}

//
// Get the UTF-8 name length
//
unsigned TypeParameterSymbol::Utf8NameLength() const
{
    return name_symbol -> Utf8_literal
        ? name_symbol -> Utf8_literal -> length
        : 0;
}

//
// Check if this type parameter is from a generic class
//
bool TypeParameterSymbol::IsClassTypeParameter() const
{
    return owner && owner -> TypeCast() != NULL;
}

//
// Check if this type parameter is from a generic method
//
bool TypeParameterSymbol::IsMethodTypeParameter() const
{
    return owner && owner -> MethodCast() != NULL;
}

//
// Get the containing type symbol (for class type parameters)
//
TypeSymbol* TypeParameterSymbol::ContainingType() const
{
    return IsClassTypeParameter() ? owner -> TypeCast() : NULL;
}

//
// Get the containing method symbol (for method type parameters)
//
MethodSymbol* TypeParameterSymbol::ContainingMethod() const
{
    return IsMethodTypeParameter() ? owner -> MethodCast() : NULL;
}

//
// Generate the generic signature for this type parameter
//
// Format (from JVMS):
//   TypeParameter:
//     Identifier ClassBound InterfaceBound*
//   ClassBound:
//     : FieldTypeSignature?
//   InterfaceBound:
//     : FieldTypeSignature
//
// Examples:
//   T -> T:Ljava/lang/Object;
//   T extends Number -> T:Ljava/lang/Number;
//   T extends Number & Comparable<T> -> T:Ljava/lang/Number;:Ljava/lang/Comparable<TT;>;
//
void TypeParameterSymbol::GenerateSignature(Control& /*control*/)
{
    // Estimate size needed
    unsigned estimated_size = NameLength() + 100;  // Buffer for bounds
    char* buffer = new char[estimated_size];
    unsigned length = 0;

    // Add identifier
    const char* name = Utf8Name();
    unsigned name_len = Utf8NameLength();
    for (unsigned i = 0; i < name_len; i++)
        buffer[length++] = name[i];

    // Add class bound (first bound if it's a class, or Object for unbounded)
    buffer[length++] = ':';

    TypeSymbol* first_bound = IsBounded() ? FirstBound() : NULL;

    // For unbounded type parameters, use java.lang.Object as the class bound
    if (!first_bound)
    {
        // Add java/lang/Object
        const char* object_sig = "Ljava/lang/Object;";
        unsigned object_len = 18;
        for (unsigned i = 0; i < object_len; i++)
            buffer[length++] = object_sig[i];
    }
    else
    {
        // Generate signature for first bound
        if (first_bound -> fully_qualified_name)
        {
            buffer[length++] = 'L';
            const char* bound_name = first_bound -> fully_qualified_name -> value;
            unsigned bound_len = first_bound -> fully_qualified_name -> length;

            // Convert dots to slashes (fully_qualified_name uses '/' already for package separator)
            for (unsigned i = 0; i < bound_len; i++)
            {
                buffer[length++] = bound_name[i];
            }
            buffer[length++] = ';';
        }

        // Add interface bounds (all bounds after the first, or all bounds if first was interface)
        for (unsigned i = 1; i < NumBounds(); i++)
        {
            buffer[length++] = ':';
            TypeSymbol* bound = Bound(i);

            if (bound && bound -> fully_qualified_name)
            {
                buffer[length++] = 'L';
                const char* bound_name = bound -> fully_qualified_name -> value;
                unsigned bound_len = bound -> fully_qualified_name -> length;

                for (unsigned j = 0; j < bound_len; j++)
                {
                    buffer[length++] = bound_name[j];
                }
                buffer[length++] = ';';
            }
        }
    }

    // Null terminate
    buffer[length] = '\0';

    // Store the signature
    signature_string = buffer;
    signature_length = length;
}


} // Close namespace Jopa block
