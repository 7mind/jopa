#ifndef table_INCLUDED
#define table_INCLUDED

#include "platform.h"
#include "symbol.h"


namespace Jikes { // Open namespace Jikes block
class VariableShadowSymbol
{
public:
    VariableSymbol* variable_symbol;

    VariableShadowSymbol(VariableSymbol* variable_symbol_)
        : variable_symbol(variable_symbol_),
          conflict(NULL)
    {}

    ~VariableShadowSymbol()
    {
        delete conflict;
    }

    VariableSymbol* Conflict(unsigned i) const { return (*conflict)[i]; }

    inline unsigned NumConflicts() const
    {
        return conflict ? conflict -> Length() : 0;
    }
    inline void AddConflict(VariableSymbol* conflict_symbol)
    {
        if (variable_symbol != conflict_symbol && ! Find(conflict_symbol))
            conflict -> Next() = conflict_symbol;
    }
    inline void CompressSpace()
    {
        if (conflict)
            conflict -> Array();
    }

private:
    friend class ExpandedFieldTable;
    VariableShadowSymbol* next;

    ConvertibleArray<VariableSymbol*>* conflict;

    bool Find(const VariableSymbol* conflict_symbol)
    {
        if (! conflict)
            conflict = new ConvertibleArray<VariableSymbol*>(4);
        for (unsigned k = 0; k < conflict -> Length(); k++)
            if ((*conflict)[k] == conflict_symbol)
                return true;
        return false;
    }
};


class MethodShadowSymbol
{
public:
    MethodSymbol* method_symbol;
    MethodShadowSymbol* next_method;

    MethodShadowSymbol(MethodSymbol* method_symbol_)
        : method_symbol(method_symbol_),
          conflict(NULL)
    {}

    ~MethodShadowSymbol()
    {
        delete conflict;
    }

    MethodSymbol* Conflict(unsigned i) const { return (*conflict)[i]; }

    inline unsigned NumConflicts() const
    {
        return conflict ? conflict -> Length() : 0;
    }
    inline void AddConflict(MethodSymbol* conflict_symbol)
    {
        if (method_symbol != conflict_symbol && ! Find(conflict_symbol))
            conflict -> Next() = conflict_symbol;
    }

    inline void CompressSpace()
    {
        if (conflict)
            conflict -> Array();
    }

private:
    friend class ExpandedMethodTable;
    MethodShadowSymbol* next;

    ConvertibleArray<MethodSymbol*>* conflict;

    bool Find(const MethodSymbol* conflict_symbol)
    {
        if (! conflict)
            conflict = new ConvertibleArray<MethodSymbol*>(4);
        for (unsigned k = 0; k < conflict -> Length(); k++)
            if ((*conflict)[k] == conflict_symbol)
                return true;
        return false;
    }
};


class TypeShadowSymbol
{
public:
    TypeSymbol* type_symbol;

    TypeShadowSymbol(TypeSymbol* type_symbol_)
        : type_symbol(type_symbol_),
          conflict(NULL)
    {}

    ~TypeShadowSymbol()
    {
        delete conflict;
    }

    TypeSymbol* Conflict(unsigned i) const { return (*conflict)[i]; }

    inline unsigned NumConflicts() const
    {
        return conflict ? conflict -> Length() : 0;
    }

    inline void AddConflict(TypeSymbol* conflict_symbol)
    {
        if (type_symbol != conflict_symbol && ! Find(conflict_symbol))
            conflict -> Next() = conflict_symbol;
    }

    inline void CompressSpace()
    {
        if (conflict)
            conflict -> Array();
    }

private:
    friend class ExpandedTypeTable;
    TypeShadowSymbol* next;

    ConvertibleArray<TypeSymbol*>* conflict;

    bool Find(const TypeSymbol* conflict_symbol)
    {
        if (! conflict)
            conflict = new ConvertibleArray<TypeSymbol*>(4);
        for (unsigned k = 0; k < conflict -> Length(); k++)
            if ((*conflict)[k] == conflict_symbol)
                return true;
        return false;
    }
};


class ExpandedTypeTable
{
public:
    enum
    {
        DEFAULT_HASH_SIZE = 251,
        MAX_HASH_SIZE = 509
    };

    ConvertibleArray<TypeShadowSymbol*> symbol_pool;

    inline void CompressSpace()
    {
        hash_size = symbol_pool.Length();
        hash_size = hash_size <= 0 ? 1 : hash_size > MAX_HASH_SIZE
            ? (unsigned) MAX_HASH_SIZE : hash_size;
        delete [] base;
        base = (TypeShadowSymbol**)
            memset(new TypeShadowSymbol*[hash_size], 0,
                   hash_size * sizeof(TypeShadowSymbol*));

        TypeShadowSymbol** array = symbol_pool.Array();
        for (unsigned i = 0; i < symbol_pool.Length(); i++)
        {
            array[i] -> CompressSpace();

            unsigned k =
                array[i] -> type_symbol -> name_symbol -> index % hash_size;
            array[i] -> next = base[k];
            base[k] = array[i];
        }
    }

