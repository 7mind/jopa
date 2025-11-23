// NOTES: The IEEE 754 emulation code in double.h and double.cpp within
// Jikes are adapted from code written by Alan M. Webb of IBM's Hursley
// lab in porting the Sun JDK to System/390.
//
// In addition, the code for emulating the remainder operator, %, is
// adapted from e_fmod.c, part of fdlibm, the Freely Distributable Math
// Library mentioned in the documentation of java.lang.StrictMath.  The
// original library is available at http://netlib2.cs.utk.edu/fdlibm.
//
// The code from fdlibm is copyrighted, as follows:
// ====================================================
// Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
//
// Developed at SunSoft, a Sun Microsystems, Inc. business.
// Permission to use, copy, modify, and distribute this
// software is freely granted, provided that this notice
// is preserved.
// ====================================================
//
// Likewise, the code for accurate conversions between floating point
// and decimal strings, in double.h, double.cpp, platform.h, and
// platform.cpp, is adapted from dtoa.c.  The original code can be
// found at http://netlib2.cs.utk.edu/fp/dtoa.c.
//
// The code in dtoa.c is copyrighted as follows:
//****************************************************************
//*
//* The author of this software is David M. Gay.
//*
//* Copyright (c) 1991, 2000, 2001 by Lucent Technologies.
//*
//* Permission to use, copy, modify, and distribute this software for any
//* purpose without fee is hereby granted, provided that this entire notice
//* is included in all copies of any software which is or includes a copy
//* or modification of this software and in all copies of the supporting
//* documentation for such software.
//*
//* THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
//* WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR LUCENT MAKES ANY
//* REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
//* OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
//*
//***************************************************************/
//
//

#ifndef double_INCLUDED
#define double_INCLUDED

#include "platform.h"
#include "long.h"


namespace Jikes { // Open namespace Jikes block
class LongInt;
class IEEEdouble;
class BigInt;

class IEEEfloat
{
    //
    // If HAVE_IEEE754 is defined, this class is simply a wrapper for
    // compiler-supported IEEE operations.  If not, then this class
    // emulates IEEE 754 behavior, in accordance with the Java Language,
    // using only integer arithmetic.
    //
private:
    friend class FloatToString;

    union
    {
        float float_value;
        u4 word; // unsigned
        i4 iword; // signed
    } value;

    //
    // two enums, so that first is signed, and second may be unsigned
    //
    enum
    {
        FRACT_SIZE = 23,         // number of bits in mantissa
        BIAS = 127,              // exponent bias
        MAX_DEC_EXP = 38,        // maximum decimal exponent
        // maximum digits in precise decimal
        MAX_DIGITS = BIAS + FRACT_SIZE + 1 - MAX_DEC_EXP
    };
    enum
    {
        SIGN_BIT = 0x80000000, // sign bit mask
        EXP_BITS = 0x7F800000, // exponent bit mask
        FRACT_BITS = 0x007FFFFF, // mantissa bit mask
        ABS_BITS = EXP_BITS | FRACT_BITS, // absolute value
        MAX_FRACT = 0x01000000, // upper limit of mantissa
        MIN_FRACT = 0x00800000, // lower limit of normalized mantissa
        NEG_ZERO = 0x80000000, // -0.0
        POS_ZERO = 0x00000000, // 0.0
        NEG_INF = 0xFF800000, // -Inf
        POS_INF = 0x7F800000, // +Inf
        NAN_BITS = 0x7FC00000, // canonical NaN
        BYTE_MASK = 0x000000FF, // mask off least significant byte
        MIN_LONG_F = 0xDF000000, // bit pattern of (float)LongInt::MIN_LONG()
        MIN_INT_F = 0xCF000000 // bit pattern of (float)Int::MIN_INT()
    };

    inline u4 SignBit() const { return value.word & SIGN_BIT; }
    inline u4 ExpBits() const { return value.word & EXP_BITS; }
    inline u4 FractBits() const { return value.word & FRACT_BITS; }

    // takes the (possibly) unnormalized fraction with its corresponding
    // exponent and sign, and creates the normalized float
    static IEEEfloat Normalize(int, int, u4);
    // takes the float and splits it into a normalized fraction and exponent;
    // the exponent is returned, the parameter fraction is modified
    int SplitInto(u4& fraction) const;

