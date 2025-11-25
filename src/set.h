#ifndef set_INCLUDED
#define set_INCLUDED

#include "platform.h"
#include "lookup.h"


namespace Jopa { // Open namespace Jopa block
class ShadowSymbol
{
public:
    ShadowSymbol* next;
    Symbol* symbol;
    unsigned pool_index;

    inline const NameSymbol* Identity() { return symbol -> Identity(); }

    ShadowSymbol(Symbol* symbol_)
        : symbol(symbol_),
          conflict(NULL)
    {}

    ~ShadowSymbol() { delete conflict; }

    Symbol* Conflict(unsigned i) { return (*conflict)[i]; }

    inline unsigned NumConflicts()
    {
        return conflict ? conflict -> Length() : 0;
    }

    inline void AddConflict(Symbol* conflict_symbol)
    {
        if (symbol != conflict_symbol && ! Find(conflict_symbol))
            conflict -> Next() = conflict_symbol;
    }

    inline void RemoveConflict(int k)
    {
        unsigned last_index = conflict -> Length() - 1;
        if (k < 0) // when k is negative, it identifies the main symbol
             symbol = (*conflict)[last_index];
        else (*conflict)[k] = (*conflict)[last_index];
        conflict -> Reset(last_index);
    }

private:
    Tuple<Symbol*>* conflict;

    bool Find(Symbol* conflict_symbol)
    {
        if (! conflict)
            conflict = new Tuple<Symbol*>(4);
        for (unsigned k = 0; k < conflict -> Length(); k++)
            if ((*conflict)[k] == conflict_symbol)
                return true;
        return false;
    }
};


class SymbolSet
{
public:
    enum
    {
        DEFAULT_HASH_SIZE = 13,
        MAX_HASH_SIZE = 1021
    };

    SymbolSet(unsigned hash_size_ = DEFAULT_HASH_SIZE)
        : symbol_pool(256),
          main_index(0),
          sub_index(0)
    {
        hash_size = (hash_size_ <= 0 ? 1 : hash_size_);

        prime_index = -1;
        do
        {
            if (hash_size < primes[prime_index + 1])
                break;
            prime_index++;
        } while (primes[prime_index] < MAX_HASH_SIZE);

        base = (ShadowSymbol**) memset(new ShadowSymbol*[hash_size], 0,
                                       hash_size * sizeof(ShadowSymbol*));
    }

    ~SymbolSet();

    //
    // Calculate the size of the set and return the value.
    //
    inline unsigned Size()
    {
        unsigned size = 0;

        for (unsigned i = 0; i < symbol_pool.Length(); i++)
        {
            ShadowSymbol* shadow = symbol_pool[i];
            Symbol* symbol = shadow -> symbol;
            for (unsigned k = 0; symbol;
                 symbol = (Symbol*) (k < shadow -> NumConflicts()
                                     ? shadow -> Conflict(k++) : NULL))
            {
                size++;
            }
        }
        return size;
    }

    //
    // Empty out the set in question - i.e., remove all its elements
    //
    inline void SetEmpty()
    {
        for (unsigned i = 0; i < symbol_pool.Length(); i++)
            delete symbol_pool[i];
        symbol_pool.Reset();
        base = (ShadowSymbol**) memset(base, 0,
                                       hash_size * sizeof(ShadowSymbol*));
    }

    //
    // Determine whether the set is empty.
    //
    bool IsEmpty() { return symbol_pool.Length() == 0; }

    //
    // Assignment of a set to another.
    //
    SymbolSet& operator=(const SymbolSet& rhs)
    {
        if (this != &rhs)
        {
            this -> SetEmpty();
            this -> Union(rhs);
        }
        return *this;
    }

    //
    // Equality comparison of two sets
    //
    bool operator==(const SymbolSet&) const;

    //
    // NonEquality comparison of two sets
    //
    inline bool operator!=(const SymbolSet& rhs) const
    {
        return ! (*this == rhs);
    }

    //
    // Union the set in question with the set passed as argument: "set"
    //
    void Union(const SymbolSet&);

    //
    // Intersect the set in question with the set passed as argument: "set"
    //
    void Intersection(const SymbolSet&);

    //
    // Return a bolean value indicating whether or not the set in question
    // intersects the set passed as argument; i.e., is there at least one
    // element of set that is also an element of "this" set.
    //
    bool Intersects(const SymbolSet&) const;

