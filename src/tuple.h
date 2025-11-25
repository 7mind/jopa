#ifndef tuple_INCLUDED
#define tuple_INCLUDED

#include "jikesapi.h"


namespace Jopa { // Open namespace Jopa block
class OutputBuffer;

//
// This Tuple template class can be used to construct a dynamic array of
// arbitrary objects. The space for the array is allocated in blocks of size
// 2**LOG_BLKSIZE. In declaring a tuple the user may specify an estimate of
// how many elements he expects. Based on that estimate, suitable values will
// be calculated for log_blksize and base_increment. If these estimates are
// off, more space will be allocated when needed. A tuple is also designed to
// act efficiently as a stack of objects.
//
template <typename T>
class Tuple
{
protected:
    friend class OutputBuffer;

    enum { DEFAULT_LOG_BLKSIZE = 3, DEFAULT_BASE_INCREMENT = 4 };

    T** base; // array of segments
    unsigned base_size; // number of segments
    unsigned top; // current number of elements
    unsigned size; // maximum number of elements

    unsigned log_blksize; // log2(elements per segment)
    unsigned base_increment; // number of segments to add when growing

    inline unsigned Blksize() const { return 1U << log_blksize; }

    //
    // Allocate another block of storage for the dynamic array.
    //
    inline void AllocateMoreSpace()
    {
        //
        // The variable size always indicates the maximum number of elements
        // that has been allocated for the array. Initially, it is set to 0
        // to indicate that the array is empty. The pool of available
        // elements is divided into segments of size 2**log_blksize each.
        // Each segment is pointed to by a slot in the array base.
        //
        // By dividing size by the size of the segment we obtain the index
        // for the next segment in base. If base is full, it is reallocated.
        //
        unsigned k = size >> log_blksize; // which segment?

        //
        // If the base is overflowed, reallocate it and initialize the new
        // elements to NULL.
        //
        if (k == base_size)
        {
            unsigned old_base_size = base_size;
            T** old_base = base;

            base_size += base_increment;
            base = new T*[base_size];

            if (old_base)
            {
                memcpy(base, old_base, old_base_size * sizeof(T*));
                delete [] old_base;
            }
            memset(&base[old_base_size], 0,
                   (base_size - old_base_size) * sizeof(T*));
        }

        //
        // We allocate a new segment and place its adjusted address in
        // base[k]. The adjustment allows us to index the segment directly,
        // instead of having to perform a subtraction for each reference.
        // See operator[] below. Finally, we update size.
        //
        base[k] = (new T[Blksize()]) - size;
        size += Blksize();
    }

public:
    //
    // This function is invoked with an integer argument n. It ensures
    // that enough space is allocated for n elements in the dynamic array.
    // I.e., that the array will be indexable in the range (0..n-1).
    //
    // Note that this function can be used as a garbage collector.  When
    // invoked with no argument(or 0), it frees up all dynamic space that
    // was allocated for the array.
    //
    inline void Resize(const unsigned n = 0)
    {
        //
        // If array did not previously contain enough space, allocate
        // the necessary additional space. Otherwise, if the array had
        // more blocks than are needed, release the extra blocks.
        //
        if (n > size)
        {
            do
            {
                AllocateMoreSpace();
            } while (n > size);
        }
        else if (n < size)
        {
            // slot is the index of the base element whose block
            // will contain the (n-1)th element.
            int slot = (n == 0 ? -1 : (n - 1) >> log_blksize);

            for (int k = (size >> log_blksize) - 1; k > slot; k--)
            {
                size -= Blksize();
                delete [] (base[k] + size);
                base[k] = NULL;
            }

            if (slot < 0)
            {
                delete [] base;
                base = NULL;
                base_size = 0;
            }
        }
        top = n;
    }

    //
    // This function is used to reset the size of a dynamic array without
    // allocating or deallocting space. It may be invoked with an integer
    // argument n which indicates the new size or with no argument which
    // indicates that the size should be reset to 0.
    //
    inline void Reset(const unsigned n = 0)
    {
        assert(n <= size);
        top = n;
    }

    //
    // Return length of the dynamic array.
    //
    inline unsigned Length() const { return top; }

    //
    // Return a reference to the ith element of the dynamic array.
    //
    inline T& operator[](const unsigned i)
    {
        //
        // See the comments above. We purposefully pre-adjusted the pointers
        // of base[n]; if we hadn't, we would be doing the less efficient
        // base[i >> log_blocksize][i & ~(-1 << log_blocksize)].
        //
        assert(i < top);
        return base[i >> log_blksize][i];
    }
    inline const T& operator[](const unsigned i) const
    {
        assert(i < top);
        return base[i >> log_blksize][i];
    }

