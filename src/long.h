#ifndef long_INCLUDED
#define long_INCLUDED

#include "platform.h"


namespace Jikes { // Open namespace Jikes block
class IEEEdouble;
class IEEEfloat;

class LongInt;
class ULongInt;

class Int
{
public:
    // Max value of i4
    static inline const i4 MAX_INT()
    {
        return 0x7FFFFFFF;
    }

    // Min value of i4
    static inline const i4 MIN_INT()
    {
        return 0x80000000;
    }
};

class BaseLong
{
protected:
    enum
    {
        SHORT_MASK = 0xFFFF, // mask for lower half of i4
        SIGN_BIT = 0x80000000 // sign bit
    };

    union
    {
        double double_value;
#ifndef HAVE_64BIT_TYPES
        u4 word[2];
#else
        u8 words;
#endif // HAVE_64BIT_TYPES
    } value;

    // Set the high word only. Does not modify the low word!
    inline void setHighWord(u4 high)
    {
#ifndef HAVE_64BIT_TYPES
# ifndef WORDS_BIGENDIAN
        value.word[1] = high;
# else
        value.word[0] = high;
# endif
#else
        setHighAndLowWords(high, LowWord());
#endif // HAVE_64BIT_TYPES
    }

    // Set the low word only. Does not modify the high word!
    inline void setLowWord(u4 low)
    {
#ifndef HAVE_64BIT_TYPES
# ifndef WORDS_BIGENDIAN
        value.word[0] = low;
# else
        value.word[1] = low;
# endif
#else
        setHighAndLowWords(HighWord(), low);
#endif // HAVE_64BIT_TYPES
    }

    // Set the value for both words.
    inline void setHighAndLowWords(u4 high, u4 low)
    {
#ifndef HAVE_64BIT_TYPES
# ifndef WORDS_BIGENDIAN
        value.word[1] = high;
        value.word[0] = low;
# else
        value.word[0] = high;
        value.word[1] = low;
# endif
#else
        value.words = (((u8) high) << 32) | low;
#endif // HAVE_64BIT_TYPES
    }

    // Set the value for both words.
    inline void setHighAndLowWords(const BaseLong& op)
    {
#ifndef HAVE_64BIT_TYPES
        value.word[0] = op.value.word[0];
        value.word[1] = op.value.word[1];
#else
        value.words = op.value.words;
#endif // HAVE_64BIT_TYPES
    }

public:

#ifndef HAVE_64BIT_TYPES
# ifndef WORDS_BIGENDIAN
    inline u4 HighWord() const { return value.word[1]; }
    inline u4 LowWord() const { return value.word[0]; }
# else
    inline u4 HighWord() const { return value.word[0]; }
    inline u4 LowWord() const { return value.word[1]; }
# endif
#else
    inline u4 HighWord() const { return (u4) (value.words >> 32); }
    inline u4 LowWord() const { return (u4) value.words; }
    inline u8 Words() const { return value.words; }
#endif // HAVE_64BIT_TYPES

    double DoubleView() const { return value.double_value; }

    BaseLong(u4 high, u4 low);
    BaseLong(u4 a); // no sign extension
    BaseLong(i4 a); // sign extends
    // Forwarding constructor, if needed, so that BaseLong(0) works.
#ifndef TYPE_I4_IS_INT
    inline BaseLong(int i) { *this = BaseLong((i4) i); }
#endif // TYPE_I4_IS_INT
    inline BaseLong() {} // construct without initializing
#ifdef HAVE_64BIT_TYPES
    inline BaseLong(u8 a) { value.words = a; } // construct in one step
#endif // HAVE_64BIT_TYPES

    BaseLong operator+(const BaseLong) const; // binary addition
    BaseLong operator+() const;     // unary plus
    BaseLong& operator+=(const BaseLong); // add and assign
    BaseLong operator++(int);      // postfix increment
    BaseLong operator++();     // prefix increment

    BaseLong operator-(const BaseLong) const; // binary subtraction
    BaseLong operator-() const;     // unary minus
    BaseLong& operator-=(const BaseLong); // subtract and assign
    BaseLong operator--(int);      // postfix decrement
    BaseLong operator--();     // prefix decrement

    BaseLong operator*(const BaseLong) const; // multiplication
    BaseLong& operator*=(const BaseLong); // multiply and assign

    //
    // NOTE: To match the JLS, mask the argument with
    // Semantic::LONG_SHIFT_MASK (0x3f)
    //
    BaseLong operator<<(int) const;      // left shift
    BaseLong& operator<<=(int);      // left shift and assign

    bool operator==(const BaseLong) const; // equal
    bool operator!=(const BaseLong) const; // not equal
    bool operator!() const;     // logical complement

    BaseLong operator~() const;     // bitwise complement
    BaseLong operator^(const BaseLong) const; // bitwise XOR
    BaseLong& operator^=(const BaseLong); // bitwise XOR and assign
    BaseLong operator|(const BaseLong) const; // bitwise OR
    BaseLong& operator|=(const BaseLong); // bitwise OR and assign
    BaseLong operator&(const BaseLong) const; // bitwise AND
    BaseLong& operator&=(const BaseLong); // bitwise AND and assign

