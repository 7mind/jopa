//
// NOTES: The IEEE 754 emulation code in double.h and double.cpp within
// Jikes are adapted from code written by Alan M. Webb of IBM's Hursley
// lab in porting the Sun JDK to System/390.
//
//
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
//
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

#include "double.h"
#include "long.h"
#include "code.h"


namespace Jopa { // Open namespace Jopa block
#ifndef HAVE_MEMBER_CONSTANTS
// VC++ can't cope with constant class members
IEEEfloat IEEEfloat::tens[] = {
    1e0f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f, 1e6f,
    1e7f, 1e8f, 1e9f, 1e10f
};
IEEEfloat IEEEfloat::bigtens[] = {
    1e8f, 1e16f, 1e32f
};

IEEEdouble IEEEdouble::tens[] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7,
    1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
    1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
};
IEEEdouble IEEEdouble::bigtens[] = {
    1e16, 1e32, 1e64, 1e128, 1e256
};
#else
const IEEEfloat IEEEfloat::tens[] = {
    1e0f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f, 1e6f,
    1e7f, 1e8f, 1e9f, 1e10f
};
const IEEEfloat IEEEfloat::bigtens[] = {
    1e8f, 1e16f, 1e32f
};

const IEEEdouble IEEEdouble::tens[] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7,
    1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
    1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
};
const IEEEdouble IEEEdouble::bigtens[] = {
    1e16, 1e32, 1e64, 1e128, 1e256
};
#endif


IEEEfloat::IEEEfloat(i4 a)
{
#ifdef HAVE_IEEE754
    value.float_value = (float) a;
#else
    int sign = 0;

    if (a < 0)
    {
        a  = -a; // even works for MIN_INT!
        sign = 1;
    }
    *this = (a == 0) ? POSITIVE_ZERO()
        : Normalize(sign, FRACT_SIZE, (u4) a);
#endif // HAVE_IEEE754
}

IEEEfloat::IEEEfloat(const LongInt &a)
{
#ifdef HAVE_IEEE754
# ifdef HAVE_64BIT_TYPES
    value.float_value = (float)(i8) a.Words();
# else
    value.float_value = ((float)(i4) a.HighWord() * (float) 0x40000000 * 4.0f)
        + (float) a.LowWord();
# endif // HAVE_64BIT_TYPES
#else
    //
    // Unfortunately, we cannot recycle the LongInt.DoubleValue() method, since
    // in rare cases the double rounding puts us off by one bit.
    //
    int sign = 0;
    LongInt l = a;

    if (a < 0)
    {
        l = -a;
        sign = 1;
        if (l < 0) // special case MIN_LONG
        {
            value.word = MIN_LONG_F;
            return;
        }
    }
    if (l == 0)
        *this = POSITIVE_ZERO();
    else if (l.HighWord() == 0)
        *this = Normalize(sign, FRACT_SIZE, l.LowWord());
    else
    {
        int exponent = FRACT_SIZE - 1, sticky = 0;
        while (l.HighWord())
        {
            sticky |= (l.LowWord() & BYTE_MASK) ? 1 : 0;
            l >>= 8;
            exponent += 8;
        }
        u4 low = l.LowWord();
        if ((i4) low < 0)
        {
            sticky |= (low & BYTE_MASK) ? 1 : 0;
            low >>= 8;
            exponent += 8;
        }
        *this = Normalize(sign, exponent, low + low + sticky);
    }
#endif // HAVE_IEEE754
}

IEEEfloat::IEEEfloat(const IEEEdouble &d)
{
#ifdef HAVE_IEEE754
    value.float_value = (float) d.DoubleView();
#else
    // Either true zero, denormalized, or too small
    if (d.Exponent() < -BIAS - 30)
        *this = (d.IsPositive() ? POSITIVE_ZERO() : NEGATIVE_ZERO());
    else
    {
        if (d.IsPositiveInfinity())
            *this = POSITIVE_INFINITY();
        else if (d.IsNegativeInfinity())
            *this = NEGATIVE_INFINITY();
        else if (d.IsNaN())
            *this = NaN();
        else
        {
            //
            // A regular, normalized number - do work on the parts
            // Shift to 26th position, add implicit msb, rounding bits
            //
            LongInt fract = d.Fraction() << 5;
            u4 fraction = fract.HighWord() | (fract.LowWord() ? 0x02000001
                                                              : 0x02000000);

            *this = Normalize(d.Sign(), d.Exponent() - 2, fraction);
        }
    }
#endif // HAVE_IEEE754
}

bool IEEEfloat::Adjust(const BigInt &delta, const BigInt &bs, const bool dsign)
{
    IEEEfloat aadj, aadj1;
    i4 y;

    aadj = Ratio(delta, bs);
    if (aadj <= 2)
    {
        if (dsign)
            aadj = aadj1 = 1;
        else if (FractBits())
        {
            if (value.word == 1)
            {
                // underflow
                *this = POSITIVE_ZERO();
                return true;
            }
            aadj = 1;
            aadj1 = -1;
        }
        else
        {
            //
            // special case - mantissa is power of 2
            //
            if (aadj < 1.0f)
                aadj = 0.5f;
            else
                aadj *= 0.5f;
            aadj1 = -aadj;
        }
    }
    else
    {
        aadj *= 0.5f;
        aadj1 = dsign ? aadj : -aadj;
    }
    y = Exponent();
    //
    // Check for overflow
    //
    if (y == BIAS)
    {
        IEEEfloat tmp(*this);
        value.word -= (FRACT_SIZE + 1) * MIN_FRACT;
        *this += aadj1 * Ulp();
        if (Exponent() >= BIAS - FRACT_SIZE)
        {
            if (tmp.value.word == POS_INF - 1)
            {
                // overflow
                *this = POSITIVE_INFINITY();
                return true;
            }
            value.word = POS_INF - 1;
            return false;
        }
        else
            value.word += (FRACT_SIZE + 1) * MIN_FRACT;
    }
    else
    {
        //
        // Compute adj so that the IEEE rounding rules will
        // correctly round *this + adj in some half-way cases.
        // If *this * Ulp() is denormalized, we must adjust aadj
        // to avoid trouble from bits lost to denormalization.
        //
        if (y <= FRACT_SIZE - BIAS || aadj > 1)
        {
            aadj1 = IEEEfloat((aadj + 0.5f).IntValue());
            if (! dsign)
                aadj1 = -aadj1;
        }
        *this += aadj1 * Ulp();
    }
    if (y == Exponent())
    {
        //
        // Can we stop now?
        // The tolerances below are conservative.
        //
        aadj -= aadj.IntValue();
        if (dsign || FractBits())
        {
            if (aadj < .4999999f || aadj > .5000001f)
                return true;
        }
        else if (aadj < .4999999f / 2)
            return true;
    }
    return false;
}