    //
    // Add an uninitialized element to the dynamic array and return its index.
    //
    inline unsigned NextIndex()
    {
        unsigned i = top++;
        if (i == size)
            AllocateMoreSpace();
        return i;
    }

    //
    // Push an element on the top of the dynamic array.
    //
    inline void Push(const T& elt) { this -> Next() = elt; }
    //
    // Return the top element of the dynamic array, removing it.
    //
    inline T Pop()
    {
        // Not "return (*this)[--top]" because that may violate an invariant
        // in operator[].
        assert(top);
        top--;
        return base[top >> log_blksize][top];
    }
    //
    // Return the top element of the dynamic array, leaving it in place.
    //
    inline T& Top()
    {
        assert(top);
        return (*this)[top - 1];
    }
    inline const T& Top() const
    {
        assert(top);
        return (*this)[top - 1];
    }

    //
    // Add an element to the dynamic array and return a reference to
    // that new element.
    //
    inline T& Next()
    {
        unsigned i = NextIndex();
        return base[i >> log_blksize][i];
    }

    //
    // Assignment of a dynamic array to another.
    //
    inline Tuple<T>& operator=(const Tuple<T>& rhs)
    {
        if (this != &rhs)
        {
            Resize(rhs.top);
            for (unsigned i = 0; i < rhs.top; i++)
                base[i >> log_blksize][i] = rhs.base[i >> log_blksize][i];
        }
        return *this;
    }

    //
    // Constructor of a Tuple, with an estimate of final size.
    //
    Tuple(const unsigned estimate = 0)
    {
        if (estimate == 0)
        {
            log_blksize = DEFAULT_LOG_BLKSIZE;
            base_increment = DEFAULT_BASE_INCREMENT;
        }
        else
        {
            for (log_blksize = 1;
                 (1U << log_blksize) < estimate && log_blksize < 31;
                 log_blksize++)
                ;
            if (log_blksize <= DEFAULT_LOG_BLKSIZE)
                base_increment = 1;
            else if (log_blksize < 13)
            {
                base_increment = 1U << (log_blksize - 4);
                log_blksize = 4;
            }
            else
            {
                base_increment = 1U << (log_blksize - 8);
                log_blksize = 8;
            }
            // Add a little margin to avoid reallocating the base.
            base_increment++;
        }

        base_size = 0;
        size = 0;
        top = 0;
        base = NULL;
    }

    //
    // Constructor of a Tuple, with specified segment size and base increment.
    // For example, Tuple(3, 4) will allocate memory every 2**3 (or 8)
    // elements, and will reallocate the base every 4 segments (or 32
    // elements). Fine tuning these parameters allows for more efficient
    // memory usage.
    //
    Tuple(const unsigned log_blksize_, const unsigned base_increment_)
        : log_blksize(log_blksize_),
          base_increment(base_increment_)
    {
        base_size = 0;
        size = 0;
        top = 0;
        base = NULL;
    }

    //
    // Initialization of a dynamic array from an existing one.
    //
    Tuple(const Tuple<T>& rhs)
        : log_blksize(rhs.log_blksize),
          base_increment(rhs.base_increment)
    {
        base_size = 0;
        size = 0;
        base = NULL;
        *this = rhs;
    }

    //
    // Destructor of a dynamic array.
    //
    virtual ~Tuple() { Resize(0); }

    //
    // Return the total size of temporary space allocated.
    //
    inline size_t SpaceAllocated() const
    {
        return base_size * sizeof(T*) + size * sizeof(T);
    }

    //
    // Return the total size of temporary space used.
    //
    inline size_t SpaceUsed() const
    {
        return (size >> log_blksize) * sizeof(T*) + top * sizeof(T);
    }
};


//
// This class is similar to Tuple, in that it is a template class used to
// construct an array of arbitrary objects. However, this class is designed
// to grow dynamically until all elements are inserted, then it is "frozen"
// by calling Array(). This moves all elements into continguous memory
// locations, allowing more efficient use of memory and traversal of
// elements. Also, a ConvertibleArray is not designed to perform stack
// operations the way Tuple can.
//
template <typename T>
class ConvertibleArray : protected Tuple<T>
{
public:
    //
    // Construct a dynamic array with an estimate for maximum size.
    //
    ConvertibleArray(const unsigned estimate = 0)
        : Tuple<T>(estimate),
          array(NULL)
    {}

    //
    // Constructor a dynamic array with specified segment size and base
    // increment. For example, ConvertibleArray(3, 4) will allocate memory
    // every 2**3 (or 8) elements, and will reallocate the base every 4
    // segments (or 32 elements). Fine tuning these parameters allows for
    // more efficient memory usage.
    //
    ConvertibleArray(const unsigned log_blksize, const unsigned base_increment)
        : Tuple<T>(log_blksize, base_increment),
          array(NULL)
    {}

