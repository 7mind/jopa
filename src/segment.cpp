#include "segment.h"


namespace Jopa { // Open namespace Jopa block
u2& PairSegment::Image(int target)
{
    if (array == NULL)
    {
        for (unsigned i = 0; i < top; i++)
        {
            if (list[i].target == target)
                return list[i].value;
        }

        if (top < (unsigned) LIST_LIMIT)
        {
            unsigned j = top++;
            list[j].target = target;
            list[j].value = 0;
            return list[j].value;
        }

        unsigned offset = ((unsigned) target) & MASK;
        array = (u2*) memset(new u2[BLKSIZE], 0, BLKSIZE * sizeof(u2));
        array -= offset;

        for (unsigned j = 0; j < top; j++)
            array[list[j].target] = list[j].value;
    }
    return array[target];
}


u2& Pair::operator[](const int target)
{
    unsigned k = ((unsigned) target) >> PairSegment::LOG_BLKSIZE;

    if (k >= base_size)
    {
        unsigned old_base_size = base_size;
        PairSegment** old_base = base;

        //
        // For the first allocation, assume that there won't be that many more.
        // If there are others add a much bigger margin.
        //
        base_size = k + (old_base_size == 0 ? 2 : 16);
        base = new PairSegment*[base_size];

        if (old_base != NULL)
        {
            memcpy(base, old_base, old_base_size * sizeof(PairSegment*));
            delete [] old_base;
        }

        memset(&base[old_base_size], 0,
               (base_size - old_base_size) * sizeof(PairSegment*));
    }
    if (! base[k])
        base[k] = segment_pool.AllocatePairSegment();
    return base[k] -> Image(target);
}


Pair& TripletSegment::Image(int target)
{
    if (array == NULL)
    {
        for (unsigned i = 0; i < top; i++)
        {
            if (list[i].target == target)
                return *list[i].value;
        }

        if (top < (unsigned) LIST_LIMIT)
        {
            unsigned j = top++;
            list[j].target = target;
            return *(list[j].value = segment_pool.AllocatePair());
        }

        unsigned offset = ((unsigned) target) & MASK;
        array = (Pair**) memset(new Pair*[BLKSIZE], 0,
                                BLKSIZE * sizeof(Pair*));
        array -= offset;

        for (unsigned j = 0; j < top; j++)
            array[list[j].target] = list[j].value;
    }
    return *(array[target] ? array[target]
             : array[target] = segment_pool.AllocatePair());
}


u2& Triplet::Image(const int target, const int target2)
{
    unsigned k = ((unsigned) target) >> TripletSegment::LOG_BLKSIZE;

    if (k >= base_size)
    {
        unsigned old_base_size = base_size;
        TripletSegment** old_base = base;

        base_size = k + 4;
        base = new TripletSegment*[base_size];

        if (old_base)
        {
            memcpy(base, old_base, old_base_size * sizeof(TripletSegment*));
            delete [] old_base;
        }
        memset(&base[old_base_size], 0,
               (base_size - old_base_size) * sizeof(TripletSegment *));
    }

    if (! base[k])
        base[k] = segment_pool.AllocateTripletSegment();
    return base[k] -> Image(target)[target2];
}


SegmentPool::SegmentPool()
    : triplet_segment_pool(1024),
      pair_segment_pool(4096),
      pair_pool(4096)
{}


SegmentPool::~SegmentPool()
{
    unsigned i;
    for (i = 0; i < triplet_segment_pool.Length(); i++)
        delete triplet_segment_pool[i];
    for (i = 0; i < pair_segment_pool.Length(); i++)
        delete pair_segment_pool[i];
    for (i = 0; i < pair_pool.Length(); i++)
        delete pair_pool[i];
}


} // Close namespace Jopa block