    // returns the value of 1 ulp (unit in last place) for this float
    IEEEfloat Ulp() const;
    // returns the ratio between two floats represented in BigInts
    static IEEEfloat Ratio(const BigInt&, const BigInt&);
    // adjusts the value of this based on the ratio of the BigInts, and
    // in direction determined by the boolean
    bool Adjust(const BigInt&, const BigInt&, const bool);

#ifndef HAVE_MEMBER_CONSTANTS
    // VC++ can't cope with constant class members
    static IEEEfloat tens[]; // powers of 10 exactly represented in float
    static IEEEfloat bigtens[]; // powers of 10 by powers of 2
#else
    static const IEEEfloat tens[]; // powers of 10 exactly represented in float
    static const IEEEfloat bigtens[]; // powers of 10 by powers of 2
#endif

public:
    //
    // Information methods, for evaluating components of the float
    //
    inline u4 Word() const { return value.word; }

    inline int Sign() const { return value.word >> 31; }
    inline int Exponent() const
    {
        return ((value.word & EXP_BITS) >> FRACT_SIZE) - BIAS;
    }
    inline u4 Fraction() const
    {
        return (value.word & FRACT_BITS) | (ExpBits() ? (u4) MIN_FRACT : 0);
    }
    static inline int Bias() { return BIAS; }
    static inline int FractSize() { return FRACT_SIZE; }

    inline bool IsNaN() const { return (value.word & ABS_BITS) > EXP_BITS; }

    inline bool IsNegative() const { return (value.iword < 0) && ! IsNaN(); }
    inline bool IsPositive() const { return (value.iword >= 0) && ! IsNaN(); }

    inline bool IsNegativeZero() const { return value.word == NEG_ZERO; }
    inline bool IsPositiveZero() const { return value.word == POS_ZERO; }
    inline bool IsZero() const { return (value.word & ABS_BITS) == POS_ZERO; }

    inline bool IsNegativeInfinity() const { return value.word == NEG_INF; }
    inline bool IsPositiveInfinity() const { return value.word == POS_INF; }
    inline bool IsInfinite() const
    {
        return (value.word & ABS_BITS) == POS_INF;
    }


    //
    // Generation methods, for creating common constants.
    // TODO: These methods should be converted to return static const
    // fields, rather than generating a non-const every time.  However,
    // adding const support is a big undertaking.
    //
    // Note: the (u4) cast is necessary to prevent MSVC from promoting the
    // enum constants to integer, which causes an infinite loop.
    static inline const IEEEfloat NaN() { return IEEEfloat((u4) NAN_BITS); }
    static inline const IEEEfloat POSITIVE_INFINITY()
    {
        return IEEEfloat((u4) POS_INF);
    }
    static inline const IEEEfloat NEGATIVE_INFINITY()
    {
        return IEEEfloat((u4) NEG_INF);
    }
    static inline const IEEEfloat POSITIVE_ZERO()
    {
        return IEEEfloat((u4) POS_ZERO);
    }
    static inline const IEEEfloat NEGATIVE_ZERO()
    {
        return IEEEfloat((u4) NEG_ZERO);
    }

    //
    // Constructors
    //
    // Create a float from the given value
    inline IEEEfloat(float f) { value.float_value = f; }

    // Convert string to float, the float is NaN if check_invalid is true
    // and input is invalid by JLS
    IEEEfloat(const char*, bool check_invalid = false);
    // Widening conversion of int to float, may lose precision
    IEEEfloat(i4);
    // Forwarding constructor, if needed, so that IEEEfloat(0) works.
#ifndef TYPE_I4_IS_INT
    inline IEEEfloat(int i) { *this = IEEEfloat((i4) i); }
#endif // TYPE_I4_IS_INT
    // Widening conversion of long to float, may lose precision
    IEEEfloat(const LongInt&);
    // Narrowing conversion of double to float, may lose precision
#ifdef HAVE_EXPLICIT
    explicit
#endif
             IEEEfloat(const IEEEdouble&);
    // Create a float without initializing it
    inline IEEEfloat() {}

    // Load a specified bit pattern (contrast to IEEEfloat(i4))
    inline IEEEfloat(u4 bits) { value.word = bits; }