    //
    // How many elements with this name symbol do we have?
    //
    inline unsigned NameCount(const Symbol* element) const
    {
        const NameSymbol* name_symbol = element -> Identity();
        for (ShadowSymbol* shadow = base[name_symbol -> index % hash_size];
             shadow; shadow = shadow -> next)
        {
            if (shadow -> Identity() == name_symbol)
                return shadow -> NumConflicts() + 1;
        }
        return 0;
    }

    //
    // Is "element" an element of the set in question ?
    //
    inline bool IsElement(const Symbol* element) const
    {
        assert(element);

        const NameSymbol* name_symbol = element -> Identity();
        for (ShadowSymbol* shadow = base[name_symbol -> index % hash_size];
             shadow; shadow = shadow -> next)
        {
            if (shadow -> Identity() == name_symbol)
            {
                Symbol* symbol = shadow -> symbol;
                for (unsigned k = 0; symbol;
                     symbol = (Symbol*) (k < shadow -> NumConflicts()
                                         ? shadow -> Conflict(k++) : NULL))
                {
                    if (symbol == element)
                        return true;
                }
                return false;
            }
        }
        return false;
    }

    //
    // Add element to the set in question if was not already there.
    //
    inline void AddElement(Symbol* element)
    {
        const NameSymbol* name_symbol = element -> Identity();
        unsigned i = name_symbol -> index % hash_size;

        ShadowSymbol* shadow;
        for (shadow = base[i]; shadow; shadow = shadow -> next)
        {
            if (shadow -> Identity() == name_symbol)
            {
                shadow -> AddConflict(element);
                return;
            }
        }

        shadow = new ShadowSymbol(element);
        shadow -> pool_index = symbol_pool.Length();
        symbol_pool.Next() = shadow;

        shadow -> next = base[i];
        base[i] = shadow;

        //
        // If the set is "adjustable" and the number of unique elements in it
        // exceeds 2 times the size of the base, and we have not yet reached
        // the maximum allowable size for a base, reallocate a larger base
        // and rehash the elements.
        //
        if (symbol_pool.Length() > (hash_size << 1) &&
            hash_size < MAX_HASH_SIZE)
        {
            Rehash();
        }
    }


    void RemoveElement(const Symbol*);

    Symbol* FirstElement()
    {
        main_index = 0;
        sub_index = 0;
        return main_index < symbol_pool.Length()
            ? symbol_pool[main_index] -> symbol : (Symbol*) NULL;
    }

    Symbol* NextElement()
    {
        Symbol* symbol = NULL;

        if (main_index < symbol_pool.Length())
        {
             if (sub_index < symbol_pool[main_index] -> NumConflicts())
                 symbol = symbol_pool[main_index] -> Conflict(sub_index++);
             else
             {
                 main_index++;
                 sub_index = 0;
                 symbol = (main_index < symbol_pool.Length()
                           ? symbol_pool[main_index] -> symbol
                           : (Symbol*) NULL);
             }
        }
        return symbol;
    }

protected:
    Tuple<ShadowSymbol*> symbol_pool;

    unsigned main_index;
    unsigned sub_index;

    ShadowSymbol** base;
    unsigned hash_size;

    static unsigned primes[];
    int prime_index;

    void Rehash();
};


//
// Single-value Mapping from a name_symbol into a symbol with that name.
//
class NameSymbolMap : public SymbolSet
{
public:
    NameSymbolMap(unsigned hash_size_ = DEFAULT_HASH_SIZE)
        : SymbolSet(hash_size_)
    {}

    //
    // Is there an element with this name in the map ?
    //
    inline Symbol* Image(const NameSymbol* name_symbol)
    {
        assert(name_symbol);

        for (ShadowSymbol* shadow = base[name_symbol -> index % hash_size];
             shadow; shadow = shadow -> next)
        {
            if (shadow -> Identity() == name_symbol)
                return shadow -> symbol;
        }
        return NULL;
    }

