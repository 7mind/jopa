#pragma once

#include "platform.h"
#include "tuple.h"


namespace Jopa { // Open namespace Jopa block
class TypeSymbol;
class MethodSymbol;
class NameSymbol;
class Symbol;
class Control;

//
// TypeParameterSymbol represents a type parameter declaration
// Examples: T, E, K extends Comparable<K>, V extends Number & Serializable
//
// Type parameters can be declared on:
// 1. Classes/Interfaces: class List<T> { ... }
// 2. Methods: <T> T identity(T arg) { ... }
//
class TypeParameterSymbol
{
public:
    //
    // The name of this type parameter (T, E, K, V, etc.)
    //
    const NameSymbol* name_symbol;

    //
    // The symbol that owns this type parameter.
    // Either a TypeSymbol (for class type parameters) or
    // MethodSymbol (for method type parameters)
    //
    Symbol* owner;

    //
    // Bounds for this type parameter (extends clauses)
    // For example, <T extends Number & Comparable> has two bounds
    // If NULL or empty, bound is implicitly Object
    //
    Tuple<TypeSymbol*>* bounds;

    //
    // Position of this type parameter in the declaration
    // First type parameter is 0, second is 1, etc.
    // Used for substitution and matching type arguments to parameters
    //
    unsigned position;

    //
    // For wildcard capture conversion.
    // When capturing ? extends Number, upper_bound becomes Number
    // When capturing ? super Integer, lower_bound becomes Integer
    //
    TypeSymbol* lower_bound;  // For ? super T
    TypeSymbol* upper_bound;  // For ? extends T (same as FirstBound())

    //
    // Constructor
    //
    TypeParameterSymbol(const NameSymbol* name,
                       Symbol* owner_symbol,
                       unsigned pos)
        : name_symbol(name)
        , owner(owner_symbol)
        , bounds(NULL)
        , position(pos)
        , lower_bound(NULL)
        , upper_bound(NULL)
        , signature_string(NULL)
    {
    }

    //
    // Destructor
    //
    ~TypeParameterSymbol()
    {
        delete bounds;
        delete [] signature_string;
    }

    //
    // Get the erased type for this type parameter.
    // This is the first bound, or Object if no bounds.
    //
    // Examples:
    //   T                  -> Object
    //   T extends Number   -> Number
    //   T extends List & Serializable -> List (first bound)
    //
    TypeSymbol* ErasedType();

    //
    // Check if this type parameter has explicit bounds
    //
    bool IsBounded() const
    {
        return bounds && bounds -> Length() > 0;
    }

    //
    // Get the first bound (primary bound)
    // This is used for erasure and is the only bound that can be a class
    //
    TypeSymbol* FirstBound() const
    {
        return (bounds && bounds -> Length() > 0) ? (*bounds)[0] : NULL;
    }

    //
    // Get the number of bounds
    //
    unsigned NumBounds() const
    {
        return bounds ? bounds -> Length() : 0;
    }

    //
    // Get the i-th bound
    //
    TypeSymbol* Bound(unsigned i) const
    {
        assert(bounds && i < bounds -> Length());
        return (*bounds)[i];
    }

    //
    // Add a bound to this type parameter
    //
    void AddBound(TypeSymbol* bound)
    {
        if (! bounds)
            bounds = new Tuple<TypeSymbol*>(2);
        bounds -> Next() = bound;
    }

    //
    // Get the name as a wide string
    //
    const wchar_t* Name() const;

    //
    // Get the name length
    //
    unsigned NameLength() const;

    //
    // Get the UTF-8 name
    //
    const char* Utf8Name() const;

    //
    // Get the UTF-8 name length
    //
    unsigned Utf8NameLength() const;

    //
    // Check if this type parameter is from a generic class
    // (as opposed to a generic method)
    //
    bool IsClassTypeParameter() const;

    //
    // Check if this type parameter is from a generic method
    //
    bool IsMethodTypeParameter() const;

    //
    // Get the containing type symbol (for class type parameters)
    // Returns NULL if this is a method type parameter
    //
    TypeSymbol* ContainingType() const;

    //
    // Get the containing method symbol (for method type parameters)
    // Returns NULL if this is a class type parameter
    //
    MethodSymbol* ContainingMethod() const;

    //
    // Generate the generic signature for this type parameter
    // Used in Signature attribute generation
    //
    // Examples:
    //   T -> T:Ljava/lang/Object;
    //   T extends Number -> T:Ljava/lang/Number;
    //   T extends Number & Comparable<T> -> T:Ljava/lang/Number;:Ljava/lang/Comparable<TT;>;
    //
    void GenerateSignature(Control&);

    //
    // Cached signature string for Signature attribute
    //
    char* signature_string;
    unsigned signature_length;
};


} // Close namespace Jopa block

