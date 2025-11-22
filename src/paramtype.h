#ifndef paramtype_INCLUDED
#define paramtype_INCLUDED

#include "platform.h"
#include "tuple.h"

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

class TypeSymbol;
class TypeParameterSymbol;
class Control;

// Forward declarations for circular dependencies
class Type;
class ParameterizedType;
class WildcardType;
class ArrayType;

//
// ParameterizedType represents an instantiation of a generic type
// with specific type arguments.
//
// Examples:
//   List<String>              - generic_type = List, type_arguments = [String]
//   Map<String, Integer>      - generic_type = Map, type_arguments = [String, Integer]
//   List<List<String>>        - nested parameterization
//   Outer<String>.Inner<Integer> - enclosing_type = Outer<String>
//
class ParameterizedType
{
public:
    //
    // The raw generic type (e.g., List for List<String>)
    //
    TypeSymbol* generic_type;

    //
    // The actual type arguments substituted for the type parameters
    // For List<String>, this contains one element: String
    // For Map<K,V>, this contains two elements in order
    //
    Tuple<Type*>* type_arguments;

    //
    // For nested generic types like Outer<String>.Inner<Integer>
    // This points to the parameterized outer type
    //
    ParameterizedType* enclosing_type;

    //
    // Constructor for simple parameterized type
    //
    ParameterizedType(TypeSymbol* generic, Tuple<Type*>* args)
        : generic_type(generic)
        , type_arguments(args)
        , enclosing_type(NULL)
    {
    }

    //
    // Constructor with enclosing type
    //
    ParameterizedType(TypeSymbol* generic,
                     Tuple<Type*>* args,
                     ParameterizedType* enclosing)
        : generic_type(generic)
        , type_arguments(args)
        , enclosing_type(enclosing)
    {
    }

    //
    // Destructor
    //
    ~ParameterizedType()
    {
        if (type_arguments)
        {
            for (unsigned i = 0; i < type_arguments -> Length(); i++)
                delete (*type_arguments)[i];
            delete type_arguments;
        }
        delete enclosing_type;
    }

    //
    // Get the erased type (just the raw generic type)
    // List<String> -> List
    //
    TypeSymbol* Erasure()
    {
        return generic_type;
    }

    //
    // Check if this is a raw type (generic type without type arguments)
    // Technically raw types are represented by just TypeSymbol*,
    // but this method checks if type_arguments is NULL
    //
    bool IsRawType() const
    {
        return type_arguments == NULL || type_arguments -> Length() == 0;
    }

    //
    // Get the number of type arguments
    //
    unsigned NumTypeArguments() const
    {
        return type_arguments ? type_arguments -> Length() : 0;
    }

    //
    // Get the i-th type argument
    //
    Type* TypeArgument(unsigned i) const
    {
        assert(type_arguments && i < type_arguments -> Length());
        return (*type_arguments)[i];
    }

    //
    // Generate the generic signature for this parameterized type
    // Used in Signature attribute
    //
    // Example: List<String> -> Ljava/util/List<Ljava/lang/String;>;
    //
    void GenerateSignature(char* buffer, unsigned& length);
};

//
// WildcardType represents wildcard type arguments
//
// Examples:
//   ?                    - UNBOUNDED
//   ? extends Number     - EXTENDS with bound Number
//   ? super Integer      - SUPER with bound Integer
//
class WildcardType
{
public:
    enum BoundKind
    {
        UNBOUNDED,      // ?
        EXTENDS,        // ? extends Foo
        SUPER           // ? super Bar
    };

    BoundKind bound_kind;
    Type* bound;        // NULL for UNBOUNDED

    //
    // Constructor
    //
    WildcardType(BoundKind kind, Type* bound_type = NULL)
        : bound_kind(kind)
        , bound(bound_type)
    {
    }

    //
    // Destructor
    //
    ~WildcardType()
    {
        delete bound;
    }

    //
    // Get the upper bound of this wildcard
    // For ? extends Number, returns Number
    // For ? super Integer, returns Object
    // For ?, returns Object
    //
    TypeSymbol* UpperBound();

    //
    // Get the lower bound of this wildcard
    // For ? super Integer, returns Integer
    // For ? extends Number, returns null (no lower bound)
    // For ?, returns null
    //
    TypeSymbol* LowerBound();

    //
    // Check if this is an unbounded wildcard
    //
    bool IsUnbounded() const { return bound_kind == UNBOUNDED; }

    //
    // Check if this is an upper-bounded wildcard (? extends Foo)
    //
    bool IsExtends() const { return bound_kind == EXTENDS; }

    //
    // Check if this is a lower-bounded wildcard (? super Bar)
    //
    bool IsSuper() const { return bound_kind == SUPER; }

    //
    // Generate the generic signature for this wildcard
    //
    // Examples:
    //   ? -> *
    //   ? extends Number -> +Ljava/lang/Number;
    //   ? super Integer -> -Ljava/lang/Integer;
    //
    void GenerateSignature(char* buffer, unsigned& length);
};