IEEEfloat::IEEEfloat(const char *str, bool check_invalid)
{
    int bb2, bb5, bbe, bd2, bd5, bbbits, bs2,
        e, e1, i, j, k;
    int nd, // number of digits in mantissa (except extra '0's)
        nd0, // number of digits before '.' (except leading '0's)
        nf, // number of digits after '.' (except trailing '0's)
        nz; // number of consecutive '0' digits
    bool nz0, // whether leading zero exists
         roundup, // whether to round up long string
         sign, // if the string represents a negative
         dsign, // the sign of delta
         esign; // the sign of the exponent
    char c;
    const char *s, *s0, *s1;
    i4 L;
    i4 y, z;

    sign = nz0 = roundup = false;
    nz = 0;

    //
    // consume whitespace
    //
    for (s = str; ; s++)
    {
        switch (*s)
        {
        case U_MINUS:
            sign = true;
            // fallthrough
        case U_PLUS:
            if (*++s)
                goto break2;
            // fallthrough
        case U_NU:
            *this = NaN();
            return;
        case U_SP:
        case U_HT:
        case U_FF:
        case U_LF:
        case U_CR:
            continue;
        default:
            goto break2;
        }
    }
 break2:
    //
    // Consume leading zeroes.
    //
    if (*s == U_0)
    {
        nz0 = true;
        while (*++s == U_0);

        if (! *s)
        {
            *this = sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
            return;
        }
        //
        // Parse hexadecimal floating point.
        //
        else if (*s == U_x || *s == U_X)
        {
            i4 fraction = 0;
            // Exponent adjustment, based on '.' location.
            int shift = FRACT_SIZE;
            bool seen_dot = false;
            while (*++s && *s == U_0); // Consume leading zeroes.
            c = *s;
            if (c == U_DOT)
            {
                seen_dot = true;
                while (*++s && *s == U_0)
                    shift -= 4;
                c = *s;
            }
            if (! c || c == U_p || c == U_P)
            {
                *this = sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
                return;
            }
            // To avoid overflow, stop after enough bits have been read.
            for (i = 0; i < (FRACT_SIZE >> 2) + 2 && c; i++, c = *++s)
            {
                if (c == U_DOT)
                {
                    if (seen_dot)
                        break;
                    c = *++s;
                    seen_dot = true;
                }
                int value;
                if (Code::IsHexDigit(c))
                    value = Code::Value(c);
                else break;
                if (seen_dot)
                    shift -= 4;
                fraction = (fraction << 4) + value;
            }
            // Round any remaining bits.
            bool sticky = false;
            while (c == U_DOT || Code::IsHexDigit(c))
            {
                if (c == U_DOT)
                {
                    if (seen_dot)
                        break;
                    seen_dot = true;
                }
                else
                {
                    if (! seen_dot)
                        shift += 4;
                    if (c != U_0)
                        sticky = true;
                }
                c = *++s;
            }
            assert(fraction != 0);
            if (sticky)
                fraction++;
            // On to the expononet.
            int exponent = 0;
            esign = false;
            if (c == U_p || c == U_P)
            {
                if (*++s == U_MINUS)
                {
                    esign = true;
                    s++;
                }
                else if (*s == U_PLUS)
                    s++;
                while ((c = *s++))
                {
                    if (! Code::IsDecimalDigit(c))
                        break;
                    exponent = exponent * 10 + c - U_0;
                    // Check for exponent overflow
                    if (exponent + shift > 19999)
                    {
                        if (check_invalid)
                            *this = NaN();
                        else
                        {
                            *this = esign ? POSITIVE_ZERO()
                                : POSITIVE_INFINITY();
                            if (sign)
                                *this = - *this;
                        }
                        return;
                    }
                }
            }
            if (esign)
                exponent = - exponent;
            *this = Normalize(sign, exponent + shift, fraction);
            if (check_invalid && (IsZero() || IsInfinite()))
                *this = NaN();
            return;
        }
    }

    //
    // parse before '.'
    //
    s0 = s;
    y = z = 0;
    for (nd = nf = 0; Code::IsDecimalDigit(c = *s); nd++, s++)
        if (nd < 8)
            y = 10 * y + c - U_0;
    nd0 = nd;
    if (c == U_DOT)
    {
        //
        // parse after '.'
        //
        c = *++s;
        if (!nd)
        {
            for ( ; c == U_0; c = *++s)
                nz++;
            if (c > U_0 && c <= U_9)
            {
                s0 = s;
                nf += nz;
                nz = 0;
            }
        }
        for ( ; Code::IsDecimalDigit(c); c = *++s)
        {
            nz++;
            if (c -= U_0)
            {
                nf += nz;
                for (i = 1; i < nz; i++)
                    if (nd++ < 8)
                        y *= 10;
                if (nd++ < 8)
                    y = 10 * y + c;
                nz = 0;
            }
        }
    }
    //
    // consume exponent
    //
    e = 0;
    if (c == U_e || c == U_E) {
        str = s;
        esign = false;
        switch (c = *++s)
        {
        case U_MINUS:
            esign = true;
            // fallthrough
        case U_PLUS:
            c = *++s;
        }
        if (Code::IsDecimalDigit(c))
        {
            if (!nd && !nz && !nz0)
            {
                *this = sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
                return;
            }
            while (c == U_0)
                c = *++s;
            if (c > U_0 && c <= U_9)
            {
                L = c - U_0;
                s1 = s;
                while (Code::IsDecimalDigit(c = *++s))
                    L = 10 * L + c - U_0;
                //
                // Avoid confusion from exponents so large that e might
                // overflow
                if (s - s1 > 8 || L > 19999)
                    e = 19999;
                else
                    e = L;
                if (esign)
                    e = -e;
            }
            else
                e = 0;
        }
        else
            s = str;
    }
    if (!nd) {
        *this = (!nz && !nz0) ? NaN()
                              : sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
        return;
    }
    //
    // for long strings, round away all digits beyond maximum precise string
    // for fraction, there are n decimal digits after '.' if lsb is 2^n;
    // but the first m of these digits are '0', for d = x*10^m
    // so, digits after MAX_DIGITS may be ignored
    //
    if (nd > MAX_DIGITS)
    {
        k = nd - MAX_DIGITS;
        i = MAX_DIGITS - nd0;
        if (i <= 0)
        {
            // decimal after last precise digit
            nd0 = MAX_DIGITS;
            nf = i;
            j = 0;
        }
        else if (i == MAX_DIGITS)
        {
            // decimal before first precise digit
            nf -= k;
            j = 0;
        }
        else
        {
            // decimal inside precise digits
            nf -= k;
            j = 1;
        }
        roundup = s0[MAX_DIGITS - 1 + j] != U_4;
        nd = MAX_DIGITS;
    }
    e1 = e -= nf;

    //
    // Now we have nd0 digits, starting at s0, followed by a
    // decimal point, followed by nd-nd0 digits.  The number we're
    // after is the integer represented by those digits times 10**e
    //
    if (! nd0)
        nd0 = nd;
    k = nd < 8 ? nd : 8;
    *this = IEEEfloat(y);
    if (nd < 8)
    {
        if (!e)
        {
            if (sign)
                *this = -*this;
            return;
        }
        if (e > 0)
        {
            if (e <= 10)
            {
                *this *= tens[e];
                if (sign)
                    *this = -*this;
                return;
            }
            i = 7 - nd;
            if (e <= 10 + i)
            {
                e -= i;
                *this *= tens[i];
                *this *= tens[e];
                if (sign)
                    *this = -*this;
                return;
            }
        }
        else if (e >= -10 )
        {
            *this /= tens[-e];
            if (sign)
                *this = -*this;
            return;
        }
    }
    e1 += nd - k;

    //
    // Get starting approximation: *this * 10**e1
    //
    if (e1 > 0)
    {
        i = e1 & 7;
        if (i)
            *this *= tens[i];
        if (e1 >>= 3)
        {
            if (e1 > MAX_DEC_EXP >> 3)
            {
                *this = check_invalid ? NaN()
                                      : sign ? NEGATIVE_INFINITY()
                                             : POSITIVE_INFINITY();
                return;
            }
            for (j = 0; e1 > 1; j++, e1 >>= 1)
                if (e1 & 1)
                    *this *= bigtens[j];
            //
            // The last multiplication could overflow.
            //
            value.word -= (FRACT_SIZE + 1) * MIN_FRACT;
            *this *= bigtens[j];
            z = Exponent();
            if (z > BIAS - FRACT_SIZE)
            {
                *this = check_invalid ? NaN()
                                      : sign ? NEGATIVE_INFINITY()
                                             : POSITIVE_INFINITY();
                return;
            }
            if (z > BIAS - FRACT_SIZE - 1)
                value.word = POS_INF - 1;
            else
                value.word += (FRACT_SIZE + 1) * MIN_FRACT;
        }
    }
    else if (e1 < 0)
    {
        e1 = -e1;
        i = e1 & 7;
        if (i)
            *this /= tens[i];
        if (e1 >>= 3)
        {
            if (e1 >= 1 << 3)
            {
                *this = check_invalid ? NaN()
                                      : sign ? NEGATIVE_ZERO()
                                             : POSITIVE_ZERO();
                return;
            }
            for (j = 0; e1 > 1; j++, e1 >>= 1)
                if (e1 & 1)
                    *this /= bigtens[j];
            //
            // The last multiplication could underflow.
            //
            IEEEfloat tmp(*this);
            *this /= bigtens[j];
            if (IsZero())
            {
                *this = tmp * 2;
                *this /= bigtens[j];
                if (IsZero())
                {
                    *this = check_invalid ? NaN()
                                          : sign ? NEGATIVE_ZERO()
                                                 : POSITIVE_ZERO();
                    return;
                }
                value.word = 1;
            }
        }
    }

    //
    // Now the hard part -- adjusting *this to the correct value.
    // Put digits into bd: true value = bd * 10^e
    //
    BigInt bd0(s0, nd0, nd, y, 8);
    if (roundup)
        ++bd0;
    while (true) {
        BigInt bd(bd0);
        BigInt bb(*this, bbe, bbbits); // *this = bb * 2^bbe
        BigInt bs(1);
        if (e >= 0)
        {
            bb2 = bb5 = 0;
            bd2 = bd5 = e;
        }
        else
        {
            bb2 = bb5 = -e;
            bd2 = bd5 = 0;
        }
        if (bbe >= 0)
            bb2 += bbe;
        else
            bd2 -= bbe;
        bs2 = bb2;
        j = bbe;
        i = j + bbbits - 1; // logb(*this)
        if (i < 1 - BIAS) // denormal
            j += BIAS + FRACT_SIZE;
        else
            j = FRACT_SIZE + 2 - bbbits;
        bb2 += j;
        bd2 += j;
        i = bb2 < bd2 ? bb2 : bd2;
        if (i > bs2)
            i = bs2;
        if (i > 0)
        {
            bb2 -= i;
            bd2 -= i;
            bs2 -= i;
        }
        if (bb5 > 0) {
            bs.pow5mult(bb5);
            bb *= bs;
        }
        if (bb2 > 0)
            bb <<= bb2;
        if (bd5 > 0)
            bd.pow5mult(bd5);
        if (bd2 > 0)
            bd <<= bd2;
        if (bs2 > 0)
            bs <<= bs2;
        BigInt delta = bb - bd;
        dsign = delta.IsNegative();
        if (dsign)
            delta.IsNegative(false);
        i = delta.compareTo(bs);
        //
        // Error is less than half an ulp -- check for
        // special case of mantissa a power of two.
        //
        if (i < 0)
        {
            if (dsign || FractBits() || Exponent() <= 1 - BIAS
                || delta.IsZero())
                break;
            delta <<= 1;
            if (delta.compareTo(bs) > 0)
                // boundary case -- decrement exponent
                value.word--;
            break;
        }
        //
        // exactly half-way between
        //
        else if (i == 0)
        {
            if (value.word & 1)
                value.word += dsign ? 1 : -1;
            break;
        }
        //
        // more than 1/2 ulp off - try again
        //
        // This is broken into a separate method because mingw gcc 2.95.2
        // has an ICE caused by register over-allocation if it is inline.
        //
        if (Adjust(delta, bs, dsign))
            break;
    }
    if (check_invalid && (IsZero() || IsInfinite()))
        *this = NaN();
    else if (sign)
        *this = -*this;
}

i4 IEEEfloat::IntValue() const
{
    if (IsNaN())
        return 0;

    int sign = Sign(),
        exponent = Exponent();

    if (exponent > 30)
        return sign ? Int::MIN_INT() : Int::MAX_INT();

    // This covers true zero and denorms.
    if (exponent < 0)
        return 0;

    i4 result = Fraction();

    if (exponent > FRACT_SIZE)
        result <<= (exponent - FRACT_SIZE);
    else if (exponent < FRACT_SIZE)
        result >>= (FRACT_SIZE - exponent);

    return sign ? -result : result;
}