    //
    // Add element to the set in question if was not already there.
    //
    inline void AddElement(Symbol* element)
    {
        assert(element);

        ShadowSymbol* shadow = NULL;
        for (shadow = base[element -> Identity() -> index % hash_size];
             shadow; shadow = shadow -> next)
        {
            if (shadow -> Identity() == element -> Identity())
                break;
        }

        //
        // If an element was already mapped into that name, replace it.
        // Otherwise, add the new element.
        //
        if (shadow)
            shadow -> symbol = element;
        else SymbolSet::AddElement(element);
    }
};


//
// Single-value Mapping from an arbitrary key to an arbitrary value, based
// on memory location (and not key contents).  At the moment, the Map does
// not support remapping a key to a new value.
//
// class Key must implement a hashing function:
//   unsigned HashCode();
//
template <typename Key, typename Value>
class Map
{
public:
    enum
    {
        DEFAULT_HASH_SIZE = 13,
        MAX_HASH_SIZE = 1021
    };

    Map(unsigned hash_size_ = DEFAULT_HASH_SIZE);
    virtual ~Map()
    {
        for (unsigned i = 0; i < symbol_pool.Length(); i++)
            delete symbol_pool[i];
        delete [] base;
    }


    //
    // Has key been mapped to an image, yet? If so, return the image.
    //
    inline Value* Image(Key* key)
    {
        assert(key);

        // Unsigned math prevents negative indices.
        unsigned k = key -> HashCode() % hash_size;
        for (Element* element = base[k]; element; element = element -> next)
        {
            if (element -> key == key)
                return element -> value;
        }
        return NULL;
    }

    //
    // Map or remap key to a given image.
    //
    void Add(Key*, Value*);

private:
    struct Element
    {
        Element* next;
        Key* key;
        Value* value;
    };

    Tuple<Element*> symbol_pool;

    Element** base;
    unsigned hash_size;

    void Rehash();
    void Resize();
};


//
// Single-value Mapping from an arbitrary symbol into another arbitrary symbol.
//
class SymbolMap
{
public:
    enum
    {
        DEFAULT_HASH_SIZE = 13,
        MAX_HASH_SIZE = 1021
    };

    SymbolMap(unsigned hash_size_ = DEFAULT_HASH_SIZE);
    ~SymbolMap();

    //
    // Has symbol been mapped to an image, yet? If so, return the image.
    //
    inline Symbol* Image(Symbol* symbol)
    {
        assert(symbol);

        unsigned k = symbol -> Identity() -> index % hash_size;
        for (Element* element = base[k]; element; element = element -> next)
        {
            if (element -> domain_element == symbol)
                return element -> image;
        }
        return NULL;
    }

    //
    // Map or remap symbol to a given image.
    //
    void Map(Symbol*, Symbol*);

private:
    struct Element
    {
        Element* next;
        Symbol* domain_element;
        Symbol* image;
    };

    Tuple<Element*> symbol_pool;

    Element** base;
    unsigned hash_size;

    static unsigned primes[];
    int prime_index;

    void Rehash();
};


class DefinitePair;
class DefiniteAssignmentSet;

//
// This Bitset template class can be used to construct sets of
// integers. The template takes as argument the address of an integer
// variable, set_size, which indicates that the universe of the sets
// is: {0..*set_size}.
//
class BitSet
{
    typedef unsigned CELL;

    CELL* s;
    unsigned set_size;
    unsigned max_set_size;

public:

    enum { EMPTY, UNIVERSE, cell_size = sizeof(CELL) * CHAR_BIT };

    //
    // Produce the empty set.
    //
    void SetEmpty()
    {
        memset(s, 0, (set_size + cell_size - 1) / cell_size * sizeof(CELL));
    }

    //
    // Produce the universe set.
    //
    void SetUniverse()
    {
        memset(s, 0xFF,
               (set_size + cell_size - 1) / cell_size * sizeof(CELL));
    }

    //
    // This function takes as argument the size of a hash table, table_size.
    // It hashes a bitset into a location within the range <1..table_size-1>.
    // Note that a set's hash value changes when its bits change, so be careful
    // that only constant sets are used as hash keys.
    //
    unsigned Hash(int table_size) const
    {
        unsigned long hash_address = 0;

        for (int i = ((int) set_size - 1) / cell_size; i >= 0; i--)
            hash_address += s[i];
        return hash_address % table_size;
    }

    //
    // Assignment of a bitset to another, the two sets must be the same size.
    //
    BitSet& operator=(const BitSet& rhs)
    {
        if (this != &rhs)
        {
            assert(set_size == rhs.set_size);
            memcpy(s, rhs.s,
                   (set_size + cell_size - 1) / cell_size * sizeof(CELL));
        }
        return *this;
    }