//
// ArrayType represents arrays of generic types
//
// Examples:
//   T[]                  - array of type variable
//   List<String>[]       - array of parameterized type
//   ? extends Number[]   - array of wildcard (actually wildcard of array)
//
class ArrayType
{
public:
    Type* component_type;   // The element type

    //
    // Constructor
    //
    ArrayType(Type* component)
        : component_type(component)
    {
    }

    //
    // Destructor
    //
    ~ArrayType()
    {
        delete component_type;
    }

    //
    // Get the erased type for this array
    // If component erases to Object, array erases to Object[]
    // If component erases to Number, array erases to Number[]
    //
    TypeSymbol* Erasure();

    //
    // Generate signature
    //
    void GenerateSignature(char* buffer, unsigned& length);
};

//
// Type is a discriminated union that can represent any type in the Java type system
//
// This includes:
// - Simple types (TypeSymbol*)
// - Parameterized types (List<String>)
// - Type parameters/variables (T)
// - Wildcards (? extends Number)
// - Arrays (T[], List<String>[])
//
class Type
{
public:
    enum TypeKind
    {
        SIMPLE_TYPE,        // Regular TypeSymbol* (int, String, List (raw))
        PARAMETERIZED_TYPE, // Generic instantiation (List<String>)
        TYPE_PARAMETER,     // Type variable (T, E, K, V)
        WILDCARD_TYPE,      // ? extends/super
        ARRAY_TYPE          // T[] or List<String>[]
    };

    TypeKind kind;

    //
    // Union of possible type representations
    // Only one of these is valid at a time, determined by 'kind'
    //
    union
    {
        TypeSymbol* simple_type;
        ParameterizedType* parameterized_type;
        TypeParameterSymbol* type_parameter;
        WildcardType* wildcard_type;
        ArrayType* array_type;
    };

    //
    // Constructors for each type kind
    //

    Type(TypeSymbol* type)
        : kind(SIMPLE_TYPE), simple_type(type)
    {
    }

    Type(ParameterizedType* ptype)
        : kind(PARAMETERIZED_TYPE), parameterized_type(ptype)
    {
    }

    Type(TypeParameterSymbol* tparam)
        : kind(TYPE_PARAMETER), type_parameter(tparam)
    {
    }

    Type(WildcardType* wildcard)
        : kind(WILDCARD_TYPE), wildcard_type(wildcard)
    {
    }

    Type(ArrayType* array)
        : kind(ARRAY_TYPE), array_type(array)
    {
    }

    //
    // Destructor
    //
    ~Type()
    {
        switch (kind)
        {
            case SIMPLE_TYPE:
                // TypeSymbol is owned elsewhere, don't delete
                break;
            case PARAMETERIZED_TYPE:
                delete parameterized_type;
                break;
            case TYPE_PARAMETER:
                // TypeParameterSymbol is owned by its containing symbol, don't delete
                break;
            case WILDCARD_TYPE:
                delete wildcard_type;
                break;
            case ARRAY_TYPE:
                delete array_type;
                break;
        }
    }

    //
    // Get the erased type
    // Converts this Type to its erasure (TypeSymbol*)
    //
    TypeSymbol* Erasure();

    //
    // Check if this type is generic (has type parameters or arguments)
    //
    bool IsGeneric();

    //
    // Check if this is a raw type (generic type used without type arguments)
    //
    bool IsRawType();

    //
    // Check if this is a type variable
    //
    bool IsTypeParameter() const { return kind == TYPE_PARAMETER; }

    //
    // Check if this is a wildcard
    //
    bool IsWildcard() const { return kind == WILDCARD_TYPE; }

    //
    // Check if this is a parameterized type
    //
    bool IsParameterized() const { return kind == PARAMETERIZED_TYPE; }

    //
    // Check if this is an array type
    //
    bool IsArray() const { return kind == ARRAY_TYPE; }

    //
    // Get the type symbol (for SIMPLE_TYPE)
    //
    TypeSymbol* GetTypeSymbol() const
    {
        assert(kind == SIMPLE_TYPE);
        return simple_type;
    }

    //
    // Get the parameterized type (for PARAMETERIZED_TYPE)
    //
    ParameterizedType* GetParameterizedType() const
    {
        assert(kind == PARAMETERIZED_TYPE);
        return parameterized_type;
    }

    //
    // Get the type parameter (for TYPE_PARAMETER)
    //
    TypeParameterSymbol* GetTypeParameter() const
    {
        assert(kind == TYPE_PARAMETER);
        return type_parameter;
    }

    //
    // Get the wildcard (for WILDCARD_TYPE)
    //
    WildcardType* GetWildcard() const
    {
        assert(kind == WILDCARD_TYPE);
        return wildcard_type;
    }

    //
    // Get the array type (for ARRAY_TYPE)
    //
    ArrayType* GetArrayType() const
    {
        assert(kind == ARRAY_TYPE);
        return array_type;
    }

    //
    // Generate the generic signature for this type
    //
    void GenerateSignature(char* buffer, unsigned& length);

    //
    // Clone this type
    //
    Type* Clone();
};

#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif

#endif // paramtype_INCLUDED
