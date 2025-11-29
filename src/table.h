#pragma once

#include "platform.h"
#include "symbol.h"
#include <cstring>
#include <vector>

namespace Jopa { // Open namespace Jopa block

// Wrapper around std::vector that provides Length() for backward compatibility
template<typename T>
class SymbolPool : public std::vector<T>
{
public:
    unsigned Length() const { return static_cast<unsigned>(this->size()); }
};

// Template base class for shadow symbols - eliminates code duplication
// across VariableShadowSymbol, MethodShadowSymbol, and TypeShadowSymbol
template<typename SymbolT>
class ShadowSymbolBase
{
public:
    SymbolT* symbol;

    explicit ShadowSymbolBase(SymbolT* symbol_)
        : symbol(symbol_)
    {}

    ~ShadowSymbolBase()
    {
        // conflicts vector handles its own cleanup
    }

    SymbolT* Conflict(unsigned i) const { return conflicts[i]; }

    unsigned NumConflicts() const { return static_cast<unsigned>(conflicts.size()); }

    void AddConflict(SymbolT* conflict_symbol)
    {
        if (symbol != conflict_symbol && !Find(conflict_symbol))
            conflicts.push_back(conflict_symbol);
    }

    void CompressSpace()
    {
        conflicts.shrink_to_fit();
    }

protected:
    std::vector<SymbolT*> conflicts;

    bool Find(const SymbolT* conflict_symbol)
    {
        for (SymbolT* s : conflicts)
            if (s == conflict_symbol)
                return true;
        return false;
    }
};

class VariableShadowSymbol : public ShadowSymbolBase<VariableSymbol>
{
public:
    VariableSymbol* variable_symbol;  // Alias for backward compatibility

    explicit VariableShadowSymbol(VariableSymbol* variable_symbol_)
        : ShadowSymbolBase(variable_symbol_), variable_symbol(variable_symbol_)
    {}

private:
    friend class ExpandedFieldTable;
    VariableShadowSymbol* next = nullptr;
};


class MethodShadowSymbol : public ShadowSymbolBase<MethodSymbol>
{
public:
    MethodSymbol* method_symbol;  // Alias for backward compatibility
    MethodShadowSymbol* next_method = nullptr;

    explicit MethodShadowSymbol(MethodSymbol* method_symbol_)
        : ShadowSymbolBase(method_symbol_), method_symbol(method_symbol_)
    {}

private:
    friend class ExpandedMethodTable;
    MethodShadowSymbol* next = nullptr;
};


class TypeShadowSymbol : public ShadowSymbolBase<TypeSymbol>
{
public:
    TypeSymbol* type_symbol;  // Alias for backward compatibility

    explicit TypeShadowSymbol(TypeSymbol* type_symbol_)
        : ShadowSymbolBase(type_symbol_), type_symbol(type_symbol_)
    {}

private:
    friend class ExpandedTypeTable;
    TypeShadowSymbol* next = nullptr;
};


// Template base class for expanded tables - eliminates code duplication
template<typename ShadowT, typename SymbolT>
class ExpandedTableBase
{
public:
    enum
    {
        DEFAULT_HASH_SIZE = 251,
        MAX_HASH_SIZE = 509
    };

    SymbolPool<ShadowT*> symbol_pool;

    explicit ExpandedTableBase(unsigned hash_size_ = DEFAULT_HASH_SIZE)
    {
        hash_size = hash_size_ <= 0 ? 1 : hash_size_ > MAX_HASH_SIZE
            ? static_cast<unsigned>(MAX_HASH_SIZE) : hash_size_;
        base.resize(hash_size, nullptr);
    }

    virtual ~ExpandedTableBase()
    {
        for (ShadowT* s : symbol_pool)
            delete s;
    }

protected:
    std::vector<ShadowT*> base;
    unsigned hash_size;

    void RehashBase()
    {
        hash_size = static_cast<unsigned>(symbol_pool.size());
        hash_size = hash_size <= 0 ? 1 : hash_size > MAX_HASH_SIZE
            ? static_cast<unsigned>(MAX_HASH_SIZE) : hash_size;
        base.assign(hash_size, nullptr);
    }
};


class ExpandedTypeTable : public ExpandedTableBase<TypeShadowSymbol, TypeSymbol>
{
public:
    explicit ExpandedTypeTable(unsigned hash_size_ = DEFAULT_HASH_SIZE)
        : ExpandedTableBase(hash_size_)
    {}

    void CompressSpace()
    {
        RehashBase();
        for (TypeShadowSymbol* shadow : symbol_pool)
        {
            shadow->CompressSpace();
            unsigned k = shadow->type_symbol->name_symbol->index % hash_size;
            shadow->next = base[k];
            base[k] = shadow;
        }
    }

    TypeShadowSymbol* InsertTypeShadowSymbol(TypeSymbol* type_symbol)
    {
        unsigned i = type_symbol->name_symbol->index % hash_size;
        TypeShadowSymbol* p = new TypeShadowSymbol(type_symbol);
        p->next = base[i];
        base[i] = p;
        symbol_pool.push_back(p);
        return p;
    }

