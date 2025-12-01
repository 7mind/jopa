#pragma once

#include "platform.h"
#include "lookup.h"
#include <algorithm>
#include <unordered_map>
#include <vector>


namespace Jopa { // Open namespace Jopa block
class SymbolSet
{
public:
    enum
    {
        DEFAULT_HASH_SIZE = 13,
        MAX_HASH_SIZE = 1021
    };

    explicit SymbolSet(unsigned hash_size_ = DEFAULT_HASH_SIZE);
    ~SymbolSet() = default;

    unsigned Size() const;
    void SetEmpty();
    bool IsEmpty() const;

    SymbolSet& operator=(const SymbolSet& rhs);

    bool operator==(const SymbolSet&) const;
    inline bool operator!=(const SymbolSet& rhs) const
    {
        return ! (*this == rhs);
    }

    void Union(const SymbolSet&);
    void Intersection(const SymbolSet&);
    bool Intersects(const SymbolSet&) const;
    unsigned NameCount(const Symbol* element) const;
    bool IsElement(const Symbol* element) const;
    void AddElement(Symbol* element);
    void RemoveElement(const Symbol*);

    Symbol* FirstElement();
    Symbol* NextElement();

protected:
    using Bucket = std::vector<Symbol*>;
    using BucketMap = std::unordered_map<const NameSymbol*, Bucket>;

    Bucket* FindBucket(const NameSymbol* name_symbol);
    const Bucket* FindBucket(const NameSymbol* name_symbol) const;
    Bucket& EnsureBucket(const NameSymbol* name_symbol);
    void RemoveBucketIfEmpty(const NameSymbol* name_symbol);

    BucketMap buckets;
    std::vector<const NameSymbol*> bucket_order;

    size_t main_index;
    size_t sub_index;
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
    Symbol* Image(const NameSymbol* name_symbol);

    //
    // Add element to the set in question if was not already there.
    //
    void AddElement(Symbol* element);
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

    explicit Map(unsigned hash_size_ = DEFAULT_HASH_SIZE);
    virtual ~Map() = default;


    //
    // Has key been mapped to an image, yet? If so, return the image.
    //
    Value* Image(Key* key);

    //
    // Map or remap key to a given image.
    //
    void Add(Key*, Value*);

    //
    // Delete all values in the map. Use this before destroying a Map
    // when the values are heap-allocated objects that need cleanup.
    //
    void DeleteValues()
    {
        for (auto& entry : mapping)
            delete entry.second;
    }

private:
    std::unordered_map<Key*, Value*> mapping;
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

    explicit SymbolMap(unsigned hash_size_ = DEFAULT_HASH_SIZE);
    ~SymbolMap() = default;

    //
    // Has symbol been mapped to an image, yet? If so, return the image.
    //
    Symbol* Image(Symbol* symbol);

    //
    // Map or remap symbol to a given image.
    //
    void Map(Symbol*, Symbol*);

private:
    std::unordered_map<Symbol*, Symbol*> mapping;
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
    // Check if all bits are set (universe).
    //
    bool IsUniverse() const
    {
        int last = ((int) set_size - 1) / cell_size;
        for (int i = 0; i < last; i++)
        {
            if (s[i] != ~((CELL) 0))
                return false;
        }
        if (set_size == 0)
            return true;
        CELL mask = (set_size % cell_size
                     ? ((CELL) 1 << (set_size % cell_size)) - (CELL) 1
                     : ~((CELL) 0));
        return (s[last] & mask) == mask;
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
Map<Key, Value>::Map(unsigned hash_size_)
{
    (void) hash_size_;
}


template<typename Key, typename Value>
Value* Map<Key, Value>::Image(Key* key)
{
    assert(key);

    typename std::unordered_map<Key*, Value*>::iterator it = mapping.find(key);
    return it != mapping.end() ? it->second : NULL;
}


template<typename Key, typename Value>
void Map<Key, Value>::Add(Key* key, Value* value)
{
    assert(key);

    const std::pair<typename std::unordered_map<Key*, Value*>::iterator, bool>
        inserted = mapping.emplace(key, value);
    if (! inserted.second)
    {
        assert(false &&
               "WARNING: Attempt to remap a key, unsupported operation !!!");
    }
}


} // Close namespace Jopa block