LongInt IEEEfloat::LongValue() const
{
    if (IsNaN())
        return LongInt(0);

    int sign = Sign(),
        exponent = Exponent();

    if (exponent > 62)
        return sign ? LongInt::MIN_LONG() : LongInt::MAX_LONG();

    // This covers true zero and denorms.
    if (exponent < 0)
        return LongInt(0);

    LongInt result(Fraction());

    if (exponent > FRACT_SIZE)
        result <<= (exponent - FRACT_SIZE);
    else if (exponent < FRACT_SIZE)
        result >>= (FRACT_SIZE - exponent);

    return sign ? (LongInt) -result : result;
}

IEEEfloat IEEEfloat::Normalize(int sign, int exponent, u4 fraction)
{
    bool round = false, sticky = false;

    assert(fraction != 0);

    //
    // Normalize right. Shift until value < MAX_FRACT.
    //
    if (fraction >= MAX_FRACT)
    {
        while (fraction >= MAX_FRACT)
        {
            sticky |= round;
            round = (fraction & 1) != 0;
            fraction >>= 1;
            exponent++;
        }
        if (round && (sticky || (fraction & 1)) && exponent > -BIAS)
            //
            // Capture any overflow caused by rounding. No other checks are
            // required because if overflow occurred, the the low order bit
            // was guaranteed to be zero. Do not round denorms yet.
            //
            if (++fraction >= MAX_FRACT)
            {
                fraction >>= 1;
                exponent++;
            }
    }

    //
    // Normalize left.  Shift until value >= MIN_FRACT.
    //
    else
        while (fraction < MIN_FRACT)
        {
            fraction <<= 1;
            exponent--;
        }

    //
    // Check and respond to overflow
    //
    if (exponent > BIAS)
        return sign ? NEGATIVE_INFINITY() : POSITIVE_INFINITY();

    //
    // Check and respond to underflow
    //
    if (exponent <= -BIAS)
    {
        if (exponent < -BIAS - FRACT_SIZE)
            return sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
        while (exponent <= -BIAS)
        {
            sticky |= round;
            round = (fraction & 1) != 0;
            fraction >>= 1;
            exponent++;
        }
        if (round && (sticky || (fraction & 1)))
            fraction++;
        exponent = -BIAS;
        if (fraction == 0)
            return sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
    }

    fraction &= FRACT_BITS;
    fraction |= ((exponent + BIAS) << FRACT_SIZE);
    if (sign)
        fraction |= SIGN_BIT;
    return IEEEfloat(fraction);
}

int IEEEfloat::SplitInto(u4 &fraction) const
{
    int exponent = Exponent();
    fraction = Fraction();

    if (exponent == -BIAS)
    {
        exponent++;
        while (fraction < MIN_FRACT)
        {
            fraction <<= 1;
            exponent--;
        }
    }

    return exponent;
}

IEEEfloat IEEEfloat::Ulp() const
{
    i4 L;
    IEEEfloat f;
    f.value.float_value = value.float_value;

    L = (i4) f.ExpBits() - FRACT_SIZE * MIN_FRACT;
    if (L > 0)
        f.value.iword = L;
    else
    {
        L = -L >> FRACT_SIZE;
        f.value.iword = L >= (i4) FRACT_SIZE ? 1 : 0x400000 >> L;
    }
    return f;
}

IEEEfloat IEEEfloat::Ratio(const BigInt &a, const BigInt &b)
{
    IEEEfloat fa, fb;
    int k;
    fa = a.FloatValue();
    fb = b.FloatValue();
    k = b.hi0bits() - a.hi0bits() + 32 * (a.wds - b.wds);
    if (k > 0)
        fa.value.word += k * MIN_FRACT;
    else
        fb.value.word -= k * MIN_FRACT;
    return fa / fb;
}

bool IEEEfloat::operator== (const IEEEfloat op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return value.float_value == op.value.float_value;
#else
    return (IsNaN() || op.IsNaN() ? false
            : IsZero() && op.IsZero() ? true
            : value.word == op.value.word);
#endif
}

bool IEEEfloat::operator!= (const IEEEfloat op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return value.float_value != op.value.float_value;
#else
    return ! (*this == op);
#endif
}

bool IEEEfloat::operator< (const IEEEfloat op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return (value.float_value < op.value.float_value);
#else
    if (IsNaN() || op.IsNaN())
        return false; // NaNs are unordered
    if (IsZero() && op.IsZero())
        return false;
    // Exploit fact that all other IEEE floating point numbers sort like
    // ints after worrying about sign.
    if (IsNegative())
        return op.IsPositive() ||
            (value.word & ABS_BITS) > (op.value.word & ABS_BITS);
    return op.IsPositive() &&
        (value.word & ABS_BITS) < (op.value.word & ABS_BITS);
#endif
}

bool IEEEfloat::operator<= (const IEEEfloat op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return (value.float_value <= op.value.float_value);
#else
    return *this < op || *this == op;
#endif
}

bool IEEEfloat::operator> (const IEEEfloat op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return (value.float_value > op.value.float_value);
#else
    if (IsNaN() || op.IsNaN())
        return false; // NaNs are unordered.
    if (IsZero() && op.IsZero())
        return false;
    // Exploit fact that all other IEEE floating point numbers sort like
    // ints after worrying about sign.
    if (IsPositive())
        return op.IsNegative() ||
            (value.word & ABS_BITS) > (op.value.word & ABS_BITS);
    return op.IsNegative() &&
        (value.word & ABS_BITS) < (op.value.word & ABS_BITS);
#endif
}

bool IEEEfloat::operator>= (const IEEEfloat op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return (value.float_value >= op.value.float_value);
#else
    return *this > op || *this == op;
#endif
}

IEEEfloat IEEEfloat::operator+ (const IEEEfloat op) const
{
#ifdef HAVE_IEEE754
    // FIXME: This could be sped up by inlining
    return IEEEfloat(value.float_value + op.value.float_value);
#else
    if (IsNaN() || op.IsNaN())
        return NaN(); // arithmetic on NaNs not allowed

    //
    // Adding unlike infinities not allowed
    // Adding infinities of same sign is infinity of that sign
    // Adding finite and infinity produces infinity
    //
    if (IsInfinite())
        return op.IsInfinite() && (Sign() != op.Sign()) ? NaN() : *this;
    if (op.IsInfinite())
        return op;

    //
    // Adding zero is easy
    //
    if (IsZero())
        return (op.IsZero()) ? (Sign() != op.Sign()) ? POSITIVE_ZERO()
                                                     : *this
                             : op;
    if (op.IsZero())
        return *this;

    //
    // Now for the real work - do manipulations on copies
    //
    i4 x, y, round = 0;
    int expx, expy, signx, signy;

    expx = SplitInto((u4 &) x);
    expy = op.SplitInto((u4 &) y);

    signx = Sign();
    signy = op.Sign();

    // If the exponents are far enough apart, the answer is easy
    if (expx > expy + 25)
        return *this;
    if (expy > expx + 25)
        return op;

    //
    // Denormalize the fractions, so that the exponents are
    // the same and then set the exponent for the result.
    // Leave enough space for overflow and INT_MIN avoidance!
    //
    if (signx)
        x = -x;
    if (signy)
        y = -y;

    x <<= 6;
    y <<= 6;

    if (expx > expy) {
        round = y << (32 + expy - expx);
        y >>= expx - expy;
    }
    else if (expy > expx)
    {
        round = x << (32 + expx - expy);
        x >>= expy - expx;
        expx = expy;
    }

    //
    // Do the arithmetic. The excess magnitude of 32-bit arithmetic means
    // overflow is impossible (we only need 1 spare bit!). We ensure that
    // pre-alignment avoids any question of INT_MIN negation problems.
    //
    x += y;
    if (round)
        x |= 1;

    //
    // If the result is negative, then make the fraction positive again
    // and remember the sign.
    //
    if (x < 0)
    {
        x = -x;
        signx = 1;
    }
    else
        signx = 0;

    if (x == 0)
        return signx ? NEGATIVE_ZERO() : POSITIVE_ZERO();

    //
    // Time to normalize again! If we need to shift left (the addition was
    // effectively a subtraction), then there cannot be any reason to round.
    // If the number fits exactly we don't have anything to do either.
    //
    return Normalize(signx, expx - 6, (u4) x);
#endif
}

IEEEfloat IEEEfloat::operator- () const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return IEEEfloat(-value.float_value);
#else
    if (IsNaN())
        return *this;
    return IEEEfloat(value.word ^ SIGN_BIT);
#endif
}

IEEEfloat IEEEfloat::operator* (const IEEEfloat op) const
{
#ifdef HAVE_IEEE754
    return IEEEfloat(value.float_value * op.value.float_value);
#else
    if (IsNaN() || op.IsNaN())
        return NaN(); // arithmetic on NaNs not allowed

    int sign = Sign() ^ op.Sign();

    //
    // If either operand is zero or infinite, then the answer is easy.
    //
    if (IsZero())
        return op.IsInfinite() ? NaN()
                               : sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();

    if (op.IsZero())
        return IsInfinite() ? NaN()
                            : sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();

    if (IsInfinite() || op.IsInfinite())
        return sign ? NEGATIVE_INFINITY() : POSITIVE_INFINITY();

    //
    // Now for the real work - do manipulations on copies
    //
    u4 x, y;
    int exponent;

    exponent = SplitInto(x) + op.SplitInto(y);

    //
    // The numbers to be multiplied are 24 bits in length (stored in 32 bit
    // integers). Using ULongInt to perform the multiplication, the result
    // will be 46-48 bits (unsigned); shift it back to 28 bits for Normalize,
    // while folding the low 20 bits into the lsb for rounding purposes.
    //

    ULongInt a = x,
             b = y;
    a *= b;
    b = a & 0xfffff;
    a >>= 20;
    x = a.LowWord() | ((b > 0) ? 1 : 0);

    return Normalize(sign, exponent - 3, x);
#endif // HAVE_IEEE754
}