    //
    // Constructor of an uninitialized bitset.
    //
#ifdef HAVE_EXPLICIT
    explicit
#endif
    BitSet(unsigned set_size_)
        : set_size(set_size_),
          max_set_size(set_size_)
    {
        //
        // Note that some C++ compilers do not know how to allocate an
        // array of size 0.
        //
        int num_cells = (set_size + cell_size - 1) / cell_size;
        s = new CELL[num_cells ? num_cells : 1];
    }

    //
    // Constructor of an initialized bitset.
    //
    BitSet(unsigned set_size_, int init)
        : set_size(set_size_),
          max_set_size(set_size_)
    {
        //
        // Note that some C++ compilers do not know how to allocate an
        // array of size 0.
        //
        int num_cells = (set_size + cell_size - 1) / cell_size;
        s = new CELL[num_cells ? num_cells : 1];
        if (init == UNIVERSE)
            SetUniverse();
        else SetEmpty();
    }

    //
    // Constructor to clone a bitset.
    //
    BitSet(const BitSet& rhs)
        : set_size(rhs.set_size),
          max_set_size(set_size)
    {
        int num_cells = (set_size + cell_size - 1) / cell_size;
        s = new CELL[num_cells ? num_cells : 1];
        memcpy(s, rhs.s, num_cells * sizeof(CELL));
    }

    //
    // Destructor of a bitset.
    //
    ~BitSet() { delete [] s; }

    //
    // Return size of a bit set.
    //
    unsigned Size() const { return set_size; }

    //
    // Return a boolean value indicating whether or not the element i
    // is in the bitset in question.
    //
    bool operator[](const unsigned i) const
    {
        assert(i < set_size);

        return 0 != (s[i / cell_size] &
                     (i % cell_size ? (CELL) 1 << (i % cell_size) : (CELL) 1));
    }

    //
    // Insert an element i in the bitset in question.
    //
    void AddElement(const unsigned i)
    {
        assert(i < set_size);

        s[i / cell_size] |= (i % cell_size ? (CELL) 1 << (i % cell_size)
                             : (CELL) 1);
    }

    //
    // Remove an element i from the bitset in question.
    //
    void RemoveElement(const unsigned i)
    {
        assert(i < set_size);

        s[i / cell_size] &= ~(i % cell_size ? (CELL) 1 << (i % cell_size)
                              : (CELL) 1);
    }

    //
    // Yield a boolean result indicating whether or not two sets are
    // identical.
    //
    bool operator==(const BitSet& rhs) const
    {
        if (set_size != rhs.set_size)
            return false;

        int i = ((int) set_size - 1) / cell_size;
        if (set_size &&
            ((s[i] ^ rhs.s[i]) &
             (i % cell_size ? ((CELL) 1 << (i % cell_size)) - (CELL) 1
              : ~((CELL) 0))) != 0)
        {
            return false;
        }
        while (--i >= 0)
        {
            if (s[i] != rhs.s[i])
                return false;
        }

        return true;
    }

    //
    // Yield a boolean result indicating whether or not two sets are
    // identical.
    //
    bool operator!=(const BitSet& rhs) const
    {
        return ! (*this == rhs);
    }

    //
    // Union of two bitsets.
    //
    BitSet operator+(const BitSet& rhs) const
    {
        return BitSet(*this) += rhs;
    }

    //
    // Union of an lvalue bitset and a rhs bitset.
    //
    BitSet& operator+=(const BitSet& rhs)
    {
        for (int i = ((int) set_size - 1) / cell_size; i >= 0; i--)
            s[i] |= rhs.s[i];

        return *this;
    }

    //
    // Intersection of two bitsets.
    //
    BitSet operator*(const BitSet& rhs) const
    {
        return BitSet(*this) *= rhs;
    }

    //
    // Intersection of an lvalue bitset and a rhs bitset.
    //
    BitSet& operator*=(const BitSet& rhs)
    {
        for (int i = ((int) set_size - 1) / cell_size; i >= 0; i--)
            s[i] &= rhs.s[i];

        return *this;
    }

    //
    // Difference of two bitsets.
    //
    BitSet operator-(const BitSet& rhs) const
    {
        return BitSet(*this) -= rhs;
    }