    //
    // Conversion routines
    //
    i4 IntValue() const;
    LongInt LongValue() const;
    inline float FloatView() const { return value.float_value; }
    inline IEEEdouble DoubleValue() const;


    //
    // Floating-point operations
    // TODO: add const reference versions
    //
    IEEEfloat operator+(const IEEEfloat) const;   // binary addition
    inline IEEEfloat operator+() { return *this; } // unary plus
    inline IEEEfloat& operator+=(const IEEEfloat op) // add and assign
    {
        return *this = *this + op;
    }
    inline IEEEfloat operator++() { return *this += 1; } // pre-increment
    inline IEEEfloat operator++(int) // post-increment
    {
        IEEEfloat result = *this;
        *this += 1;
        return result;
    }

    IEEEfloat operator-() const;        // unary minus
    inline IEEEfloat operator-(const IEEEfloat op) const // binary subtraction
    {
        return *this + (-op);
    }
    inline IEEEfloat& operator-=(const IEEEfloat op) // subtract and assign
    {
        return *this = *this - op;
    }
    inline IEEEfloat operator--() { return *this -= 1; } // pre-decrement
    inline IEEEfloat operator--(int) // post-decrement
    {
        IEEEfloat result = *this;
        *this -= 1;
        return result;
    }

    IEEEfloat operator*(const IEEEfloat) const;   // multiplication
    inline IEEEfloat& operator*=(const IEEEfloat op) // multiply and assign
    {
        return *this = *this * op;
    }

    IEEEfloat operator/(const IEEEfloat) const;   // divide
    inline IEEEfloat& operator/=(const IEEEfloat op) // divide and assign
    {
        return *this = *this / op;
    }

    IEEEfloat operator%(const IEEEfloat) const;   // modulus
    inline IEEEfloat& operator%=(const IEEEfloat op) // modulus and assign
    {
        return *this = *this % op;
    }

    //
    // Comparison operators.  Recall that NaN does not compare, and 0.0 == -0.0
    //
    bool operator==(const IEEEfloat) const; // equal
    bool operator!=(const IEEEfloat) const; // not equal
    bool operator<(const IEEEfloat) const; // less-than
    bool operator>(const IEEEfloat) const; // greater-than
    bool operator<=(const IEEEfloat) const; // less-than or equal
    bool operator>=(const IEEEfloat) const; // greater-than or equal