    TypeShadowSymbol* FindTypeShadowSymbol(const NameSymbol* name_symbol)
    {
        for (TypeShadowSymbol* p = base[name_symbol->index % hash_size]; p; p = p->next)
            if (p->type_symbol->name_symbol == name_symbol)
                return p;
        return nullptr;
    }
};


class ExpandedFieldTable : public ExpandedTableBase<VariableShadowSymbol, VariableSymbol>
{
public:
    explicit ExpandedFieldTable(unsigned hash_size_ = DEFAULT_HASH_SIZE)
        : ExpandedTableBase(hash_size_)
    {}

    void CompressSpace()
    {
        RehashBase();
        for (VariableShadowSymbol* shadow : symbol_pool)
        {
            shadow->CompressSpace();
            unsigned k = shadow->variable_symbol->name_symbol->index % hash_size;
            shadow->next = base[k];
            base[k] = shadow;
        }
    }

    VariableShadowSymbol* InsertVariableShadowSymbol(VariableSymbol* variable_symbol)
    {
        unsigned i = variable_symbol->name_symbol->index % hash_size;
        VariableShadowSymbol* p = new VariableShadowSymbol(variable_symbol);
        p->next = base[i];
        base[i] = p;
        symbol_pool.push_back(p);
        return p;
    }

    VariableShadowSymbol* FindVariableShadowSymbol(const NameSymbol* name_symbol)
    {
        for (VariableShadowSymbol* p = base[name_symbol->index % hash_size]; p; p = p->next)
            if (p->variable_symbol->name_symbol == name_symbol)
                return p;
        return nullptr;
    }
};


class ExpandedMethodTable : public ExpandedTableBase<MethodShadowSymbol, MethodSymbol>
{
public:
    explicit ExpandedMethodTable(unsigned hash_size_ = DEFAULT_HASH_SIZE)
        : ExpandedTableBase(hash_size_)
    {}

    void CompressSpace()
    {
        RehashBase();
        for (MethodShadowSymbol* shadow : symbol_pool)
        {
            shadow->CompressSpace();
            const NameSymbol* name_symbol = shadow->method_symbol->name_symbol;
            MethodShadowSymbol* base_shadow = FindMethodShadowSymbol(name_symbol);
            if (!base_shadow)
            {
                unsigned k = name_symbol->index % hash_size;
                shadow->next = base[k];
                base[k] = shadow;
                shadow->next_method = nullptr;
            }
            else
            {
                shadow->next_method = base_shadow->next_method;
                base_shadow->next_method = shadow;
            }
        }
    }

    MethodShadowSymbol* FindMethodShadowSymbol(const NameSymbol* name_symbol)
    {
        for (MethodShadowSymbol* p = base[name_symbol->index % hash_size]; p; p = p->next)
            if (p->method_symbol->name_symbol == name_symbol)
                return p;
        return nullptr;
    }

    MethodShadowSymbol* InsertMethodShadowSymbol(MethodSymbol* method_symbol)
    {
        unsigned i = method_symbol->name_symbol->index % hash_size;
        MethodShadowSymbol* p = new MethodShadowSymbol(method_symbol);
        p->next_method = nullptr;
        p->next = base[i];
        base[i] = p;
        symbol_pool.push_back(p);
        return p;
    }

    void Overload(MethodShadowSymbol* base_shadow, MethodSymbol* overload_method)
    {
        MethodShadowSymbol* shadow = new MethodShadowSymbol(overload_method);
        symbol_pool.push_back(shadow);
        shadow->next_method = base_shadow->next_method;
        base_shadow->next_method = shadow;
    }

    MethodShadowSymbol* Overload(MethodSymbol* overload_method)
    {
        MethodShadowSymbol* base_shadow =
            FindMethodShadowSymbol(overload_method->name_symbol);
        if (!base_shadow)
            return InsertMethodShadowSymbol(overload_method);
        Overload(base_shadow, overload_method);
        return base_shadow->next_method;
    }

    MethodShadowSymbol* FindOverloadMethodShadow(MethodSymbol* overload_method,
                                                 Semantic* sem, TokenIndex tok)
    {
        if (!overload_method->IsTyped())
            overload_method->ProcessMethodSignature(sem, tok);

        for (MethodShadowSymbol* method_shadow = FindMethodShadowSymbol(
                 overload_method->name_symbol);
             method_shadow;
             method_shadow = method_shadow->next_method)
        {
            MethodSymbol* method = method_shadow->method_symbol;

            if (overload_method == method)
                return method_shadow;

            if (!method->IsTyped())
                method->ProcessMethodSignature(sem, tok);

            if (overload_method->NumFormalParameters() ==
                method->NumFormalParameters())
            {
                int i;
                for (i = static_cast<int>(method->NumFormalParameters()) - 1; i >= 0; i--)
                {
                    if (method->FormalParameter(i)->Type() !=
                        overload_method->FormalParameter(i)->Type())
                    {
                        break;
                    }
                }

                if (i < 0)
                    return method_shadow;
            }
        }

        return nullptr;
    }
};


} // Close namespace Jopa block