    //
    // Difference of an lvalue bitset and a rhs bitset.
    //
    BitSet& operator-=(const BitSet& rhs)
    {
        for (int i = ((int) set_size - 1) / cell_size; i >= 0; i--)
            s[i] &= (~ rhs.s[i]);

        return *this;
    }

    //
    // Changes the size of the set. Any new bits are given the value of init.
    //
    void Resize(const unsigned new_size, const int init = EMPTY)
    {
        if (new_size > max_set_size)
        {
            int new_cell_count = (new_size + cell_size - 1) / cell_size;
            int old_cell_count = (max_set_size + cell_size - 1) / cell_size;
            if (new_cell_count > old_cell_count && new_cell_count > 1)
            {
                // Must grow the storage for the set.
                CELL* tmp = s;
                s = new CELL[new_cell_count];
                memcpy(s, tmp, old_cell_count * sizeof(CELL));
                delete [] tmp;
            }
            max_set_size = new_size;
        }
        if (new_size > set_size)
        {
            // Initialize new bits.
            int i = (new_size - 1) / cell_size;
            while (i > ((int) set_size + cell_size - 1) / cell_size - 1)
                s[i--] = init == EMPTY ? (CELL) 0 : ~((CELL) 0);
            if (set_size)
            {
                if (init == EMPTY)
                    s[i] &= (set_size % cell_size
                             ? ((CELL) 1 << (set_size % cell_size)) - (CELL) 1
                             : ~((CELL) 0));
                else
                    s[i] |= (set_size % cell_size
                             ? ~(((CELL) 1 << (set_size % cell_size))
                                 - (CELL) 1)
                             : (CELL) 0);
            }
        }
        set_size = new_size;
    }
};


//
// The DefinitePair class holds two Bitsets, one for definite assignment,
// and one for definite unassignment.
//
class DefinitePair
{
public:
    BitSet da_set;
    BitSet du_set;

    //
    // Constructor to clone a definite pair.
    //
    inline DefinitePair(const DefinitePair& rhs)
        : da_set(rhs.da_set),
          du_set(rhs.du_set)
    {}

    //
    // Other useful constructors.
    //
    inline DefinitePair(unsigned size = 0)
        : da_set(size, BitSet::EMPTY),
          du_set(size, BitSet::UNIVERSE)
    {}

    inline DefinitePair(unsigned size, int init)
        : da_set(size, init),
          du_set(size, init)
    {}

    inline DefinitePair(const BitSet da, const BitSet du)
        : da_set(da),
          du_set(du)
    {
        assert(da.Size() == du.Size());
    }

    //
    // Set to the results when true * results when false
    //
    inline DefinitePair(const DefiniteAssignmentSet& set);

    //
    // Set both bitsets.
    //
    inline void SetEmpty()
    {
        da_set.SetEmpty();
        du_set.SetEmpty();
    }
    inline void SetUniverse()
    {
        da_set.SetUniverse();
        du_set.SetUniverse();
    }
    inline void AssignAll()
    {
        da_set.SetUniverse();
        du_set.SetEmpty();
    }

    //
    // Resize the bitsets.
    //
    inline void Resize(const unsigned size)
    {
        da_set.Resize(size, BitSet::EMPTY);
        du_set.Resize(size, BitSet::UNIVERSE);
    }
    inline void Resize(const unsigned size, const int init)
    {
        da_set.Resize(size, init);
        du_set.Resize(size, init);
    }

    inline DefinitePair& operator=(const DefinitePair& rhs)
    {
        if (this != &rhs)
        {
            da_set = rhs.da_set;
            du_set = rhs.du_set;
        }
        return *this;
    }

    inline DefinitePair& operator=(const DefiniteAssignmentSet& rhs);

    inline unsigned Size() const { return da_set.Size(); }

    //
    // Modify element i in both bitsets.
    //
    inline void AddElement(unsigned i)
    {
        da_set.AddElement(i);
        du_set.AddElement(i);
    }
    inline void RemoveElement(unsigned i)
    {
        da_set.RemoveElement(i);
        du_set.RemoveElement(i);
    }

    //
    // An assignment statement adds to da, but removes from du; reclaim it when
    // the variable leaves scope.
    //
    inline void AssignElement(unsigned i)
    {
        da_set.AddElement(i);
        du_set.RemoveElement(i);
    }
    inline void ReclaimElement(unsigned i)
    {
        da_set.RemoveElement(i);
        du_set.AddElement(i);
    }