    //
    // Methods for hashing floats, behave like java.lang.Float counterparts:
    //  * -0.0f and 0.0f are different, with positive 0 comparing as greater
    //    than negative 0.
    //  * identical bit patterns of NaN are the same in equals, but not
    //    unique patterns
    //  * all bit patterns of NaN compare as greater than any other float,
    //    including positive infinity, but the same as any other NaN pattern
    //
    inline bool equals(const IEEEfloat op) const
    {
        return value.word == op.value.word;
    }
    inline i4 hashCode() const { return value.iword; }
    inline int compareTo(const IEEEfloat op) const
    {
        return IsNaN() ? 1
            : (IsZero() && op.IsZero()) ? op.Sign() - Sign()
            : (*this < op) ? -1 : *this > op;
    }
};


class IEEEdouble : public BaseLong
    //
    // If HAVE_IEEE754 is defined, this class is simply a wrapper for
    // compiler-supported IEEE operations.  If not, then this class
    // emulates IEEE 754 behavior, in accordance with the Java Language,
    // using only integer arithmetic.
    //
{
private:
    friend class DoubleToString;

    //
    // two enums, so that first is signed, and second may be unsigned
    //
    enum
    {
        FRACT_SIZE = 52,                 // number of bits in mantissa
        FRACT_SIZE_HI = FRACT_SIZE - 32, // mantissa bits in high word
        BIAS = 1023,                     // exponent bias
        MAX_DEC_EXP = 308,               // maximum decimal exponent
        // maximum digits in precise decimal
        MAX_DIGITS = BIAS + FRACT_SIZE + 1 - MAX_DEC_EXP
    };
    enum
    {
        EXP_BITS = 0x7FF00000, // exponent bit mask
        FRACT_BITS = 0x000FFFFF, // mantissa bit mask
        ABS_BITS = EXP_BITS | FRACT_BITS, // absolute value
        MAX_FRACT = 0x00200000, // upper limit of mantissa
        MIN_FRACT = 0x00100000, // lower limit of mantissa
        NEG_ZERO_HI = 0x80000000, // -0.0
        POS_ZERO_HI = 0x00000000, // 0.0
        NEG_INF_HI = 0xFFF00000, // -Inf
        POS_INF_HI = 0x7FF00000, // +Inf
        NAN_HI = 0x7FF80000, // canonical NaN
        ZERO_LO = 0x00000000 // low half of special values above
    };

    inline u4 SignBit() const { return HighWord() & SIGN_BIT; }
    inline u4 ExpBits() const { return HighWord() & EXP_BITS; }
    inline u4 FractBits() const { return HighWord() & FRACT_BITS; }

    // takes the (possibly) unnormalized fraction with its corresponding
    // exponent and sign, and creates the normalized double
    static IEEEdouble Normalize(int, int, ULongInt);
    // takes the double and splits it into a normalized fraction and exponent;
    // the exponent is returned, the parameter fraction is modified
    int SplitInto(BaseLong& fraction) const;

    // returns the value of 1 ulp (unit in last place) for this double
    IEEEdouble Ulp() const;
    // returns the ratio between two doubles represented in BigInts
    static IEEEdouble Ratio(const BigInt& a, const BigInt& b);


#ifndef HAVE_MEMBER_CONSTANTS
    // VC++ can't cope with constant class members
    static IEEEdouble tens[]; // powers of 10 exactly represented in float
    static IEEEdouble bigtens[]; // powers of 10 by powers of 2
#else
    static const IEEEdouble tens[]; // powers of 10 exactly represented in float
    static const IEEEdouble bigtens[]; // powers of 10 by powers of 2
#endif

public:
    //
    // Information methods, for evaluating components of the float
    //
    inline int Sign() const { return HighWord() >> 31; }
    inline int Exponent() const
    {
        return ((HighWord() & EXP_BITS) >> (FRACT_SIZE_HI)) - BIAS;
    }
    inline LongInt Fraction() const
    {
        return LongInt(((HighWord() & FRACT_BITS) |
                        (ExpBits() ? (u4) MIN_FRACT : 0)),
                       LowWord());
    }
    static inline int Bias() { return BIAS; }
    static inline int FractSize() { return FRACT_SIZE; }

    inline bool IsNaN() const
    {
        // optimized for no branching, idea from fdlibm.c
        u4 high = HighWord();
        u4 low = LowWord();
        return ((high & ABS_BITS) | ((low | -(i4) low) >> 31)) > EXP_BITS;
    }

    inline bool IsNegative() const
    {
        return ((i4) HighWord() < 0) && ! IsNaN();
    }
    inline bool IsPositive() const
    {
        return ((i4) HighWord() >= 0) && ! IsNaN();
    }

    inline bool IsNegativeZero() const
    {
        return (HighWord() == NEG_ZERO_HI) && (LowWord() == ZERO_LO);
    }
    inline bool IsPositiveZero() const
    {
        return (HighWord() == POS_ZERO_HI) && (LowWord() == ZERO_LO);
    }
    inline bool IsZero() const
    {
        return ((HighWord() & ABS_BITS) == POS_ZERO_HI) &&
            (LowWord() == ZERO_LO);
    }

    inline bool IsNegativeInfinity() const
    {
        return (HighWord() == NEG_INF_HI) && (LowWord() == ZERO_LO);
    }
    inline bool IsPositiveInfinity() const
    {
        return (HighWord() == POS_INF_HI) && (LowWord() == ZERO_LO);
    }
    inline bool IsInfinite() const
    {
        return ((HighWord() & ABS_BITS) == POS_INF_HI) &&
            (LowWord() == ZERO_LO);
    }


    //
    // Generation methods, for creating common constants
    // TODO: These methods should be converted to return static const
    // fields, rather than generating a non-const every time, as in
    // BaseLong (long.h).  However, adding const support is a big undertaking.
    //
    static inline const IEEEdouble NaN()
    {
        return IEEEdouble(NAN_HI, ZERO_LO);
    }
    static inline const IEEEdouble POSITIVE_INFINITY()
    {
        return IEEEdouble(POS_INF_HI, ZERO_LO);
    }
    static inline const IEEEdouble NEGATIVE_INFINITY()
    {
        return IEEEdouble(NEG_INF_HI, ZERO_LO);
    }
    static inline const IEEEdouble POSITIVE_ZERO()
    {
        return IEEEdouble(POS_ZERO_HI, ZERO_LO);
    }
    static inline const IEEEdouble NEGATIVE_ZERO()
    {
        return IEEEdouble(NEG_ZERO_HI, ZERO_LO);
    }


    //
    // Constructors
    //
    // Create a double from the given value
    inline IEEEdouble(double d) { value.double_value = d; }
    // Convert string to double, the double is NaN if check_invalid is true
    // and input is invalid by JLS
    IEEEdouble(const char*, bool check_invalid = false);

    // widening conversion of int to double, no information lost
    IEEEdouble(i4);
    // Forwarding constructor, if needed, so that IEEEdouble(0) works.
#ifndef TYPE_I4_IS_INT
    inline IEEEdouble(int i) { *this = IEEEdouble((i4) i); }
#endif // TYPE_I4_IS_INT
    // Widening conversion of long to double, may lose precision
    IEEEdouble(const LongInt&);
    // Widening conversion of float to double, no information lost
    IEEEdouble(const IEEEfloat&);
    // Create a double without initializing it
    inline IEEEdouble() {}
    // Load a specified bit pattern (contrast to IEEEfloat(LongInt))
    inline IEEEdouble(u4 hi, u4 lo) { setHighAndLowWords(hi, lo); }
    inline IEEEdouble(const BaseLong& bits) { setHighAndLowWords(bits); }

    //
    // Conversion routines
    //
    i4 IntValue() const;
    LongInt LongValue() const;
    inline IEEEfloat FloatValue() const { return IEEEfloat(*this); }


    //
    // Floating-point operations
    // TODO: add const reference versions
    //
    IEEEdouble operator+(const IEEEdouble) const;   // binary addition
    inline IEEEdouble operator+() const { return *this; } // unary plus
    inline IEEEdouble& operator+=(const IEEEdouble op) // add and assign
    {
        return *this = *this + op;
    }
    inline IEEEdouble operator++() { return *this += 1; } // pre-increment
    inline IEEEdouble operator++(int) // post-increment
    {
        IEEEdouble result = *this;
        *this += 1;
        return result;
    }

    IEEEdouble operator-() const;         // unary minus
    inline IEEEdouble operator-(const IEEEdouble op) const // binary subtract
    {
        return *this + (-op);
    }
    inline IEEEdouble& operator-=(const IEEEdouble op) // subtract and assign
    {
        return *this = *this - op;
    }
    inline IEEEdouble operator--() { return *this -= 1; } // pre-decrement
    inline IEEEdouble operator--(int) // post-decrement
    {
        IEEEdouble result = *this;
        *this -= 1;
        return result;
    }

    IEEEdouble operator*(const IEEEdouble) const;   // multiplication
    inline IEEEdouble& operator*=(const IEEEdouble op) // multiply and assign
    {
        return *this = *this * op;
    }

    IEEEdouble operator/(const IEEEdouble) const;   // divide
    inline IEEEdouble& operator/=(const IEEEdouble op) // divide and assign
    {
        return *this = *this / op;
    }

    IEEEdouble operator%(const IEEEdouble) const;   // modulus
    inline IEEEdouble& operator%=(const IEEEdouble op) // modulus and assign
    {
        return *this = *this % op;
    }

    //
    // Comparison operators.  Recall that NaN does not compare, and 0.0 == -0.0
    //
    bool operator==(const IEEEdouble) const; // equal
    bool operator!=(const IEEEdouble) const; // not equal
    bool operator<(const IEEEdouble) const; // less-than
    bool operator>(const IEEEdouble) const; // greater-than
    bool operator<=(const IEEEdouble) const; // less-than or equal
    bool operator>=(const IEEEdouble) const; // greater-than or equal

    //
    // Methods for hashing doubles, behave like java.lang.Double counterparts:
    //  * -0.0 and 0.0 are different, with positive 0 comparing as greater
    //    than negative 0.
    //  * identical bit patterns of NaN are the same in equals, but not
    //    unique patterns
    //  * all bit patterns of NaN compare as greater than any other float,
    //    including positive infinity, but the same as any other NaN pattern
    //
    inline bool equals(const IEEEdouble op) const
    {
        return (BaseLong) *this == (BaseLong) op;
    }
    inline i4 hashCode() const { return ((BaseLong) *this).hashCode(); }
    inline int compareTo(const IEEEdouble op) const
    {
        return IsNaN() ? 1
            : (IsZero() && op.IsZero()) ? op.Sign() - Sign()
            : (*this < op) ? -1 : *this > op;
    }
};


//
// Helper class for storing precise decimal values of floating point
// when converting to or from decimal strings.
//
class BigInt
{
public:
    // init with value, set exponent and bit count
    BigInt(const IEEEfloat&, int& exp, int& bitcnt);
    BigInt(const IEEEdouble&, int& exp, int& bitcnt);
    // init with int
    inline BigInt(int);
    // init with value of string; where before is the number of digits
    // before the '.', total is the number of digits to parse, start
    // is the previously parsed value of the first few digits, and
    // startsize gives the number of digits previously parsed
    BigInt(const char*, int before, int total, u4 start, int startsize);
    // copy constructor and assignment operator
    BigInt& operator=(const BigInt&);
    BigInt(const BigInt& op) { data = NULL; *this = op; }
    // destructor
    ~BigInt() { delete data; }

private:
    // resize data[] to be 1<<k elements
    inline void resize(int k);

