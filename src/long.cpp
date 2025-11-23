#include "long.h"
#include "double.h"


namespace Jikes { // Open namespace Jikes block
BaseLong::operator LongInt() const
{
    return LongInt(HighWord(), LowWord());
}

BaseLong::operator ULongInt() const
{
    return ULongInt(HighWord(), LowWord());
}

bool BaseLong::operator== (const BaseLong op) const
{
#ifdef HAVE_64BIT_TYPES
    return value.words == op.value.words;
#else
    return value.word[0] == op.value.word[0]
        && value.word[1] == op.value.word[1];
#endif // HAVE_64BIT_TYPES
}

bool BaseLong::operator!= (const BaseLong op) const
{
#ifdef HAVE_64BIT_TYPES
    return value.words != op.value.words;
#else
    return value.word[0] != op.value.word[0]
        || value.word[1] != op.value.word[1];
#endif // HAVE_64BIT_TYPES
}

bool BaseLong::operator!() const
{
#ifdef HAVE_64BIT_TYPES
    return !value.words;
#else
    return (HighWord() != 0) || (LowWord() != 0);
#endif // HAVE_64BIT_TYPES
}

BaseLong BaseLong::operator~() const
{
#ifdef HAVE_64BIT_TYPES
    return BaseLong(~value.words);
#else
    return BaseLong(~HighWord(), ~LowWord());
#endif // HAVE_64BIT_TYPES
}

BaseLong BaseLong::operator^ (const BaseLong op) const
{
#ifdef HAVE_64BIT_TYPES
    return BaseLong(value.words ^ op.value.words);
#else
    return BaseLong(HighWord() ^ op.HighWord(), LowWord() ^ op.LowWord());
#endif // HAVE_64BIT_TYPES
}

BaseLong &BaseLong::operator^= (const BaseLong op)
{
    return *this = *this ^ op;
}

BaseLong BaseLong::operator| (const BaseLong op) const
{
#ifdef HAVE_64BIT_TYPES
    return BaseLong(value.words | op.value.words);
#else
    return BaseLong(HighWord() | op.HighWord(), LowWord() | op.LowWord());
#endif // HAVE_64BIT_TYPES
}

BaseLong &BaseLong::operator|= (const BaseLong op)
{
    return *this = *this | op;
}

BaseLong BaseLong::operator& (const BaseLong op) const
{
#ifdef HAVE_64BIT_TYPES
    return BaseLong(value.words & op.value.words);
#else
    return BaseLong(HighWord() & op.HighWord(), LowWord() & op.LowWord());
#endif // HAVE_64BIT_TYPES
}

BaseLong &BaseLong::operator&= (const BaseLong op)
{
    return *this = *this & op;
}

bool BaseLong::operator&& (const BaseLong op) const
{
    return (*this != 0) && (op != 0);
}

bool BaseLong::operator|| (const BaseLong op) const
{
    return (*this != 0) || (op != 0);
}

BaseLong BaseLong::operator<< (int op) const
{
#ifdef HAVE_64BIT_TYPES
    // TODO: Does this work correctly?
    return BaseLong(value.words << op);
#else
    u4 n = op; // Always treat this value as positive

    //
    // Correct compilers treat x << 0 as x, and x << 32+ as 0.
    // But, since not all compilers follow these rules, we special-case on
    // the shift amount.
    //
    if (n == 0)
        return *this;
    if (n < 32)
    {
        u4 lo = LowWord();
        return BaseLong((HighWord() << n) | (lo >> (32 - n)), lo << n);
    }
    if (n == 32)
        return BaseLong(LowWord(), 0);
    if (n >= 64)
        return BaseLong(0);
    return BaseLong(LowWord() << (n - 32), 0);
#endif // HAVE_64BIT_TYPES
}

BaseLong &BaseLong::operator<<= (int op)
{
    return *this = *this << op;
}

BaseLong BaseLong::operator+ (const BaseLong op) const
{
#ifdef HAVE_64BIT_TYPES
    return BaseLong(value.words + op.value.words);
#else
    u4 ushort1 = (LowWord() & SHORT_MASK) + (op.LowWord() & SHORT_MASK),
       ushort2 = (ushort1 >> 16) + (LowWord() >> 16) + (op.LowWord() >> 16),
       ushort3 = (ushort2 >> 16) + (HighWord() & SHORT_MASK) + (op.HighWord() & SHORT_MASK),
       ushort4 = (ushort3 >> 16) + (HighWord() >> 16) + (op.HighWord() >> 16);

    return BaseLong((ushort3 & SHORT_MASK) | (ushort4 << 16), (ushort1 & SHORT_MASK) | (ushort2 << 16));
#endif // HAVE_64BIT_TYPES
}

BaseLong &BaseLong::operator+= (const BaseLong op)
{
    return *this = *this + op;
}

BaseLong BaseLong::operator++ (int)
{
    BaseLong temp = *this;
    *this += 1;
    return temp;
}

BaseLong BaseLong::operator++ ()
{
    return *this += 1;
}

BaseLong BaseLong::operator+ () const
{
    return *this;
}

BaseLong BaseLong::operator- () const
{
    return ~(*this) + 1;
}

BaseLong BaseLong::operator- (const BaseLong op) const
{
    return *this + (-op);
}

BaseLong &BaseLong::operator-= (const BaseLong op)
{
    return *this = *this - op;
}

BaseLong BaseLong::operator-- (int)
{
    BaseLong temp = *this;
    *this -= 1;
    return temp;
}

BaseLong BaseLong::operator-- ()
{
    return *this -= 1;
}

BaseLong BaseLong::operator* (const BaseLong op) const
{
#ifdef HAVE_64BIT_TYPES
    return BaseLong(value.words * op.value.words);
#else
    u4 x0 = LowWord()   & SHORT_MASK,
       x1 = LowWord()  >> 16,
       x2 = HighWord()  & SHORT_MASK,
       x3 = HighWord() >> 16,

       y0 = op.LowWord()   & SHORT_MASK,
       y1 = op.LowWord()  >> 16,
       y2 = op.HighWord()  & SHORT_MASK,
       y3 = op.HighWord() >> 16;

    BaseLong result  = BaseLong(0, x0 * y0),
             partial = BaseLong(0, x0 * y1);
    result += partial << 16;
    partial = BaseLong(0, x0 * y2);
    result += partial << 32;
    partial = BaseLong(0, x0 * y3);
    result += partial << 48;

    partial = BaseLong(0, x1 * y0);
    result += partial << 16;
    partial = BaseLong(0, x1 * y1);
    result += partial << 32;
    partial = BaseLong(0, x1 * y2);
    result += partial << 48;

    partial = BaseLong(0, x2 * y0);
    result += partial << 32;
    partial = BaseLong(0, x2 * y1);
    result += partial << 48;

    partial = BaseLong(0, x3 * y0);
    result += partial << 48;

    return result;
#endif // HAVE_64BIT_TYPES
}

BaseLong &BaseLong::operator*= (const BaseLong op)
{
    return *this = *this * op;
}


BaseLong::BaseLong(u4 high, u4 low)
{
    setHighAndLowWords(high, low);
}

BaseLong::BaseLong(u4 a)
{
    setHighAndLowWords(0, a);
}

BaseLong::BaseLong(i4 a)
{
    //
    // Since the carry bit is not guaranteed to ripple, we cannot use this code.
    //
    //        a >> 31;
    //
    setHighAndLowWords(a < 0 ? 0xFFFFFFFF : 0x00000000, a);
}


void BaseLong::Divide(const BaseLong &dividend, const BaseLong &divisor,
                      BaseLong &quotient, BaseLong &remainder)
{
#ifdef HAVE_64BIT_TYPES
    quotient = BaseLong(dividend.value.words / divisor.value.words);
    remainder = BaseLong(dividend.value.words % divisor.value.words);
#else
    u4 high = dividend.HighWord(),
       low  = dividend.LowWord(),
       remainder_high = 0;

    for (int i = 0; i < 32; i++)
    {
        remainder_high = (remainder_high << 1) | (high >> 31);
        high <<= 1;
        if ((ULongInt) divisor <= remainder_high)
        {
            high++;
            remainder_high -= divisor.LowWord();
        }
    }

    remainder = BaseLong(0, remainder_high);

    for (int j = 0; j < 32; j++)
    {
        remainder <<= 1;
        remainder.setLowWord(remainder.LowWord() | (low >> 31));
        low <<= 1;
        if ((ULongInt) divisor <= remainder)
        {
            low++;
            remainder -= divisor;
        }
    }

    quotient = BaseLong(high, low);
#endif // HAVE_64BIT_TYPES
}


ULongInt &ULongInt::operator/= (const ULongInt op)
{
    return *this = *this / op;
}


ULongInt ULongInt::operator/ (const ULongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    return ULongInt(value.words / op.value.words);
#else
    BaseLong quotient,
             remainder;

    Divide(*this, op, quotient, remainder);

    return quotient;
#endif // HAVE_64BIT_TYPES
}

ULongInt ULongInt::operator% (const ULongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    return ULongInt(value.words % op.value.words);
#else
    BaseLong quotient,
             remainder;

    Divide(*this, op, quotient, remainder);

    return remainder;
#endif // HAVE_64BIT_TYPES
}

ULongInt &ULongInt::operator%= (const ULongInt op)
{
    return *this = *this % op;
}


ULongInt ULongInt::operator>> (int op) const
{
#ifdef HAVE_64BIT_TYPES
    // TODO: Does this work correctly?
    return ULongInt(value.words >> op);
#else
    u4 n = op; // Always treat this value as positive

    //
    // Correct compilers treat x >> as x, and x >> 32+ as 0.
    // But, since not all compilers follow these rules, we special-case on
    // the shift amount.
    //

    if (n == 0)
        return *this;
    if (n < 32)
    {
        u4 hi = HighWord();
        return ULongInt(hi >> n, (hi << (32 - n)) | (LowWord() >> n));
    }
    if (n == 32)
        return ULongInt(0, HighWord());
    if (n >= 64)
        return ULongInt(0);
    return ULongInt(0, HighWord() >> (n - 32));
#endif // HAVE_64BIT_TYPES
}

ULongInt &ULongInt::operator>>= (int op)
{
    return *this = *this >> op;
}

bool ULongInt::operator< (const ULongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    return value.words < op.value.words;
#else
    u4 a = HighWord(), b = op.HighWord();
    return (a == b ? LowWord() < op.LowWord() : a < b);
#endif // HAVE_64BIT_TYPES
}

bool ULongInt::operator<= (const ULongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    return value.words <= op.value.words;
#else
    return (*this < op) || (*this == op);
#endif // HAVE_64BIT_TYPES
}

bool ULongInt::operator> (const ULongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    return value.words > op.value.words;
#else
    u4 a = HighWord(), b = op.HighWord();
    return (a == b ? LowWord() > op.LowWord() : a > b);
#endif // HAVE_64BIT_TYPES
}

bool ULongInt::operator>= (const ULongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    return value.words >= op.value.words;
#else
    return (*this > op) || (*this == op);
#endif // HAVE_64BIT_TYPES
}

const LongInt *LongInt::max_long_const = NULL;
const LongInt *LongInt::min_long_const = NULL;

LongInt::LongInt(const IEEEfloat &f)
{
    *this = f.LongValue();
}

LongInt::LongInt(const IEEEdouble &d)
{
    *this = d.LongValue();
}

LongInt LongInt::operator/ (const LongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    bool negative_dividend = (i8) value.words < 0,
         negative_divisor  = (i8) op.value.words < 0;

    u8 a = negative_dividend ? -(i8) value.words : value.words,
       b = negative_divisor  ? -(i8) op.value.words : op.value.words;

    return LongInt((negative_dividend ^ negative_divisor) ? -(a / b) : a / b);
#else
    bool negative_dividend = ((HighWord() & 0x80000000) != 0),
         negative_divisor  = ((op.HighWord() & 0x80000000) != 0);

    BaseLong a = (negative_dividend ? -(*this) : (BaseLong) *this),
             b = (negative_divisor  ? -(op)    : (BaseLong) op),
             quotient,
             remainder;

    Divide(a, b, quotient, remainder);

    return (negative_dividend ^ negative_divisor ? -quotient : quotient);
#endif // HAVE_64BIT_TYPES
}

LongInt &LongInt::operator/= (const LongInt op)
{
    return *this = *this / op;
}

LongInt LongInt::operator% (const LongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    bool negative_dividend = (i8) value.words < 0,
         negative_divisor  = (i8) op.value.words < 0;

    u8 a = negative_dividend ? -(i8) value.words : value.words,
       b = negative_divisor  ? -(i8) op.value.words : op.value.words;

    return LongInt(negative_dividend ? -(a % b) : a % b);
#else
    bool negative_dividend = ((HighWord() & 0x80000000) != 0),
         negative_divisor  = ((op.HighWord() & 0x80000000) != 0);

    BaseLong a = (negative_dividend ? -(*this) : (BaseLong) *this),
             b = (negative_divisor  ? -(op)    : (BaseLong) op),
             quotient,
             remainder;

    Divide(a, b, quotient, remainder);

    return (negative_dividend ? -remainder : remainder);
#endif // HAVE_64BIT_TYPES
}

LongInt &LongInt::operator%= (const LongInt op)
{
    return *this = *this % op;
}

LongInt LongInt::operator>> (int op) const
{
#ifdef HAVE_64BIT_TYPES
    // TODO: Does this work correctly?
    return LongInt((u8) ((i8) value.words >> op));
#else
    u4 n = op; // Always treat this value as positive

    //
    // Correct compilers treat x >> 0 as x, and x >> 32+ as x<0 ? -1 : 0.
    // But, since not all compilers follow these rules, we special-case on
    // the shift amount.
    //

    if (n == 0)
        return *this;

    i4 hi = HighWord();
    u4 shift = (hi & SIGN_BIT) ? 0xffffffff : 0;

    if (n < 32)
         return LongInt(hi >> n, (hi << (32 - n)) | (LowWord() >> n));
    if (n == 32)
        return LongInt(shift, hi);
    if (n >= 64)
        return LongInt(shift, shift);
    return LongInt(shift, hi >> (n - 32));
#endif // HAVE_64BIT_TYPES
}

LongInt &LongInt::operator>>= (int op)
{
    return *this = *this >> op;
}

bool LongInt::operator< (const LongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    return (i8) value.words < (i8) op.value.words;
#else
    i4 a = HighWord(), b = op.HighWord();
    return (a == b) ? LowWord() < op.LowWord() : a < b;
#endif // HAVE_64BIT_TYPES
}

bool LongInt::operator<= (const LongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    return (i8) value.words <= (i8) op.value.words;
#else
    return (*this < op) || (*this == op);
#endif // HAVE_64BIT_TYPES
}

bool LongInt::operator> (const LongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    return (i8) value.words > (i8) op.value.words;
#else
    i4 a = HighWord(), b = op.HighWord();
    return (a == b) ? LowWord() > op.LowWord() : a > b;
#endif // HAVE_64BIT_TYPES
}

bool LongInt::operator>= (const LongInt op) const
{
#ifdef HAVE_64BIT_TYPES
    return (i8) value.words >= (i8) op.value.words;
#else
    return (*this > op) || (*this == op);
#endif // HAVE_64BIT_TYPES
}


} // Close namespace Jikes block

