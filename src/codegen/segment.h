#pragma once

#include "platform.h"
#include "tuple.h"


namespace Jopa { // Open namespace Jopa block
class SegmentPool;


//
// A portion of the implementation of Pair.
//
class PairSegment
{
public:
    static constexpr int LIST_LIMIT = 5;
    static constexpr int LOG_BLKSIZE = 8;
    static constexpr unsigned BLKSIZE = 1U << LOG_BLKSIZE;
    static constexpr unsigned MASK = ~(BLKSIZE - 1U);

private:
    struct TargetValuePair
    {
        int target;
        u2 value;
    };

    TargetValuePair list[LIST_LIMIT];
    unsigned top;
    u2* array;

public:
    PairSegment() : top(0), array(NULL) {}

    ~PairSegment()
    {
        if (array)
        {
            unsigned offset = ((unsigned) list[0].target) & MASK;
            delete [] (array + offset);
        }
    }

    //
    // Returns the modifiable value associated with the integer key.
    //
    u2& Image(int);
};


//
// A map of int -> u2 pairs.
//
class Pair
{
public:
    Pair(SegmentPool& segment_pool_, unsigned estimate = 0)
        : segment_pool(segment_pool_)
    {
        //
        // DO NOT PERFORM THESE INITIALIZATION IN THE METHOD DECLARATOR !!!
        // There appears to be a bug in the xlC compiler that causes base to
        // not be initialized properly !!!
        //
        base_size = estimate ? (estimate >> PairSegment::LOG_BLKSIZE) + 1 : 0;
        base = (PairSegment**) (estimate
                                ? memset(new PairSegment*[base_size], 0,
                                         base_size * sizeof(PairSegment*))
                                : NULL);
    }

    ~Pair() { delete [] base; }

    u2& operator[](const int);

private:
    SegmentPool& segment_pool;
    PairSegment** base;
    unsigned base_size;
};


//
// A portion of the implementation of Triplet.
//
class TripletSegment
{
public:
    static constexpr int LIST_LIMIT = 5;
    static constexpr int LOG_BLKSIZE = 8;
    static constexpr unsigned BLKSIZE = 1U << LOG_BLKSIZE;
    static constexpr unsigned MASK = ~(BLKSIZE - 1U);

private:
    struct TargetValuePair
    {
        int target;
        Pair* value;
    };

    SegmentPool& segment_pool;

    TargetValuePair list[LIST_LIMIT];
    unsigned top;
    Pair** array;

public:
    TripletSegment(SegmentPool& segment_pool_)
        : segment_pool(segment_pool_),
          top(0),
          array(NULL)
    {}

    ~TripletSegment()
    {
        if (array)
        {
            unsigned offset = ((unsigned) list[0].target) & MASK;
            delete [] (array + offset);
        }
    }

    Pair& Image(int);
};


//
// A map of (int, int) -> u2 triplets.
//
class Triplet
{
public:
    Triplet(SegmentPool& segment_pool_, unsigned estimate = 0)
        : segment_pool(segment_pool_)
    {
        //
        // DO NOT PERFORM THESE INITIALIZATION IN THE METHOD DECLARATOR !!!
        // There appears to be a bug in the xlC compiler that causes base to
        // not be initialized properly !!!
        //
        base_size = estimate
            ? (estimate >> TripletSegment::LOG_BLKSIZE) + 1 : 0;
        base = (TripletSegment**)
            (estimate ? memset(new TripletSegment*[base_size], 0,
                               base_size * sizeof(TripletSegment*))
             : NULL);
    }

    ~Triplet() { delete [] base; }

    u2& Image(const int, const int);

private:
    SegmentPool& segment_pool;
    TripletSegment** base;
    unsigned base_size;
};


//
// Manages the memory used by Pairs and Triplets.
//
class SegmentPool
{
    Tuple<TripletSegment*> triplet_segment_pool;
    Tuple<PairSegment*> pair_segment_pool;
    Tuple<Pair*> pair_pool;

public:
    SegmentPool();
    ~SegmentPool();

    Pair* AllocatePair(unsigned estimate = 0)
    {
        return pair_pool.Next() = new Pair(*this, estimate);
    }
    PairSegment* AllocatePairSegment()
    {
        return pair_segment_pool.Next() = new PairSegment();
    }
    TripletSegment* AllocateTripletSegment()
    {
        return triplet_segment_pool.Next() = new TripletSegment(*this);
    }
};


} // Close namespace Jopa block