    // return count of high-order 0's in x
    static int hi0bits(u4 x);
    // return count of low-order 0's in y, shift y so that
    // least significant 1 is at right
    static int lo0bits(u4& y);

public:
    // return number of leading 0's in most significant word of data
    inline int hi0bits() const { assert(data); return hi0bits(data[wds - 1]); }

    // return true if this is 0
    inline bool IsZero() const { assert(data); return ! (data[0] || wds > 1); }

    // return true if this is negative
    inline bool IsNegative() const { return neg; }
    // set whether this is negative
    inline void IsNegative(bool value) { neg = value; }

    // operators
    BigInt operator+(const unsigned op) const;
    inline BigInt& operator+=(const unsigned op) { return *this = *this + op; }
    inline BigInt& operator++() { return *this += 1; } // pre-increment
    BigInt operator-(const BigInt& op) const;
    BigInt operator*(const BigInt& op) const;
    BigInt operator*(unsigned op) const;
    inline BigInt& operator*=(const BigInt& op) { return *this = *this * op; }
    inline BigInt& operator*=(unsigned op) { return *this = *this * op; }
    BigInt operator<<(unsigned op) const;
    inline BigInt& operator<<=(unsigned op) { return *this = *this << op; }

    // equivalent to *this = *this * m + a, with less work
    BigInt& multadd(unsigned m, unsigned a);
    // return *this *= pow(5, k)
    BigInt& pow5mult(unsigned k);
    // return *this == b ? 0 : *this < b ? negative : positive;
    int compareTo(const BigInt& b) const;
    // tmp = *this % S; *this /= S; return tmp;
    int quorem(const BigInt& S);