IEEEfloat IEEEfloat::operator/ (const IEEEfloat op) const
{
#ifdef HAVE_IEEE754
    return op.IsZero() ? ((IsNaN() || IsZero()) ? NaN()
                               : (IsPositive() ^ op.IsPositive())
                                       ? NEGATIVE_INFINITY()
                                       : POSITIVE_INFINITY())
                       : IEEEfloat(value.float_value / op.value.float_value);
#else // HAVE_IEEE754
    if (IsNaN() || op.IsNaN())
        return NaN(); // arithmetic on NaNs not allowed

    int sign = Sign() ^ op.Sign();

    //
    // Infinities and zeroes are special.
    //

    if (IsInfinite())
        return (op.IsInfinite() ? NaN()
                : sign ? NEGATIVE_INFINITY() : POSITIVE_INFINITY());

    if (op.IsInfinite())
        return sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();

    if (IsZero())
        return op.IsZero() ? NaN()
                           : sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();

    if (op.IsZero())
        return sign ? NEGATIVE_INFINITY() : POSITIVE_INFINITY();

    //
    // Now for the real work - do manipulations on copies
    //
    u4 x, y;
    int exponent;

    exponent = SplitInto(x) - op.SplitInto(y);

    u4  mask   = 0x80000000,
        result = 0;

    // Because both values are normalised, a single shift guarantees results.

    if (x < y)
    {
        x <<= 1;
        exponent--;
    }

    //
    // If the numerator is larger, then it is divisible.
    // Reflect this in the result, and do the subtraction.
    // Magnify the numerator again and reduce the mask.
    //
    while (mask)
    {
        if (x >= y)
        {
            result += mask;
            x -= y;
            if (x == 0)
                break;
        }
        x <<= 1;
        mask >>= 1;
    }

    return Normalize(sign, exponent - 8, result);
#endif // HAVE_IEEE754
}

IEEEfloat IEEEfloat::operator% (const IEEEfloat op) const
{
#ifdef HAVE_IEEE754
    return IEEEfloat((op.IsZero() ? NaN().value.float_value
                      : (float) fmod((double) value.float_value,
                                     (double) op.value.float_value)));
#else // HAVE_IEEE754
    if (IsNaN() || op.IsNaN())
        return NaN(); // arithmetic on NaNs not allowed

    //
    // Infinities and zeroes are special.
    //

    if (IsInfinite() || op.IsZero())
        return NaN();

    if (IsZero() || op.IsInfinite())
        return *this;

    //
    // Now for the real work - do manipulations on copies
    // This algorithm is from fdlibm.c - see above notice
    //

    int sign = Sign();
    u4 x, y;
    int expy, exponent;
    i4 z;

    expy = op.SplitInto(y);
    exponent = SplitInto(x) - expy;

    if (exponent < 0 || (exponent == 0 && x < y))
        return *this;

    while (exponent--)
    {
        z = x - y;
        if (z < 0)
            x <<= 1;
        else if (z == 0)
            return sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
        else
            x = z + z;
    }
    z = x - y;
    if (z >= 0)
        x = z;

    if (x == 0)
        return sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
    return Normalize(sign, expy, x);
#endif // HAVE_IEEE754
}



IEEEdouble::IEEEdouble(const IEEEfloat &f)
{
#ifdef HAVE_IEEE754
    value.double_value = (double) f.FloatView();
#else
    int sign = f.Sign();
    int exponent = f.Exponent();

    if (exponent == -IEEEfloat::Bias())
    {
        if (f.IsZero())
            *this = sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
        else
        {
            //
            // This is a denormalized number, shift it to fit double format
            //
            *this = Normalize(sign, 1 - IEEEfloat::Bias(),
                              ULongInt(f.Fraction()) << 29);
        }
    }
    else
    {
        if (f.IsInfinite())
            *this = sign ? NEGATIVE_INFINITY() : POSITIVE_INFINITY();
        else if (f.IsNaN())
            *this = NaN();
        else
        {
            // Regular, normalized number.  Shift it to fit double format
            *this = Normalize(sign, exponent, ULongInt(f.Fraction()) << 29);
        }
    }
#endif // HAVE_IEEE754
}

IEEEdouble::IEEEdouble(i4 a)
{
#ifdef HAVE_IEEE754
    value.double_value = (double) a;
#else
    int sign = 0;

    if (a < 0)
    {
        a  = -a; // even works for MIN_INT!
        sign = 1;
    }
    *this = (a == 0) ? POSITIVE_ZERO()
        : Normalize(sign, FRACT_SIZE, ULongInt((u4) a));
#endif // HAVE_IEEE754
}

IEEEdouble::IEEEdouble(const LongInt &a)
{
#ifdef HAVE_IEEE754
# ifdef HAVE_64BIT_TYPES
    value.double_value = (double)(i8) a.Words();
# else
    value.double_value = ((double)(i4) a.HighWord() * (double) 0x40000000 *
                          4.0) + (double) a.LowWord();
# endif // HAVE_64BIT_TYPES
#else
    int sign = 0;
    LongInt l = a;

    if (a < 0)
    {
        l  = -a; // even works for MIN_LONG!
        sign = 1;
    }
    *this = (l == 0) ? POSITIVE_ZERO()
        : Normalize(sign, FRACT_SIZE, ULongInt(l));
#endif // HAVE_IEEE754
}