    //
    // da == da && du == du
    //
    inline bool operator==(const DefinitePair& rhs) const
    {
        return da_set == rhs.da_set && du_set == rhs.du_set;
    }
    inline bool operator!=(const DefinitePair& rhs) const
    {
        return ! (*this == rhs);
    }

    //
    // Union
    //
    inline DefinitePair operator+(const DefinitePair& rhs) const
    {
        return DefinitePair(*this) += rhs;
    }
    inline DefinitePair& operator+=(const DefinitePair& rhs)
    {
        da_set += rhs.da_set;
        du_set += rhs.du_set;
        return *this;
    }

    //
    // Intersection
    //
    inline DefinitePair operator*(const DefinitePair& rhs) const
    {
        return DefinitePair(*this) *= rhs;
    }
    inline DefinitePair& operator*=(const DefinitePair& rhs)
    {
        da_set *= rhs.da_set;
        du_set *= rhs.du_set;
        return *this;
    }

    //
    // Difference
    //
    inline DefinitePair operator-(const DefinitePair& rhs) const
    {
        return DefinitePair(*this) -= rhs;
    }
    inline DefinitePair& operator-=(const DefinitePair& rhs)
    {
        da_set -= rhs.da_set;
        du_set -= rhs.du_set;
        return *this;
    }
};


class DefiniteAssignmentSet
{
public:
    DefinitePair true_pair;
    DefinitePair false_pair;

    inline DefiniteAssignmentSet(unsigned set_size)
        : true_pair(set_size),
          false_pair(set_size)
    {}

    inline DefiniteAssignmentSet(DefinitePair& true_pair_,
                                 DefinitePair& false_pair_)
        : true_pair(true_pair_),
          false_pair(false_pair_)
    {}

    inline DefiniteAssignmentSet(DefinitePair& pair)
        : true_pair(pair),
          false_pair(pair)
    {}

    inline BitSet DASet() const
    {
        return true_pair.da_set * false_pair.da_set;
    }
    inline BitSet DUSet() const
    {
        return true_pair.du_set * false_pair.du_set;
    }
    inline DefinitePair Merge() const
    {
        return DefinitePair(DASet(), DUSet());
    }
    inline void AddElement(unsigned i)
    {
        true_pair.AddElement(i);
        false_pair.AddElement(i);
    }

    //
    // An assignment statement adds to da, but removes from du; reclaim it when
    // the variable leaves scope.
    //
    inline void AssignElement(unsigned i)
    {
        true_pair.AssignElement(i);
        false_pair.AssignElement(i);
    }

};


template<typename Key, typename Value>
void Map<Key, Value>::Rehash()
{
    Resize();

    for (unsigned i = 0; i < symbol_pool.Length(); i++)
    {
        Element* element = symbol_pool[i];
        unsigned k = element -> key -> HashCode() % hash_size;
        element -> next = base[k];
        base[k] = element;
    }
}


template<typename Key, typename Value>
void Map<Key, Value>::Add(Key* key, Value* value)
{
    assert(key);

    Element* element;
    unsigned k = key -> HashCode() % hash_size;
    for (element = base[k]; element; element = element -> next)
    {
        if (element -> key == key)
            break;
    }

    //
    // If this is a new element, add it to the map.
    //
    if (! element)
    {
        element = new Element();
        element -> key = key;
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
            hash_size < (unsigned) MAX_HASH_SIZE)
        {
            Rehash();
        }
    }
    else
    {
        assert(false &&
               "WARNING: Attempt to remap a key, unsupported operation !!!");
    }
    element -> value = value;
}


template<typename Key, typename Value>
Map<Key, Value>::Map(unsigned hash_size_)
{
    hash_size = (hash_size_ <= 0 ? 1 : hash_size_);
    base = NULL;
    Resize();
}


template<typename Key, typename Value>
void Map<Key, Value>::Resize()
{
    static int prime_index = -1;
    static const unsigned primes[] = {DEFAULT_HASH_SIZE, 101, 401,
                                      MAX_HASH_SIZE};
    while (hash_size >= primes[prime_index + 1] && hash_size < MAX_HASH_SIZE)
    {
        hash_size = primes[++prime_index];
    }
    delete [] base;
    base = new Element*[hash_size];
    memset(base, 0, hash_size * sizeof(Element*));
}



} // Close namespace Jopa block

#endif // set_INCLUDED

