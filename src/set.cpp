#include "set.h"
#include <cstdio>


namespace Jopa { // Open namespace Jopa block

SymbolSet::SymbolSet(unsigned hash_size_)
    : main_index(0),
      sub_index(0)
{
    (void) hash_size_;
}


SymbolSet::Bucket* SymbolSet::FindBucket(const NameSymbol* name_symbol)
{
    typename BucketMap::iterator it = buckets.find(name_symbol);
    return it == buckets.end() ? NULL : &it->second;
}


const SymbolSet::Bucket* SymbolSet::FindBucket(const NameSymbol* name_symbol) const
{
    typename BucketMap::const_iterator it = buckets.find(name_symbol);
    return it == buckets.end() ? NULL : &it->second;
}


SymbolSet::Bucket& SymbolSet::EnsureBucket(const NameSymbol* name_symbol)
{
    const std::pair<typename BucketMap::iterator, bool> inserted =
        buckets.try_emplace(name_symbol, Bucket());
    if (inserted.second)
        bucket_order.push_back(name_symbol);
    return inserted.first->second;
}


void SymbolSet::RemoveBucketIfEmpty(const NameSymbol* name_symbol)
{
    typename BucketMap::iterator it = buckets.find(name_symbol);
    if (it != buckets.end() && it->second.empty())
    {
        buckets.erase(it);
        typename std::vector<const NameSymbol*>::iterator order_it =
            std::find(bucket_order.begin(), bucket_order.end(), name_symbol);
        if (order_it != bucket_order.end())
        {
            const size_t erased_index =
                static_cast<size_t>(order_it - bucket_order.begin());
            bucket_order.erase(order_it);
            if (main_index >= erased_index)
            {
                main_index = 0;
                sub_index = 0;
            }
        }
    }
}


unsigned SymbolSet::Size() const
{
    unsigned size = 0;
    for (const auto& entry : buckets)
        size += static_cast<unsigned>(entry.second.size());
    return size;
}


void SymbolSet::SetEmpty()
{
    buckets.clear();
    bucket_order.clear();
    main_index = 0;
    sub_index = 0;
}


bool SymbolSet::IsEmpty() const
{
    return buckets.empty();
}


SymbolSet& SymbolSet::operator=(const SymbolSet& rhs)
{
    if (this != &rhs)
    {
        SetEmpty();
        Union(rhs);
    }
    return *this;
}


bool SymbolSet::operator==(const SymbolSet& rhs) const
{
    if (this == &rhs)
        return true;
    if (Size() != rhs.Size())
        return false;

    for (const auto& entry : buckets)
    {
        const Bucket* other_bucket = rhs.FindBucket(entry.first);
        if (! other_bucket || other_bucket->size() != entry.second.size())
            return false;
        for (Symbol* symbol : entry.second)
        {
            if (std::find(other_bucket->begin(), other_bucket->end(), symbol)
                == other_bucket->end())
            {
                return false;
            }
        }
    }
    return true;
}


void SymbolSet::Union(const SymbolSet& set)
{
    if (this == &set)
        return;

    for (const auto& entry : set.buckets)
        for (Symbol* symbol : entry.second)
            AddElement(symbol);
}


void SymbolSet::Intersection(const SymbolSet& set)
{
    if (this == &set)
        return;

    BucketMap new_buckets;
    std::vector<const NameSymbol*> new_order;

    for (const auto& entry : buckets)
    {
        const Bucket* other_bucket = set.FindBucket(entry.first);
        if (! other_bucket)
            continue;

        Bucket intersection;
        for (Symbol* symbol : entry.second)
        {
            if (std::find(other_bucket->begin(), other_bucket->end(), symbol)
                != other_bucket->end())
            {
                intersection.push_back(symbol);
            }
        }

        if (! intersection.empty())
        {
            new_order.push_back(entry.first);
            new_buckets.emplace(entry.first, std::move(intersection));
        }
    }

    buckets.swap(new_buckets);
    bucket_order.swap(new_order);
    main_index = 0;
    sub_index = 0;
}


bool SymbolSet::Intersects(const SymbolSet& set) const
{
    for (const auto& entry : set.buckets)
    {
        const Bucket* bucket = FindBucket(entry.first);
        if (! bucket)
            continue;

        for (Symbol* symbol : entry.second)
        {
            if (std::find(bucket->begin(), bucket->end(), symbol)
                != bucket->end())
            {
                return true;
            }
        }
    }
    return false;
}


unsigned SymbolSet::NameCount(const Symbol* element) const
{
    const NameSymbol* name_symbol = element -> Identity();
    const Bucket* bucket = FindBucket(name_symbol);
    return bucket ? static_cast<unsigned>(bucket->size()) : 0;
}


bool SymbolSet::IsElement(const Symbol* element) const
{
    assert(element);

    const NameSymbol* name_symbol = element -> Identity();
    const Bucket* bucket = FindBucket(name_symbol);
    if (! bucket)
        return false;

    return std::find(bucket->begin(), bucket->end(), element) != bucket->end();
}


void SymbolSet::AddElement(Symbol* element)
{
    assert(element && "AddElement called with NULL element");

    const NameSymbol* name_symbol = element -> Identity();
    assert(name_symbol && "element->Identity() returned NULL");

    Bucket& bucket = EnsureBucket(name_symbol);
    if (std::find(bucket.begin(), bucket.end(), element) == bucket.end())
        bucket.push_back(element);
}


void SymbolSet::RemoveElement(const Symbol* element)
{
    assert(element);

    const NameSymbol* name_symbol = element -> Identity();
    Bucket* bucket = FindBucket(name_symbol);
    if (! bucket)
        return;

    typename Bucket::iterator it =
        std::find(bucket->begin(), bucket->end(), element);
    if (it != bucket->end())
    {
        bucket->erase(it);
        RemoveBucketIfEmpty(name_symbol);
    }
}


Symbol* SymbolSet::FirstElement()
{
    main_index = 0;
    sub_index = 0;

    while (main_index < bucket_order.size())
    {
        const Bucket& bucket = buckets[bucket_order[main_index]];
        if (! bucket.empty())
            return bucket.front();
        main_index++;
    }
    return NULL;
}


Symbol* SymbolSet::NextElement()
{
    while (main_index < bucket_order.size())
    {
        const Bucket& bucket = buckets[bucket_order[main_index]];
        if (sub_index + 1 < bucket.size())
        {
            sub_index++;
            return bucket[sub_index];
        }

        main_index++;
        sub_index = 0;
        if (main_index < bucket_order.size())
        {
            const Bucket& next_bucket = buckets[bucket_order[main_index]];
            if (! next_bucket.empty())
                return next_bucket.front();
        }
    }
    return NULL;
}


Symbol* NameSymbolMap::Image(const NameSymbol* name_symbol)
{
    assert(name_symbol);

    const Bucket* bucket = FindBucket(name_symbol);
    return bucket && ! bucket->empty() ? (*bucket)[0] : NULL;
}


void NameSymbolMap::AddElement(Symbol* element)
{
    assert(element);

    const NameSymbol* name_symbol = element -> Identity();
    assert(name_symbol);

    Bucket& bucket = EnsureBucket(name_symbol);
    bucket.clear();
    bucket.push_back(element);
}


Symbol* SymbolMap::Image(Symbol* symbol)
{
    assert(symbol);

    std::unordered_map<Symbol*, Symbol*>::iterator it =
        mapping.find(symbol);
    return it != mapping.end() ? it->second : NULL;
}


void SymbolMap::Map(Symbol* symbol, Symbol* image)
{
    assert(symbol);

    const std::pair<std::unordered_map<Symbol*, Symbol*>::iterator, bool>
        inserted = mapping.emplace(symbol, image);
    if (! inserted.second)
    {
        fprintf(stderr, "WARNING: This should not have happened !!!");
        inserted.first->second = image;
    }
}


SymbolMap::SymbolMap(unsigned hash_size_)
{
    (void) hash_size_;
}


} // Close namespace Jopa block