IEEEdouble::IEEEdouble(const char *str, bool check_invalid)
{
    int bb2, bb5, bbe, bd2, bd5, bbbits, bs2,
        e, e1, i, j, k;
    int nd, // number of digits in mantissa (except extra '0's)
        nd0, // number of digits before '.' (except leading '0's)
        nf, // number of digits after '.' (except trailing '0's)
        nz; // number of consecutive '0' digits
    bool nz0, // whether leading zero exists
         roundup, // whether to round up long string
         sign, // if the string represents a negative
         dsign, // the sign of delta
         esign; // the sign of the exponent
    char c;
    const char *s, *s0, *s1;
    IEEEdouble aadj, aadj1;
    i4 L;
    i4 y, z;

    sign = nz0 = roundup = false;
    nz = 0;

    //
    // consume whitespace
    //
    for (s = str; ; s++)
    {
        switch (*s)
        {
        case U_MINUS:
            sign = true;
            // fallthrough
        case U_PLUS:
            if (*++s)
                goto break2;
            // fallthrough
        case U_NU:
            *this = NaN();
            return;
        case U_SPACE:
        case U_HT:
        case U_FF:
        case U_LF:
        case U_CR:
            continue;
        default:
            goto break2;
        }
    }
 break2:
    //
    // Consume leading zeroes.
    //
    if (*s == U_0)
    {
        nz0 = true;
        while (*++s == U_0);

        if (! *s)
        {
            *this = sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
            return;
        }
        //
        // Parse hexadecimal floating point.
        //
        else if (*s == U_x || *s == U_X)
        {
            LongInt fraction = 0;
            // Exponent adjustment, based on '.' location.
            int shift = FRACT_SIZE;
            bool seen_dot = false;
            while (*++s && *s == U_0); // Consume leading zeroes.
            c = *s;
            if (c == U_DOT)
            {
                seen_dot = true;
                while (*++s && *s == U_0)
                    shift -= 4;
                c = *s;
            }
            if (! c || c == U_p || c == U_P)
            {
                *this = sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
                return;
            }
            // To avoid overflow, stop after enough bits have been read.
            for (i = 0; i < (FRACT_SIZE >> 2) + 2 && c; i++, c = *++s)
            {
                if (c == U_DOT)
                {
                    if (seen_dot)
                        break;
                    c = *++s;
                    seen_dot = true;
                }
                int value;
                if (Code::IsHexDigit(c))
                    value = Code::Value(c);
                else break;
                if (seen_dot)
                    shift -= 4;
                fraction = (fraction << 4) + value;
            }
            // Round any remaining bits.
            bool sticky = false;
            while (c == U_DOT || Code::IsHexDigit(c))
            {
                if (c == U_DOT)
                {
                    if (seen_dot)
                        break;
                    seen_dot = true;
                }
                else
                {
                    if (! seen_dot)
                        shift += 4;
                    if (c != U_0)
                        sticky = true;
                }
                c = *++s;
            }
            assert(fraction != 0);
            if (sticky)
                fraction++;
            // On to the expononet.
            int exponent = 0;
            esign = false;
            if (c == U_p || c == U_P)
            {
                if (*++s == U_MINUS)
                {
                    esign = true;
                    s++;
                }
                else if (*s == U_PLUS)
                    s++;
                while ((c = *s++))
                {
                    if (! Code::IsDecimalDigit(c))
                        break;
                    exponent = exponent * 10 + c - U_0;
                    // Check for exponent overflow
                    if (exponent + shift > 19999)
                    {
                        if (check_invalid)
                            *this = NaN();
                        else
                        {
                            *this = esign ? POSITIVE_ZERO()
                                : POSITIVE_INFINITY();
                            if (sign)
                                *this = - *this;
                        }
                        return;
                    }
                }
            }
            if (esign)
                exponent = - exponent;
            *this = Normalize(sign, exponent + shift, fraction);
            if (check_invalid && (IsZero() || IsInfinite()))
                *this = NaN();
            return;
        }
    }

    //
    // parse before '.'
    //
    s0 = s;
    y = z = 0;
    for (nd = nf = 0; Code::IsDecimalDigit(c = *s); nd++, s++)
        if (nd < 9)
            y = 10 * y + c - U_0;
        else if (nd < 16)
            z = 10 * z + c - U_0;
    nd0 = nd;
    if (c == U_DOT)
    {
        //
        // parse after '.'
        //
        c = *++s;
        if (!nd)
        {
            for ( ; c == U_0; c = *++s)
                nz++;
            if (c > U_0 && c <= U_9)
            {
                s0 = s;
                nf += nz;
                nz = 0;
            }
        }
        for ( ; Code::IsDecimalDigit(c); c = *++s)
        {
            nz++;
            if (c -= U_0)
            {
                nf += nz;
                for (i = 1; i < nz; i++)
                    if (nd++ < 9)
                        y *= 10;
                    else if (nd <= 16)
                        z *= 10;
                if (nd++ < 9)
                    y = 10 * y + c;
                else if (nd <= 16)
                    z = 10 * z + c;
                nz = 0;
            }
        }
    }
    //
    // consume exponent
    //
    e = 0;
    if (c == U_e || c == U_E) {
        str = s;
        esign = false;
        switch (c = *++s)
        {
        case U_MINUS:
            esign = true;
            // fallthrough
        case U_PLUS:
            c = *++s;
        }
        if (Code::IsDecimalDigit(c))
        {
            if (!nd && !nz && !nz0)
            {
                *this = sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
                return;
            }
            while (c == U_0)
                c = *++s;
            if (c > U_0 && c <= U_9)
            {
                L = c - U_0;
                s1 = s;
                while (Code::IsDecimalDigit(c = *++s))
                    L = 10 * L + c - U_0;
                //
                // Avoid confusion from exponents so large that e might
                // overflow
                if (s - s1 > 8 || L > 19999)
                    e = 19999;
                else
                    e = L;
                if (esign)
                    e = -e;
            }
            else
                e = 0;
        }
        else
            s = str;
    }
    if (!nd) {
        *this = (!nz && !nz0) ? NaN()
                              : sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
        return;
    }
    //
    // for long strings, round away all digits beyond maximum precise string
    // for fraction, there are n decimal digits after '.' if lsb is 2^n;
    // but the first m of these digits are '0', for d = x*10^m
    // so, digits after MAX_DIGITS may be ignored
    //
    if (nd > MAX_DIGITS)
    {
        k = nd - MAX_DIGITS;
        i = MAX_DIGITS - nd0;
        if (i <= 0)
        {
            // decimal after last precise digit
            nd0 = MAX_DIGITS;
            nf = i;
            j = 0;
        }
        else if (i == MAX_DIGITS)
        {
            // decimal before first precise digit
            nf -= k;
            j = 0;
        }
        else
        {
            // decimal inside precise digits
            nf -= k;
            j = 1;
        }
        roundup = s0[MAX_DIGITS - 1 + j] != U_4;
        nd = MAX_DIGITS;
    }
    e1 = e -= nf;

    //
    // Now we have nd0 digits, starting at s0, followed by a
    // decimal point, followed by nd-nd0 digits.  The number we're
    // after is the integer represented by those digits times 10**e
    //
    if (! nd0)
        nd0 = nd;
    k = nd < 16 ? nd : 16;
    *this = IEEEdouble(y);
    if (k > 9)
        *this = *this * tens[k - 9] + z;
    if (nd < 16)
    {
        if (!e)
        {
            if (sign)
                *this = -*this;
            return;
        }
        if (e > 0)
        {
            if (e <= 22)
            {
                *this *= tens[e];
                if (sign)
                    *this = -*this;
                return;
            }
            i = 15 - nd;
            if (e <= 22 + i)
            {
                e -= i;
                *this *= tens[i];
                *this *= tens[e];
                if (sign)
                    *this = -*this;
                return;
            }
        }
        else if (e >= -22 )
        {
            *this /= tens[-e];
            if (sign)
                *this = -*this;
            return;
        }
    }
    e1 += nd - k;

    //
    // Get starting approximation: *this * 10**e1
    //
    if (e1 > 0)
    {
        i = e1 & 0xf;
        if (i)
            *this *= tens[i];
        if (e1 >>= 4)
        {
            if (e1 > MAX_DEC_EXP >> 4)
            {
                *this = check_invalid ? NaN()
                                      : sign ? NEGATIVE_INFINITY()
                                             : POSITIVE_INFINITY();
                return;
            }
            for (j = 0; e1 > 1; j++, e1 >>= 1)
                if (e1 & 1)
                    *this *= bigtens[j];
            //
            // The last multiplication could overflow.
            //
            setHighWord(HighWord() - (FRACT_SIZE + 1) * MIN_FRACT);
            *this *= bigtens[j];
            z = Exponent();
            if (z > BIAS - FRACT_SIZE)
            {
                *this = check_invalid ? NaN()
                                      : sign ? NEGATIVE_INFINITY()
                                             : POSITIVE_INFINITY();
                return;
            }
            if (z > BIAS - FRACT_SIZE - 1)
                setHighAndLowWords(POS_INF_HI - 1, ZERO_LO - 1);
            else
                setHighWord(HighWord() + (FRACT_SIZE + 1) * MIN_FRACT);
        }
    }
    else if (e1 < 0)
    {
        e1 = -e1;
        i = e1 & 0xf;
        if (i)
            *this /= tens[i];
        if (e1 >>= 4)
        {
            if (e1 >= 1 << 5)
            {
                *this = check_invalid ? NaN()
                                      : sign ? NEGATIVE_ZERO()
                                             : POSITIVE_ZERO();
                return;
            }
            for (j = 0; e1 > 1; j++, e1 >>= 1)
                if (e1 & 1)
                    *this /= bigtens[j];
            //
            // The last multiplication could underflow.
            //
            IEEEdouble tmp(*this);
            *this /= bigtens[j];
            if (IsZero())
            {
                *this = tmp * 2;
                *this /= bigtens[j];
                if (IsZero())
                {
                    *this = check_invalid ? NaN()
                                          : sign ? NEGATIVE_ZERO()
                                                 : POSITIVE_ZERO();
                    return;
                }
                setHighAndLowWords(0, 1);
            }
        }
    }

    //
    // Now the hard part -- adjusting *this to the correct value.
    // Put digits into bd: true value = bd * 10^e
    //
    BigInt bd0(s0, nd0, nd, y, 9);
    if (roundup)
        ++bd0;
    while (true) {
        BigInt bd(bd0);
        BigInt bb(*this, bbe, bbbits); // *this = bb * 2^bbe
        BigInt bs(1);
        if (e >= 0)
        {
            bb2 = bb5 = 0;
            bd2 = bd5 = e;
        }
        else
        {
            bb2 = bb5 = -e;
            bd2 = bd5 = 0;
        }
        if (bbe >= 0)
            bb2 += bbe;
        else
            bd2 -= bbe;
        bs2 = bb2;
        j = bbe;
        i = j + bbbits - 1; // logb(*this)
        if (i < 1 - BIAS) // denormal
            j += BIAS + FRACT_SIZE;
        else
            j = FRACT_SIZE + 2 - bbbits;
        bb2 += j;
        bd2 += j;
        i = bb2 < bd2 ? bb2 : bd2;
        if (i > bs2)
            i = bs2;
        if (i > 0)
        {
            bb2 -= i;
            bd2 -= i;
            bs2 -= i;
        }
        if (bb5 > 0) {
            bs.pow5mult(bb5);
            bb *= bs;
        }
        if (bb2 > 0)
            bb <<= bb2;
        if (bd5 > 0)
            bd.pow5mult(bd5);
        if (bd2 > 0)
            bd <<= bd2;
        if (bs2 > 0)
            bs <<= bs2;
        BigInt delta = bb - bd;
        dsign = delta.IsNegative();
        if (dsign)
            delta.IsNegative(false);
        i = delta.compareTo(bs);
        //
        // Error is less than half an ulp -- check for
        // special case of mantissa a power of two.
        //
        if (i < 0)
        {
            if (dsign || LowWord() || FractBits()
                || Exponent() <= 1 - BIAS || delta.IsZero())
                break;
            delta <<= 1;
            if (delta.compareTo(bs) > 0)
                // boundary case -- decrement exponent
                BaseLong::operator --();
            break;
        }
        //
        // exactly half-way between
        //
        else if (i == 0)
        {
            if (LowWord() & 1)
                BaseLong::operator +=(dsign ? 1 : -1);
            break;
        }
        //
        // more than 1/2 ulp off - try again
        //
        aadj = Ratio(delta, bs);
        if (aadj <= 2)
        {
            if (dsign)
                aadj = aadj1 = 1;
            else if (FractBits() || LowWord())
            {
                if (!HighWord() && LowWord() == 1)
                {
                    // underflow
                    *this = POSITIVE_ZERO();
                    break;
                }
                aadj = 1;
                aadj1 = -1;
            }
            else
            {
                //
                // special case - mantissa is power of 2
                //
                if (aadj < 1.0)
                    aadj = 0.5;
                else
                    aadj *= 0.5;
                aadj1 = -aadj;
            }
        }
        else
        {
            aadj *= 0.5;
            //
            // gcc 2.95.2 has an ICE from too many registers with this line:
            //            aadj1 = dsign ? aadj : -aadj;
            //
            if (dsign)
                aadj1 = aadj;
            else
                aadj1 = -aadj;
        }
        y = Exponent();
        //
        // Check for overflow
        //
        if (y == BIAS)
        {
            IEEEdouble tmp(*this);
            setHighWord(HighWord() - (FRACT_SIZE + 1) * MIN_FRACT);
            *this += aadj1 * Ulp();
            if (Exponent() >= BIAS - FRACT_SIZE)
            {
                if (tmp.HighWord() == POS_INF_HI - 1 &&
                    tmp.LowWord() == ZERO_LO - 1)
                {
                    // overflow
                    *this = POSITIVE_INFINITY();
                    break;
                }
                setHighAndLowWords(POS_INF_HI - 1, ZERO_LO - 1);
                continue;
            }
            else
                setHighWord(HighWord() + (FRACT_SIZE + 1) * MIN_FRACT);
        }
        else
        {
            //
            // Compute adj so that the IEEE rounding rules will
            // correctly round *this + adj in some half-way cases.
            // If *this * Ulp() is denormalized, we must adjust aadj
            // to avoid trouble from bits lost to denormalization.
            //
            if (y <= FRACT_SIZE - BIAS || aadj > 1)
            {
                aadj1 = IEEEdouble((aadj + 0.5).IntValue());
                if (! dsign)
                    aadj1 = -aadj1;
            }
            *this += aadj1 * Ulp();
        }
        if (y == Exponent())
        {
            //
            // Can we stop now?
            // The tolerances below are conservative.
            //
            aadj -= aadj.IntValue();
            if (dsign || FractBits() || LowWord())
            {
                if (aadj < .4999999 || aadj > .5000001)
                    break;
            }
            else if (aadj < .4999999 / 2)
                break;
        }
    }
    if (check_invalid && (IsZero() || IsInfinite()))
        *this = NaN();
    else if (sign)
        *this = -*this;
}

