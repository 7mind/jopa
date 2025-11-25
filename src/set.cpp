#include "set.h"


namespace Jopa { // Open namespace Jopa block
unsigned SymbolSet::primes[] = {DEFAULT_HASH_SIZE, 101, 401, MAX_HASH_SIZE};

void SymbolSet::Rehash()
{
    hash_size = primes[++prime_index];

    delete [] base;
    base = (ShadowSymbol**) memset(new ShadowSymbol*[hash_size], 0,
                                   hash_size * sizeof(ShadowSymbol*));

    for (unsigned k = 0; k < symbol_pool.Length(); k++)
    {
        ShadowSymbol* shadow = symbol_pool[k];
        unsigned i = shadow -> Identity() -> index % hash_size;
        shadow -> next = base[i];
        base[i] = shadow;
    }
}


SymbolSet::~SymbolSet()
{
    SetEmpty();
    delete [] base;
}


bool SymbolSet::operator==(const SymbolSet& rhs) const
{
    if (this != &rhs)
    {
        if (symbol_pool.Length() != rhs.symbol_pool.Length())
            return false;

        for (unsigned i = 0; i < symbol_pool.Length(); i++)
        {
            ShadowSymbol* shadow = symbol_pool[i];
            Symbol* symbol = shadow -> symbol;
            for (unsigned k = 0; symbol;
                 symbol = (Symbol*) (k < shadow -> NumConflicts()
                                     ? shadow -> Conflict(k++) : NULL))
            {
                if (! rhs.IsElement(symbol))
                    return false;
            }
        }
    }
    return true;
}


//
// Union the set in question with the set passed as argument: "set"
//
void SymbolSet::Union(const SymbolSet& set)
{
    if (this != &set)
    {
        for (unsigned i = 0; i < set.symbol_pool.Length(); i++)
        {
            ShadowSymbol* shadow = set.symbol_pool[i];
            Symbol* symbol = shadow -> symbol;
            for (unsigned k = 0; symbol;
                 symbol = (Symbol*) (k < shadow -> NumConflicts()
                                     ? shadow -> Conflict(k++) : NULL))
                AddElement(symbol);
        }
    }
}


//
// Intersect the set in question with the set passed as argument: "set"
//
void SymbolSet::Intersection(const SymbolSet& set)
{
    if (this != &set)
    {
        Tuple<Symbol*> old_symbol_pool(symbol_pool.Length());
        for (unsigned i = 0; i < symbol_pool.Length(); i++)
        {
            ShadowSymbol* shadow = symbol_pool[i];
            Symbol* symbol = shadow -> symbol;
            for (unsigned k = 0; symbol;
                 symbol = (Symbol*) (k < shadow -> NumConflicts()
                                     ? shadow -> Conflict(k++) : NULL))
                old_symbol_pool.Next() = symbol;
        }

        SetEmpty();
        for (unsigned j = 0; j < old_symbol_pool.Length(); j++)
        {
            if (set.IsElement(old_symbol_pool[j]))
                AddElement(old_symbol_pool[j]);
        }
    }
}


//
// Return a boolean value indicating whether or not the set in question
// intersects the set passed as argument: "set" i.e., is there at least one
// element of set that is also an element of "this" set.
//
bool SymbolSet::Intersects(const SymbolSet& set) const
{
    for (unsigned i = 0; i < set.symbol_pool.Length(); i++)
    {
        ShadowSymbol* shadow = set.symbol_pool[i];
        Symbol* symbol = shadow -> symbol;
        for (unsigned k = 0; symbol;
             symbol = (Symbol*) (k < shadow -> NumConflicts()
                                 ? shadow -> Conflict(k++) : NULL))
            if (IsElement(symbol))
                return true;
    }
    return false;
}


//
// Remove element from the set
//
void SymbolSet::RemoveElement(const Symbol* element)
{
    const NameSymbol* name_symbol = element -> Identity();
    unsigned i = name_symbol -> index % hash_size;
    ShadowSymbol* previous = NULL;
    ShadowSymbol* shadow;
    for (shadow = base[i]; shadow; previous = shadow, shadow = shadow -> next)
    {
        if (shadow -> Identity() == name_symbol)
        {
            Symbol* symbol = shadow -> symbol;
            unsigned k;
            for (k = 0; symbol;
                 symbol = (Symbol*) (k < shadow -> NumConflicts()
                                     ? shadow -> Conflict(k++) : NULL))
            {
                if (symbol == element)
                    break;
            }

            if (symbol)
            {
                if (shadow -> NumConflicts() == 0)
                    break;
                shadow -> RemoveConflict(k - 1);
            }
            return;
        }
    }

    if (shadow) // element is the only object contained in shadow
    {
        if (previous == NULL)
             base[i] = shadow -> next;
        else previous -> next = shadow -> next;

        unsigned last_index = symbol_pool.Length() - 1;
        if (shadow -> pool_index != last_index)
        {
            // move last element to position previously occupied by element
            // being deleted
            symbol_pool[last_index] -> pool_index = shadow -> pool_index;
            symbol_pool[shadow -> pool_index] = symbol_pool[last_index];
        }

        symbol_pool.Reset(last_index); // remove last slot in symbol_pool
        delete shadow;
    }
}


unsigned SymbolMap::primes[] = {DEFAULT_HASH_SIZE, 101, 401, MAX_HASH_SIZE};

void SymbolMap::Rehash()
{
    hash_size = primes[++prime_index];

    delete [] base;
    base = (Element**) memset(new Element*[hash_size], 0,
                              hash_size * sizeof(Element*));

    for (unsigned i = 0; i < symbol_pool.Length(); i++)
    {
        Element* element = symbol_pool[i];
        unsigned k =
            element -> domain_element -> Identity() -> index % hash_size;
        element -> next = base[k];
        base[k] = element;
    }
}


void SymbolMap::Map(Symbol* symbol, Symbol* image)
{
    assert(symbol);

    Element* element;
    unsigned k = symbol -> Identity() -> index % hash_size;
    for (element = base[k]; element; element = element -> next)
    {
        if (element -> domain_element == symbol)
            break;
    }

    //
    // If this is a new element, add it to the map.
    //
    if (! element)
    {
        element = new Element();
        element -> domain_element = symbol;
        element -> next = base[k];
        base[k] = element;

        symbol_pool.Next() = element;

        //
        // If the number of unique elements in the map exceeds 2 times
        // the size of the base, and we have not yet reached the maximum
        // allowable size for a base, reallocate a larger base and rehash
        // the elements.
        //
        if (symbol_pool.Length() > (hash_size << 1) &&
            hash_size < MAX_HASH_SIZE)
        {
            Rehash();
        }
    }
    else
    {
        fprintf(stderr, "WARNING: This should not have happened !!!");
    }
    element -> image = image;
}


SymbolMap::SymbolMap(unsigned hash_size_)
{
    hash_size = (hash_size_ <= 0 ? 1 : hash_size_);

    prime_index = -1;
    do
    {
        if (hash_size < primes[prime_index + 1])
            break;
        prime_index++;
    } while (primes[prime_index] < MAX_HASH_SIZE);

    base = (Element**) memset(new Element*[hash_size], 0,
                              hash_size * sizeof(Element*));
}


SymbolMap::~SymbolMap()
{
    for (unsigned i = 0; i < symbol_pool.Length(); i++)
        delete symbol_pool[i];
    delete [] base;
}


} // Close namespace Jopa block