    virtual ~ConvertibleArray() { delete [] array; }

    //
    // This function converts a tuple into a regular array and destroys the
    // original tuple. Once called, the tuple can no longer grow.
    //
    inline T* Array()
    {
        if (! array)
            Compact();
        return array;
    }
    inline const T* Array() const
    {
        if (! array)
        {
            //
            // We must discard the const qualifier of this; but it is safe
            // since we are not changing the contents (just the storage).
            //
#ifdef HAVE_CONST_CAST
            (const_cast<ConvertibleArray<T>*> (this)) -> Compact();
#else
            ((ConvertibleArray<T>*) this) -> Compact();
#endif // HAVE_CONST_CAST
        }
        return array;
    }

    //
    // Access an element of the array.
    //
    inline T& operator[](const unsigned i)
    {
        assert(i < Tuple<T>::top);
        return array ? array[i]
            : Tuple<T>::base[i >> Tuple<T>::log_blksize][i];
    }

    inline const T& operator[](const unsigned i) const
    {
        assert(i < Tuple<T>::top);
        return array ? array[i]
            : Tuple<T>::base[i >> Tuple<T>::log_blksize][i];
    }

    //
    // Returns the index of the next element, if this has not yet been
    // converted to an array.
    //
    inline unsigned NextIndex()
    {
        assert(! array);
        return Tuple<T>::NextIndex();
    }
    //
    // Returns the next element, if this has not yet been converted to
    // an array.
    //
    inline T& Next()
    {
        assert(! array);
        return Tuple<T>::Next();
    }

    //
    // These methods of Tuple work as is.
    //
    using Tuple<T>::Length;
    using Tuple<T>::SpaceUsed;

    inline size_t SpaceAllocated()
    {
        return array ? SpaceUsed() : Tuple<T>::SpaceAllocated();
    }

private:
    //
    // Compact the array into a single contiguous chunk of memory, prohibiting
    // further dynamic growth.
    //
    void Compact()
    {
        // Special case an empty ConvertibleArray, since some compilers have
        // problems with 0-length arrays.
        if (! Tuple<T>::top)
        {
            array = new T[1];
            Tuple<T>::Resize();
            return;
        }

        array = new T[Tuple<T>::top];

        unsigned i = 0;
        unsigned processed_size = 0;
        // the last non-empty slot!
        unsigned n = (Tuple<T>::top - 1) >> Tuple<T>::log_blksize;
        while (i < n)
        {
            memcpy(array + processed_size,
                   Tuple<T>::base[i] + processed_size,
                   Tuple<T>::Blksize() * sizeof(T));
            delete [] (Tuple<T>::base[i] + processed_size);
            i++;
            processed_size += Tuple<T>::Blksize();
        }
        memcpy(array + processed_size,
               Tuple<T>::base[n] + processed_size,
               (Tuple<T>::top - processed_size) * sizeof(T));
        delete [] (Tuple<T>::base[n] + processed_size);
        delete [] Tuple<T>::base;
        Tuple<T>::base = NULL;
        Tuple<T>::size = 0;
    }

    T* array;
};


//
// This class allows the building of an output file in memory, before writing
// it all at once.
//
class OutputBuffer
{
public:
    OutputBuffer(unsigned log_blksize = 13, unsigned base_increment = 128)
        : buffer(log_blksize, base_increment)
    {}

    inline void PutU1(u1 u) { buffer.Next() = u; }

    inline void PutU2(u2 u)
    {
        buffer.Next() = u >> 8;
        buffer.Next() = u & 0xff;
    }

    inline void PutU4(u4 u)
    {
        buffer.Next() = u >> 24;
        buffer.Next() = (u >> 16) & 0xff;
        buffer.Next() = (u >> 8)  & 0xff;
        buffer.Next() = u & 0xff;
    }

    inline void PutN(const u1* u, int n)
    {
        while (--n >= 0)
            buffer.Next() = *u++;
    }

    inline bool WriteToFile(const char* file_name)
    {
        JopaAPI::FileWriter* file =
            JopaAPI::getInstance() -> write(file_name, buffer.top);

        // NB if file was invalid it would have been destroyed by write()
        if (file == NULL)
            return false;

        unsigned size = 0;
        // the last non-empty slot!
        unsigned n = (buffer.top - 1) >> buffer.log_blksize;

        for (unsigned i = 0; i < n; i++)
        {
            file -> write(buffer.base[i] + size, buffer.Blksize());
            size += buffer.Blksize();
        }
        file -> write(buffer.base[n] + size, buffer.top - size);

        delete file;
        return true;
    }

private:
    Tuple<u1> buffer;
};


} // Close namespace Jopa block

#endif // tuple_INCLUDED