i4 IEEEdouble::IntValue() const
{
    if (IsNaN())
        return 0;

#ifdef HAVE_IEEE754
    if (value.double_value < (double)(i4) Int::MIN_INT())
        return Int::MIN_INT();
    else if (value.double_value > (double) Int::MAX_INT())
        return Int::MAX_INT();
    return (i4) value.double_value;
#else
    int sign = Sign(),
        exponent = Exponent();

    if (exponent > 30)
        return sign ? Int::MIN_INT() : Int::MAX_INT();

    // This includes true zero and denorms.
    if (exponent < 0)
        return 0;

    i4 result = (i4) (Fraction() >> (FRACT_SIZE - exponent)).LowWord();

    return sign ? -result : result;
#endif // ! HAVE_IEEE754
}

LongInt IEEEdouble::LongValue() const
{
    if (IsNaN())
        return LongInt(0);

    int sign = Sign(),
        exponent = Exponent();

    if (exponent > 62)
        return sign ? LongInt::MIN_LONG() : LongInt::MAX_LONG();

    // This covers true zero and denorms.
    if (exponent < 0)
        return LongInt(0);

    LongInt result = Fraction();

    if (exponent > (int) FRACT_SIZE)
        result <<= (exponent - FRACT_SIZE);
    else if (exponent < (int) FRACT_SIZE)
        result >>= (FRACT_SIZE - exponent);

    return sign ? (LongInt) -result : result;
}

IEEEdouble IEEEdouble::Normalize(int sign, int exponent, ULongInt fraction)
{
    bool round = false, sticky = false;

    assert(fraction != 0);

    //
    // Normalize right. Shift until value < MAX_FRACT.
    //
    if (fraction.HighWord() >= MAX_FRACT)
    {
        while (fraction.HighWord() >= MAX_FRACT)
        {
            sticky |= round;
            round = (fraction.LowWord() & 1) != 0;
            fraction >>= 1;
            exponent++;
        }
        if (round && (sticky || (fraction.LowWord() & 1)) &&
            exponent > -(int) BIAS)
        {
            //
            // Capture any overflow caused by rounding. No other checks are
            // required because if overflow occurred, the the low order bit
            // was guaranteed to be zero. Do not round denorms yet.
            //
            if ((++fraction).HighWord() >= MAX_FRACT)
            {
                fraction >>= 1;
                exponent++;
            }
        }
    }

    //
    // Normalize left. Shift until value >= MIN_FRACT.
    //
    else
        while (fraction.HighWord() < MIN_FRACT)
        {
            fraction <<= 1;
            exponent--;
        }

    //
    // Check and respond to overflow
    //
    if (exponent > BIAS)
        return sign ? NEGATIVE_INFINITY() : POSITIVE_INFINITY();

    //
    // Check and respond to underflow
    //
    if (exponent <= -BIAS)
    {
        if (exponent < -BIAS - FRACT_SIZE)
            return sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
        while (exponent <= -BIAS)
        {
            sticky |= round;
            round = (fraction.LowWord() & 1) != 0;
            fraction >>= 1;
            exponent++;
        }
        if (round && (sticky || (fraction.LowWord() & 1)))
            fraction++;
        exponent = -BIAS;
        if (fraction == 0)
            return sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
    }

    return IEEEdouble((sign << 31) | ((exponent + BIAS) << FRACT_SIZE_HI)
                                   | (fraction.HighWord() & FRACT_BITS),
                      fraction.LowWord());
}

int IEEEdouble::SplitInto(BaseLong &fraction) const
{
    int exponent = Exponent();
    fraction = Fraction();

    if (exponent == -(int) BIAS)
    {
        exponent++;
        while (fraction.HighWord() < MIN_FRACT)
        {
            fraction <<= 1;
            exponent--;
        }
    }

    return exponent;
}

IEEEdouble IEEEdouble::Ulp() const
{
    i4 L;
    IEEEdouble d;
    d.value.double_value = value.double_value;

    L = (i4) d.ExpBits() - FRACT_SIZE * MIN_FRACT;
    if (L > 0)
        d.setHighAndLowWords((u4) L, 0);
    else
    {
        L = -L >> FRACT_SIZE_HI;
        if (L < (i4) FRACT_SIZE_HI)
            d.setHighAndLowWords(MIN_FRACT >> (L + 1), 0);
        else
        {
            L -= FRACT_SIZE_HI;
            d.setHighAndLowWords(0, L >= 31 ? 1 : 1 << (31 - L));
        }
    }
    return d;
}

IEEEdouble IEEEdouble::Ratio(const BigInt &a, const BigInt &b)
{
    IEEEdouble da, db;
    int k;
    da = a.DoubleValue();
    db = b.DoubleValue();
    k = b.hi0bits() - a.hi0bits() + 32 * (a.wds - b.wds);
    if (k > 0)
        da.setHighWord(da.HighWord() + k * MIN_FRACT);
    else
        db.setHighWord(db.HighWord() - k * MIN_FRACT);
    return da / db;
}

bool IEEEdouble::operator== (const IEEEdouble op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    // TODO: Microsoft VC++ botches this, mixing 12.0 and NaN
    return value.double_value == op.value.double_value;
#else
    return (IsNaN() || op.IsNaN() ? false
            : IsZero() && op.IsZero() ? true
            : (BaseLong) *this == (BaseLong) op);
#endif
}

bool IEEEdouble::operator!= (const IEEEdouble op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return value.double_value != op.value.double_value;
#else
    return !(*this == op);
#endif
}

IEEEdouble IEEEdouble::operator+ (const IEEEdouble op) const
{
#ifdef HAVE_IEEE754
    // FIXME: This could be sped up by inlining
    return IEEEdouble(value.double_value + op.value.double_value);
#else
    if (IsNaN() || op.IsNaN())
        return NaN(); // arithmetic on NaNs not allowed

    //
    // Adding unlike infinities not allowed
    // Adding infinities of same sign is infinity of that sign
    // Adding finite and infinity produces infinity
    //
    if (IsInfinite())
        return op.IsInfinite() && (Sign() != op.Sign()) ? NaN() : *this;
    if (op.IsInfinite())
        return op;

    //
    // Adding zero is easy
    //
    if (IsZero())
        return (op.IsZero()) ? (Sign() != op.Sign()) ? POSITIVE_ZERO()
                                                     : *this
                             : op;
    if (op.IsZero())
        return *this;

    //
    // Now for the real work - do manipulations on copies
    //
    LongInt x, y, round = 0;
    int expx, expy, signx, signy;

    expx = SplitInto(x);
    expy = op.SplitInto(y);

    signx = Sign();
    signy = op.Sign();

    // If the exponents are far enough apart, the answer is easy
    if (expx > expy + 54)
        return *this;
    if (expy > expx + 54)
        return op;

    //
    // Denormalize the fractions, so that the exponents are
    // the same and then set the exponent for the result.
    // Leave enough space for overflow and LONG_MIN avoidance!
    //
    if (signx)
        x = -x;
    if (signy)
        y = -y;

    x <<= 8;
    y <<= 8;

    if (expx > expy)
    {
        round = y << (64 + expy - expx);
        y >>= expx - expy;
    }
    else if (expy > expx)
    {
        round = x << (64 + expx - expy);
        x >>= expy - expx;
        expx = expy;
    }

    //
    // Do the arithmetic. The excess magnitude of 64-bit arithmetic means
    // overflow is impossible (we only need 1 spare bit!). We ensure that
    // pre-alignment avoids any question of LONG_MIN negation problems.
    //
    x += y;
    if (round != 0)
        x |= 1;

    //
    // If the result is negative, then make the fraction positive again
    // and remember the sign.
    //
    if (x < 0)
    {
        x = -x;
        signx = 1;
    }
    else
        signx = 0;

    if (x == 0)
        return signx ? NEGATIVE_ZERO() : POSITIVE_ZERO();

    //
    // Time to normalize again! If we need to shift left (the addition was
    // effectively a subtraction), then there cannot be any reason to round.
    // If the number fits exactly we don't have anything to do either.
    //
    return Normalize(signx, expx - 8, (ULongInt) x);
#endif
}

IEEEdouble IEEEdouble::operator- () const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return IEEEdouble(-value.double_value);
#else
    if (IsNaN())
        return *this;
    return IEEEdouble(HighWord() ^ SIGN_BIT, LowWord());
#endif // HAVE_IEEE754
}