    ExpandedTypeTable(unsigned hash_size_ = DEFAULT_HASH_SIZE)
        : symbol_pool(10, 4)
    {
        hash_size = hash_size_ <= 0 ? 1 : hash_size_ > MAX_HASH_SIZE
            ? (unsigned) MAX_HASH_SIZE : hash_size_;
        base = (TypeShadowSymbol**)
            memset(new TypeShadowSymbol*[hash_size], 0,
                   hash_size * sizeof(TypeShadowSymbol*));
    }

    ~ExpandedTypeTable()
    {
        for (unsigned k = 0; k < symbol_pool.Length(); k++)
            delete symbol_pool[k];
        delete [] base;
    }

    inline TypeShadowSymbol* InsertTypeShadowSymbol(TypeSymbol* type_symbol)
    {
        unsigned i = type_symbol -> name_symbol -> index % hash_size;
        TypeShadowSymbol* p = new TypeShadowSymbol(type_symbol);
        p -> next = base[i];
        base[i] = p;
        symbol_pool.Next() = p;

        return p;
    }

    inline TypeShadowSymbol* FindTypeShadowSymbol(const NameSymbol* name_symbol)
    {
        TypeShadowSymbol* p;
        for (p = base[name_symbol -> index % hash_size]; p; p = p -> next)
             if (p -> type_symbol -> name_symbol == name_symbol)
                 break;
        return p;
    }

private:
    TypeShadowSymbol** base;
    unsigned hash_size;
};

class ExpandedFieldTable
{
public:
    enum
    {
        DEFAULT_HASH_SIZE = 251,
        MAX_HASH_SIZE = 509
    };

    ConvertibleArray<VariableShadowSymbol*> symbol_pool;

    inline void CompressSpace()
    {
        hash_size = symbol_pool.Length();
        hash_size = hash_size <= 0 ? 1 : hash_size > MAX_HASH_SIZE
            ? (unsigned) MAX_HASH_SIZE : hash_size;
        delete [] base;
        base = (VariableShadowSymbol**)
            memset(new VariableShadowSymbol*[hash_size], 0,
                   hash_size * sizeof(VariableShadowSymbol*));

        VariableShadowSymbol** array = symbol_pool.Array();
        for (unsigned i = 0; i < symbol_pool.Length(); i++)
        {
            array[i] -> CompressSpace();

            unsigned k = array[i] -> variable_symbol -> name_symbol -> index %
                hash_size;
            array[i] -> next = base[k];
            base[k] = array[i];
        }
    }

    ExpandedFieldTable(unsigned hash_size_ = DEFAULT_HASH_SIZE)
        : symbol_pool(10, 4)
    {
        hash_size = hash_size_ <= 0 ? 1 : hash_size_ > MAX_HASH_SIZE
            ? (unsigned) MAX_HASH_SIZE : hash_size_;
        base = (VariableShadowSymbol**)
            memset(new VariableShadowSymbol*[hash_size], 0,
                   hash_size * sizeof(VariableShadowSymbol*));
    }
    ~ExpandedFieldTable()
    {
        for (unsigned i = 0; i < symbol_pool.Length(); i++)
            delete symbol_pool[i];
        delete [] base;
    }

    inline VariableShadowSymbol* InsertVariableShadowSymbol(VariableSymbol* variable_symbol)
    {
        unsigned i = variable_symbol -> name_symbol -> index % hash_size;
        VariableShadowSymbol* p = new VariableShadowSymbol(variable_symbol);
        p -> next = base[i];
        base[i] = p;
        symbol_pool.Next() = p;

        return p;
    }

    inline VariableShadowSymbol* FindVariableShadowSymbol(const NameSymbol* name_symbol)
    {
        VariableShadowSymbol* p;
        for (p = base[name_symbol -> index % hash_size]; p; p = p -> next)
            if (p -> variable_symbol -> name_symbol == name_symbol)
                break;
        return p;
    }

private:
    VariableShadowSymbol** base;
    unsigned hash_size;
};


class ExpandedMethodTable
{
public:
    enum
    {
        DEFAULT_HASH_SIZE = 251,
        MAX_HASH_SIZE = 509
    };

    ConvertibleArray<MethodShadowSymbol*> symbol_pool;