    // converts to scaled native value, between 1 and 2.
    IEEEfloat FloatValue() const;
    IEEEdouble DoubleValue() const;

private:
    friend class IEEEfloat;
    friend class IEEEdouble;

    int k;      // log2 maxwds
    int maxwds; // size of data[]
    bool neg;   // true for negative
    int wds;    // current memory use of data[]
    u4* data;   // bit storage

#ifndef HAVE_MEMBER_CONSTANTS
    static u4 fives[];          // powers of 5 that fit in int
    static BigInt* bigfives[];  // bigger powers of 5, by powers of 2
#else /* HAVE_MEMBER_CONSTANTS */
    static const u4 fives[];
    static const BigInt* bigfives[];
#endif /* HAVE_MEMBER_CONSTANTS */

};


inline IEEEdouble IEEEfloat::DoubleValue() const
{
    return IEEEdouble(*this);
}

inline BigInt::BigInt(int i) : data(NULL)
{
    resize(0);
    data[0] = i;
    wds = 1;
    neg = i < 0;
}

inline void BigInt::resize(int k)
{
    maxwds = 1 << k;
    delete data;
    data = new u4[maxwds];
    this -> k = k;
    wds = 0;
    neg = false;
}


} // Close namespace Jikes block

#endif // double_INCLUDED