IEEEdouble IEEEdouble::operator* (const IEEEdouble op) const
{
#ifdef HAVE_IEEE754
    return IEEEdouble(value.double_value * op.value.double_value);
#else
    if (IsNaN() || op.IsNaN())
        return NaN(); // arithmetic on NaNs not allowed

    int sign = Sign() ^ op.Sign();

    //
    // If either operand is zero or infinite, then the answer is easy.
    //
    if (IsZero())
        return op.IsInfinite() ? NaN()
                               : sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();

    if (op.IsZero())
        return IsInfinite() ? NaN()
                            : sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();

    if (IsInfinite() || op.IsInfinite())
        return sign ? NEGATIVE_INFINITY() : POSITIVE_INFINITY();

    //
    // Now for the real work - do manipulations on copies
    //
    ULongInt x, y, z, w, pr1, pr2;
    u4 round;
    int exponent;

    exponent = SplitInto(x) + op.SplitInto(y);

    //
    // The numbers to be multiplied are 53 bits in length (stored in 64 bit
    // integers). Split them in 32 bit parts, then shift result into place.
    // Fold the low order bits into the lsb for rounding purposes.
    //
    z = ULongInt(x.LowWord());
    w = ULongInt(y.LowWord());
    x >>= 32;
    y >>= 32;

    pr1 = z * w; // low*low
    pr2 = z * y + x * w + pr1.HighWord(); // low*high + high*low + adjusted pr1
    round = pr1.LowWord();
    x = (x * y) << 20; // high*high, adjusted

    round |= pr2.LowWord() & 0xFFF;
    pr2 >>= 12;

    x += pr2 | (round ? 1 : 0);

    return Normalize(sign, exponent - 8, x);
#endif // HAVE_IEEE754
}

IEEEdouble IEEEdouble::operator/ (const IEEEdouble op) const
{
#ifdef HAVE_IEEE754
    return op.IsZero() ? ((IsNaN() || IsZero()) ? NaN()
                          : ((IsPositive() ^ op.IsPositive())
                             ? NEGATIVE_INFINITY()
                             : POSITIVE_INFINITY()))
        : IEEEdouble(value.double_value / op.value.double_value);
#else // HAVE_IEEE754
    if (IsNaN() || op.IsNaN())
        return NaN(); // arithmetic on NaNs not allowed

    int sign = Sign() ^ op.Sign();

    //
    // Infinities and zeroes are special.
    //

    if (IsInfinite())
        return (op.IsInfinite() ? NaN()
                : sign ? NEGATIVE_INFINITY() : POSITIVE_INFINITY());

    if (op.IsInfinite())
        return sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();

    if (IsZero())
        return op.IsZero() ? NaN()
                           : sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();

    if (op.IsZero())
        return sign ? NEGATIVE_INFINITY() : POSITIVE_INFINITY();

    //
    // Now for the real work - do manipulations on copies
    //
    ULongInt x, y;
    int exponent;

    exponent = SplitInto(x) - op.SplitInto(y);

    ULongInt mask  (0x80000000, 0x00000000),
             result(0x00000000, 0x00000000);

    // Because both values are normalised, a single shift guarantees results.

    if (x < y)
    {
        x <<= 1;
        exponent--;
    }

    //
    // If the numerator is larger, then it is divisible.
    // Reflect this in the result, and do the subtraction.
    // Magnify the numerator again and reduce the mask.
    //
    while (mask != 0)
    {
        if (x >= y)
        {
            result += mask;
            x -= y;
            if (x == 0)
                break;
        }
        x <<= 1;
        mask >>= 1;
    }

    return Normalize(sign, exponent - 11, result);
#endif // HAVE_IEEE754
}

IEEEdouble IEEEdouble::operator% (const IEEEdouble op) const
{
#ifdef HAVE_IEEE754
    return IEEEdouble((op.IsZero() ? NaN().value.double_value
                       : fmod(value.double_value, op.value.double_value)));
#else // HAVE_IEEE754
    if (IsNaN() || op.IsNaN())
        return NaN(); // arithmetic on NaNs not allowed

    //
    // Infinities and zeroes are special.
    //

    if (IsInfinite() || op.IsZero())
        return NaN();

    if (IsZero() || op.IsInfinite())
        return *this;

    //
    // Now for the real work - do manipulations on copies
    // This algorithm is from fdlibm.c - see above notice
    //

    int sign = Sign();
    ULongInt x, y;
    int expy, exponent;
    LongInt z;

    expy = op.SplitInto(y);
    exponent = SplitInto(x) - expy;

    if (exponent < 0 || (exponent == 0 && x < y))
        return *this;

    while (exponent--)
    {
        z = x - y;
        if (z < 0)
            x <<= 1;
        else if (z == 0)
            return sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
        else
            x = z + z;
    }
    z = x - y;
    if (z >= 0)
        x = z;

    if (x == 0)
        return sign ? NEGATIVE_ZERO() : POSITIVE_ZERO();
    return Normalize(sign, expy, x);
#endif // HAVE_IEEE754
}

bool IEEEdouble::operator< (const IEEEdouble op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return (value.double_value < op.value.double_value);
#else
    if (IsNaN() || op.IsNaN())
        return false; // NaNs are unordered
    if (IsZero() && op.IsZero())
        return false;
    // Exploit fact that all other IEEE floating point numbers sort like
    // ints after worrying about sign.
    u4 a = HighWord() & ABS_BITS, b = op.HighWord() & ABS_BITS;
    if (IsNegative())
        return op.IsPositive() ||
            (a > b || (a == b && LowWord() > op.LowWord()));
    return op.IsPositive() &&
        (a < b || (a == b && LowWord() < op.LowWord()));
#endif
}

bool IEEEdouble::operator<= (const IEEEdouble op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return (value.double_value <= op.value.double_value);
#else
    return *this < op || *this == op;
#endif
}

bool IEEEdouble::operator> (const IEEEdouble op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return (value.double_value > op.value.double_value);
#else
    if (IsNaN() || op.IsNaN())
        return false; // NaNs are unordered.
    if (IsZero() && op.IsZero())
        return false;
    // Exploit fact that all other IEEE floating point numbers sort like
    // ints after worrying about sign.
    u4 a = HighWord() & ABS_BITS, b = op.HighWord() & ABS_BITS;
    if (IsPositive())
        return op.IsNegative() ||
            (a > b || (a == b && LowWord() > op.LowWord()));
    return op.IsNegative() &&
        (a < b || (a == b && LowWord() < op.LowWord()));
#endif
}

bool IEEEdouble::operator>= (const IEEEdouble op) const
{
    // FIXME: This could be sped up by inlining
#ifdef HAVE_IEEE754
    return (value.double_value >= op.value.double_value);
#else
    return *this > op || *this == op;
#endif
}


#ifndef HAVE_MEMBER_CONSTANTS
u4 BigInt::fives[] = {
    1, 5, 25, 125, 625, 3125, 15625, 78125
};
BigInt *BigInt::bigfives[] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
#else // HAVE_MEMBER_CONSTANTS
const u4 BigInt::fives[] = {
    1, 5, 25, 125, 625, 3125, 15625, 78125
};
const BigInt *BigInt::bigfives[] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
#endif // HAVE_MEMBER_CONSTANTS

BigInt::BigInt(const IEEEfloat &f, int &e, int &bits) : data(NULL)
{
    int fe, k;
    u4 z;
    resize(0);
    z = f.Fraction();
    fe = f.Exponent();
    k = lo0bits(z);
    data[0] = z;
    wds = 1;
    if (fe + IEEEfloat::Bias())
    {
        e = k + fe - IEEEfloat::FractSize();
        bits = IEEEfloat::FractSize() - k + 1;
    }
    else
    {
        e = k + fe - IEEEfloat::FractSize() + 1;
        bits = 32 - hi0bits(z);
    }
}

BigInt::BigInt(const IEEEdouble &d, int &e, int &bits) : data(NULL)
{
    int de, k;
    LongInt x;
    u4 y, z;

    resize(1);
    x = d.Fraction();
    z = x.HighWord();
    de = d.Exponent();
    if ((y = x.LowWord()) != 0)
    {
        if ((k = lo0bits(y)) != 0)
        {
            data[0] = y | z << (32 - k);
            z >>= k;
        }
        else
            data[0] = y;
        wds = (data[1] = z) ? 2 : 1;
    }
    else
    {
        k = lo0bits(z) + 32;
        data[0] = z;
        wds = 1;
    }
    if (de + IEEEdouble::Bias())
    {
        e = k + de - IEEEdouble::FractSize();
        bits = IEEEdouble::FractSize() - k + 1;
    }
    else
    {
        e = k + de - IEEEdouble::FractSize() + 1;
        bits = 32 * wds - hi0bits(data[wds - 1]);
    }
}

BigInt::BigInt(const char *s, int nd0, int nd, u4 start, int startsize) :
    data(NULL)
{
    int i, k;
    u4 x, y;
    x = (nd + 8) / 9;
    for (k = 0, y = 1; x > y; y <<= 1, k++);

    resize(k);
    data[0] = start;
    wds = 1;
    i = startsize;
    if (startsize < nd0)
    {
        s += startsize;
        do
            multadd(10, *s++ - U_0);
        while (++i < nd0);
        s++;
    }
    else
        s += startsize + 1;

    for ( ; i < nd; i++)
        multadd(10, *s++ - U_0);
}

BigInt &BigInt::operator =(const BigInt &b)
{
    if (this != &b)
    {
        k = b.k;
        maxwds = b.maxwds;
        neg = b.neg;
        wds = b.wds;
        delete[] data;
        data = new u4[maxwds];
        memcpy(data, b.data, wds * sizeof(u4));
    }
    return *this;
}

int BigInt::hi0bits(u4 x)
{
    int k = 0;
    if (! (x & 0xffff0000))
    {
        k = 16;
        x <<= 16;
    }
    if (! (x & 0xff000000))
    {
        k += 8;
        x <<= 8;
    }
    if (! (x & 0xf0000000))
    {
        k += 4;
        x <<= 4;
    }
    if (! (x & 0xc0000000))
    {
        k += 2;
        x <<= 2;
    }
    if (! (x & 0x80000000))
    {
        k++;
        if (! (x & 0x40000000))
            return 32;
    }
    return k;
}

int BigInt::lo0bits(u4 &y)
{
    int k;
    if (y & 7)
    {
        if (y & 1)
            return 0;
        if (y & 2)
        {
            y >>= 1;
            return 1;
        }
        y >>= 2;
        return 2;
    }
    k = 0;
    if (! (y & 0xffff))
    {
        k = 16;
        y >>= 16;
    }
    if (! (y & 0xff))
    {
        k += 8;
        y >>= 8;
    }
    if (! (y & 0xf))
    {
        k += 4;
        y >>= 4;
    }
    if (! (y & 0x3))
    {
        k += 2;
        y >>= 2;
    }
    if (! (y & 1))
    {
        k++;
        y >>= 1;
        if (! y)
            return 32;
    }
    return k;
}