    bool operator&&(const BaseLong) const; // logical AND (not short-circuit)
    bool operator||(const BaseLong) const; // logical OR (not short circuit)

    static void Divide(const BaseLong&, const BaseLong&, BaseLong&, BaseLong&);

    operator LongInt() const;          // Cast to LongInt
    operator ULongInt() const;         // Cast to ULongInt

    // mirrors java.lang.Long, useful in hashing
    inline i4 hashCode() const { return HighWord() ^ LowWord(); }
};


class LongInt : public BaseLong
{
public:

    inline LongInt(u4 a, u4 b) : BaseLong(a, b) {}
    inline LongInt(u4 a) : BaseLong(a) {} // no sign extension
    inline LongInt(i4 a) : BaseLong(a) {} // sign extends
    // Forwarding constructor, if needed, so that LongInt(0) works.
#ifndef TYPE_I4_IS_INT
    inline LongInt(int i) { *this = LongInt((i4) i); }
#endif // TYPE_I4_IS_INT
#ifdef HAVE_EXPLICIT
    explicit
#endif
             inline LongInt(const BaseLong& a) : BaseLong(a) {}
    inline LongInt() : BaseLong() {} // uninitialized
    // narrowing conversion of double to long
#ifdef HAVE_EXPLICIT
    explicit
#endif
             LongInt(const IEEEdouble&);

#ifdef HAVE_EXPLICIT
    explicit
#endif
    LongInt(const IEEEfloat&); // narrowing conversion of float to long

#ifdef HAVE_64BIT_TYPES
    inline LongInt(u8 a) : BaseLong(a) {} // construct in one step
#endif // HAVE_64BIT_TYPES

    //
    // These constants are generated when first used.  The memory they
    // use can be reclaimed with ConstantCleanup().
    //
    static inline const LongInt MAX_LONG()
    {
        return max_long_const ? *max_long_const
            : *(max_long_const = new LongInt(0x7FFFFFFF, 0xFFFFFFFF));
    }
    static inline const LongInt MIN_LONG()
    {
        return min_long_const ? *min_long_const
            : *(min_long_const = new LongInt(0x80000000, 0x00000000));
    }
    static void ConstantCleanup()
    {
        // The explicit casts are there to workaround a MSVC
        // behaviour which doesn't allow an implicit cast
        // from const X* to void* for the delete operator.
        delete (LongInt*) max_long_const;
        delete (LongInt*) min_long_const;
    }


private:
    static const LongInt* max_long_const;
    static const LongInt* min_long_const;

public:
    LongInt operator/(const LongInt) const; // divide
    LongInt& operator/=(const LongInt); // divide and assign

    LongInt operator%(const LongInt) const; // modulus
    LongInt& operator%=(const LongInt); // modulus and assign

    //
    // NOTE: To match the JLS, mask the argument with
    // Semantic::LONG_SHIFT_MASK (0x3f)
    //
    LongInt operator>>(int) const;     // right shift
    LongInt& operator>>=(int);     // right shift and assign

    bool operator<(const LongInt) const; // less-than
    bool operator>(const LongInt) const; // greater-than
    bool operator<=(const LongInt) const; // less-than or equal
    bool operator>=(const LongInt) const; // greater-than or equal
};


class ULongInt : public BaseLong
{
public:

    inline ULongInt(u4 a, u4 b) : BaseLong(a, b) {}
    inline ULongInt(u4 a) : BaseLong(a) {} // no sign extension
    inline ULongInt(i4 a) : BaseLong(a) {} // sign extended
    // Forwarding constructor, if needed, so that ULongInt(0) works.
#ifndef TYPE_I4_IS_INT
    inline ULongInt(int i) { *this = ULongInt((i4) i); }
#endif // TYPE_I4_IS_INT
    inline ULongInt() : BaseLong() {} // uninitialized
#ifdef HAVE_64BIT_TYPES
    inline ULongInt(u8 a) : BaseLong(a) {} // construct in one step
#endif // HAVE_64BIT_TYPES

    ULongInt operator/(const ULongInt) const; // divide
    ULongInt& operator/=(const ULongInt); // divide and assign

    ULongInt operator%(const ULongInt) const; // modulus
    ULongInt& operator%=(const ULongInt); // modulus and assign

    //
    // NOTE: To match the JLS, mask the argument with
    // Semantic::LONG_SHIFT_MASK (0x3f)
    //
    ULongInt operator>>(int) const;      // right shift
    ULongInt& operator>>=(int);      // right shift and assign

    bool operator<(const ULongInt) const; // less-than
    bool operator>(const ULongInt) const; // greater-than
    bool operator<=(const ULongInt) const; // less-than or equal
    bool operator>=(const ULongInt) const; // greater-than or equal
};


} // Close namespace Jikes block

#endif // long_INCLUDED