    inline void CompressSpace()
    {
        hash_size = symbol_pool.Length();
        hash_size = hash_size <= 0 ? 1 : hash_size > MAX_HASH_SIZE
            ? (unsigned) MAX_HASH_SIZE : hash_size;
        delete [] base;
        base = (MethodShadowSymbol**)
            memset(new MethodShadowSymbol*[hash_size], 0,
                   hash_size * sizeof(MethodShadowSymbol*));

        MethodShadowSymbol** array = symbol_pool.Array();
        for (unsigned i = 0; i < symbol_pool.Length(); i++)
        {
            array[i] -> CompressSpace();

            const NameSymbol* name_symbol =
                array[i] -> method_symbol -> name_symbol;
            MethodShadowSymbol* base_shadow =
                FindMethodShadowSymbol(name_symbol);
            if (! base_shadow)
            {
                unsigned k = name_symbol -> index % hash_size;
                array[i] -> next = base[k];
                base[k] = array[i];
                array[i] -> next_method = NULL;
            }
            else
            {
                array[i] -> next_method = base_shadow -> next_method;
                base_shadow -> next_method = array[i];
            }
        }
    }

    ExpandedMethodTable(unsigned hash_size_ = DEFAULT_HASH_SIZE)
        : symbol_pool(10, 4)
    {
        hash_size = hash_size_ <= 0 ? 1 : hash_size_ > MAX_HASH_SIZE
            ? (unsigned) MAX_HASH_SIZE : hash_size_;
        base = (MethodShadowSymbol**)
            memset(new MethodShadowSymbol*[hash_size], 0,
                   hash_size * sizeof(MethodShadowSymbol*));
    }
    ~ExpandedMethodTable()
    {
        for (unsigned i = 0; i < symbol_pool.Length(); i++)
            delete symbol_pool[i];
       delete [] base;
    }

    inline MethodShadowSymbol* FindMethodShadowSymbol(const NameSymbol* name_symbol)
    {
        MethodShadowSymbol* p;
        for (p = base[name_symbol -> index % hash_size]; p; p = p -> next)
            if (p -> method_symbol -> name_symbol == name_symbol)
                break;
        return p;
    }

    inline MethodShadowSymbol* InsertMethodShadowSymbol(MethodSymbol* method_symbol)
    {
        unsigned i = method_symbol -> name_symbol -> index % hash_size;
        MethodShadowSymbol* p = new MethodShadowSymbol(method_symbol);
        p -> next_method = NULL;
        p -> next = base[i];
        base[i] = p;
        symbol_pool.Next() = p;

        return p;
    }

    inline void Overload(MethodShadowSymbol* base_shadow,
                         MethodSymbol* overload_method)
    {
        //
        // Insert the new overload as the second list element, to preserve
        // the existing base, while making Overload(MethodSymbol*) work.
        //
        MethodShadowSymbol* shadow = new MethodShadowSymbol(overload_method);
        symbol_pool.Next() = shadow;
        shadow -> next_method = base_shadow -> next_method;
        base_shadow -> next_method = shadow;
    }

    inline MethodShadowSymbol* Overload(MethodSymbol* overload_method)
    {
        MethodShadowSymbol* base_shadow =
            FindMethodShadowSymbol(overload_method -> name_symbol);
        if (! base_shadow)
            return InsertMethodShadowSymbol(overload_method);
        Overload(base_shadow, overload_method);
        //
        // Return the newly created overload; this relies on the current
        // behavior of Overload(MethodSymbol*, MethodSymbol*).
        //
        return base_shadow -> next_method;
    }

    MethodShadowSymbol* FindOverloadMethodShadow(MethodSymbol* overload_method,
                                                 Semantic* sem, TokenIndex tok)
    {
        if (! overload_method -> IsTyped())
            overload_method -> ProcessMethodSignature(sem, tok);

        MethodShadowSymbol* method_shadow;
        for (method_shadow = FindMethodShadowSymbol(overload_method ->
                                                    name_symbol);
             method_shadow;
             method_shadow = method_shadow -> next_method)
        {
            MethodSymbol* method = method_shadow -> method_symbol;

            if (overload_method == method)
                return method_shadow;

            if (! method -> IsTyped())
                method -> ProcessMethodSignature(sem, tok);

            if (overload_method -> NumFormalParameters() ==
                method -> NumFormalParameters())
            {
                int i;
                for (i = method -> NumFormalParameters() - 1; i >= 0; i--)
                {
                    if (method -> FormalParameter(i) -> Type() !=
                        overload_method -> FormalParameter(i) -> Type())
                    {
                        break;
                    }
                }

                if (i < 0)
                    return method_shadow;
            }
        }

        return method_shadow;
    }

private:
    MethodShadowSymbol** base;
    unsigned hash_size;
};


} // Close namespace Jikes block

#endif // table_INCLUDED