BigInt BigInt::operator +(const unsigned op) const
{
    int i = 0; // counter
    u4 carry = op; // carry between words
    ULongInt sum; // sum
    BigInt result(*this);
    u4 *x = result.data; // access to data

    do
    {
        sum = ULongInt(*x) + carry;
        carry = sum.HighWord();
        *x++ = sum.LowWord();
    } while (carry && ++i < wds);
    if (carry && i == wds)
    {
        if (wds == maxwds)
        {
            result.maxwds = 1 << (++result.k);
            x = new u4[result.maxwds];
            memcpy(x, result.data, wds * sizeof(u4));
            delete result.data;
            result.data = x;
        }
        result.data[result.wds++] = carry;
    }
    return result;
}

BigInt BigInt::operator -(const BigInt &op) const
{
    const BigInt *a = this, *b = &op;
    BigInt zero(0);
    int i, wa, wb;
    u4 *xa, *xae, *xb, *xbe, *xc;
    u4 borrow;
    ULongInt y;

    i = a -> compareTo(op);
    if (! i)
        return zero;
    if (i < 0)
    {
        const BigInt *tmp = a;
        a = b;
        b = tmp;
        i = 1;
    }
    else
        i = 0;

    BigInt c(0);
    c.resize(a -> k);
    c.neg = i != 0;
    wa = a -> wds;
    xa = a -> data;
    xae = xa + wa;
    wb = b -> wds;
    xb = b -> data;
    xbe = xb + wb;
    xc = c.data;
    borrow = 0;
    do
    {
        y = ULongInt(*xa++) - *xb++ - borrow;
        borrow = y.HighWord() & 1;
        *xc++ = y.LowWord();
    } while (xb < xbe);
    while (xa < xae)
    {
        y = ULongInt(*xa++) - borrow;
        borrow = y.HighWord() & 1;
        *xc++ = y.LowWord();
    }
    while (!*--xc)
        wa--;
    c.wds = wa;
    return c;
}

BigInt BigInt::operator *(unsigned op) const
{
    int i = 0; // counter
    u4 carry = 0; // carry between words
    ULongInt product; // product
    BigInt result(*this);
    u4 *x = result.data; // access to data
    ULongInt factor = (u4) op; // avoid creating object multiple times

    do
    {
        product = ULongInt(*x) * factor + carry;
        carry = product.HighWord();
        *x++ = product.LowWord();
    } while (++i < wds);
    if (carry)
    {
        if (wds == maxwds)
        {
            result.maxwds = 1 << (++result.k);
            x = new u4[result.maxwds];
            memcpy(x, result.data, wds * sizeof(u4));
            delete result.data;
            result.data = x;
        }
        result.data[result.wds++] = carry;
    }
    return result;
}

BigInt BigInt::operator *(const BigInt &op) const
{
    const BigInt *a = this, *b = &op;
    int k; // c -> k
    int wa, wb, wc; // wds in each of a, b, c
    u4 *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
    u4 y, carry;
    ULongInt z;

    if (a -> wds < b -> wds)
    {
        const BigInt *tmp = a;
        a = b;
        b = tmp;
    }
    k = a -> k;
    wa = a -> wds;
    wb = b -> wds;
    wc = wa + wb;
    if (wc > a -> maxwds)
        k++;
    BigInt c(0);
    c.resize(k);
    c.neg = a -> neg ^ b -> neg;
    for (x = c.data, xa = x + wc; x < xa; x++)
        *x = 0;
    xa = a -> data;
    xae = xa + wa;
    xb = b -> data;
    xbe = xb + wb;
    xc0 = c.data;
    for ( ; xb < xbe; xc0++)
    {
        if ((y = *xb++) != 0)
        {
            x = xa;
            xc = xc0;
            carry = 0;
            do
            {
                z = ULongInt(*x++) * y + *xc + carry;
                carry = z.HighWord();
                *xc++ = z.LowWord();
            } while (x < xae);
            *xc = carry;
        }
    }
    for (xc0 = c.data, xc = xc0 + wc; wc > 0 && !*--xc; --wc);

    c.wds = wc;
    return c;
}

BigInt BigInt::operator <<(unsigned op) const
{
    int i, k1, n, n1;
    u4 *x, *x1, *xe, z;
    n = op >> 5;
    k1 = k;
    n1 = n + wds + 1;
    for (i = maxwds; n1 > i; i <<= 1)
        k1++;
    BigInt result(*this);
    result.maxwds = 1 << k1;
    result.k = k1;
    delete[] result.data;
    result.data = x1 = new u4[result.maxwds];
    for (i = 0; i < n; i++)
        *x1++ = 0;
    x = data;
    xe = x + wds;
    if (op &= 0x1f)
    {
        k1 = 32 - op;
        z = 0;
        do
        {
            *x1++ = *x << op | z;
            z = *x++ >> k1;
        } while (x < xe);
        if ((*x1 = z) != 0)
            ++n1;
    }
    else
        do
            *x1++ = *x++;
        while (x < xe);
    result.wds = n1 - 1;
    return result;
}

BigInt &BigInt::multadd(unsigned m, unsigned a)
{
    int i = 0; // counter
    u4 *x = data; // access to data
    u4 carry = a; // carry between words
    ULongInt product; // product
    ULongInt factor = (u4) m; // avoid creating object multiple times

    do
    {
        product = ULongInt(*x) * factor + carry;
        carry = product.HighWord();
        *x++ = product.LowWord();
    } while (++i < wds);
    if (carry)
    {
        if (wds == maxwds)
        {
            maxwds = 1 << (++k);
            x = new u4[maxwds];
            memcpy(x, data, wds * sizeof(u4));
            delete[] data;
            data = x;
        }
        data[wds++] = carry;
    }
    return *this;
}

BigInt &BigInt::pow5mult(unsigned k)
{
    const BigInt *p5;
    int i;
    assert(k < 0x800);
    if ((i = k & 0x7) != 0)
        *this *= fives[i];
    if (! (k >>= 3))
        return *this;
    if (! (p5 = bigfives[i = 0]))
        p5 = bigfives[i] = new BigInt(390625);
    while (true)
    {
        if (k & 1)
            *this *= *p5;
        if (! (k >>= 1))
            break;
        if (! (p5 = bigfives[++i]))
            p5 = bigfives[i] = new BigInt(*bigfives[i-1] * *bigfives[i-1]);
    }
    return *this;
}

int BigInt::compareTo(const BigInt &b) const
{
    u4 *xa, *xa0, *xb, *xb0;
    int i, j;
    i = wds;
    j = b.wds;
    if (i -= j)
        return i;
    xa0 = data;
    xa = xa0 + j;
    xb0 = b.data;
    xb = xb0 + j;
    while (xa > xa0)
        if (*--xa != *--xb)
            return *xa < *xb ? -1 : 1;
    return 0;
}

int BigInt::quorem(const BigInt &S)
{
    int n;
    u4 *bx, *bxe, q, *sx, *sxe;
    u4 borrow, carry;
    ULongInt y, ys;
    n = S.wds;
    if (wds < n)
        return 0;
    sx = S.data;
    sxe = sx + --n;
    bx = data;
    bxe = bx + n;
    q = *bxe / (*sxe + 1);
    if (q)
    {
        borrow = 0;
        carry = 0;
        do
        {
            ys = ULongInt(*sx++) * q + carry;
            carry = ys.HighWord();
            y = ULongInt(*bx) - ys.LowWord() - borrow;
            borrow = y.HighWord() & 1;
            *bx++ = y.LowWord();
        } while (sx <= sxe);
        if (!*bxe)
        {
            bx = data;
            while (--bxe > bx && !*bxe)
                --n;
            wds = n;
        }
    }
    if (compareTo(S) >= 0)
    {
        q++;
        borrow = 0;
        carry = 0;
        bx = data;
        sx = S.data;
        do
        {
            ys = ULongInt(*sx++) + carry;
            carry = ys.HighWord();
            y = ULongInt(*bx) - ys.LowWord() - borrow;
            borrow = y.HighWord() & 1;
            *bx++ = y.LowWord();
        } while (sx <= sxe);
        bx = data;
        bxe = bx + n;
        if (!*bxe)
        {
            while (--bxe > bx && !*bxe)
                --n;
            wds = n;
        }
    }
    return q;
}

IEEEfloat BigInt::FloatValue() const
{
    u4 *xa, y, z;
    int k;
    xa = data + wds;
    y = *--xa;
    k = hi0bits(y);
    if (k < 8)
        return IEEEfloat(0x3f800000 | y >> (8 - k));
    z = xa > data ? *--xa : 0;
    if (k -= 8)
        return IEEEfloat(0x3f800000 | y << k | z >> (32 - k));
    else
        return IEEEfloat(0x3f800000 | y);
}

IEEEdouble BigInt::DoubleValue() const
{
    u4 *xa, w, y, z, hi, lo;
    int k;
    xa = data + wds;
    y = *--xa;
    k = hi0bits(y);
    if (k < 11)
    {
        hi = 0x3ff00000 | y >> (11 - k);
        w = xa > data ? *--xa : 0;
        lo = y << (32 - 11 + k) | w >> (11 - k);
        return IEEEdouble(hi, lo);
    }
    z = xa > data ? *--xa : 0;
    if (k -= 11)
    {
        hi = 0x3ff00000 | y << k | z >> (32 - k);
        y = xa > data ? *--xa : 0;
        lo = z << k | y >> (32 - k);
    }
    else
    {
        hi = 0x3ff00000 | y;
        lo = z;
    }
    return IEEEdouble(hi, lo);
}




} // Close namespace Jopa block

