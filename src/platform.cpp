// NOTE: The code for accurate conversions between floating point
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

#include "platform.h"
#include "long.h"
#include "double.h"


namespace Jopa { // Open namespace Jopa block
// Define the PathSeparator() function with the proper
// impl for each platform.

#ifdef HAVE_PATHNAME_STYLE_DOS
char PathSeparator() { return U_SEMICOLON; } // ";"
#else
char PathSeparator() { return U_COLON; } // ":"
#endif // ! HAVE_PATHNAME_STYLE_DOS


// Define the SystemMkdir() function with the proper
// impl for each platform.

int SystemMkdir(char* dirname)
{
#ifdef HAVE_GLIBC_MKDIR
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO);
#endif // HAVE_GLIBC_MKDIR

#ifdef HAVE_LIBC5_MKDIR
    reteurn mkdir(dirname, S_IRWXU);
#endif // HAVE_LIBC5_MKDIR

#ifdef HAVE_WIN32_MKDIR
    return mkdir(dirname);
#endif // HAVE_WIN32_MKDIR

#ifdef HAVE_MAC_MKDIR
    return mkdir(dirname, 0);
#endif // HAVE_MAC_MKDIR
    assert(false);
    return 0;
}

//
// The configure script checks each of these to see if we need our own
// implementation.
//
#ifndef HAVE_WCSLEN
size_t wcslen(const wchar_t* cs)
{
    int n = 0;
    while (*cs++)
        n++;
    return n;
}
#endif // HAVE_WCSLEN

#ifndef HAVE_WCSCPY
wchar_t* wcscpy(wchar_t* s, const wchar_t* ct)
{
    wchar_t* ptr;
    for (ptr = s; *ct; ptr++, ct++)
        *ptr = *ct;
    *ptr = U_NULL;
    return s;
}
#endif // HAVE_WCSCPY

#ifndef HAVE_WCSNCPY
wchar_t* wcsncpy(wchar_t* s, const wchar_t* ct, size_t n)
{
    wchar_t* ptr;
    for (ptr = s; *ct && n-- > 0; ptr++, ct++)
        *ptr = *ct;
    while (n-- > 0)
        *ptr++ = U_NULL;
    return s;
}
#endif // HAVE_WCSNCPY

#ifndef HAVE_WCSCAT
wchar_t* wcscat(wchar_t* s, const wchar_t* ct)
{
    wchar_t* ptr = s;
    while (*ptr)
        ptr++;
    wcscpy(ptr, ct);
    return s;
}
#endif // HAVE_WCSCAT

#ifndef HAVE_WCSCMP
int wcscmp(const wchar_t* cs, const wchar_t* ct)
{
    while (*cs == *ct && *cs && *ct)
    {
        cs++;
        ct++;
    }
    return (*cs == *ct ? 0 : (*cs < *ct ? -1 : 1));
}
#endif // HAVE_WCSCMP

#ifndef HAVE_WCSNCMP
int wcsncmp(const wchar_t* cs, const wchar_t* ct, size_t n)
{
    while (*cs == *ct && *cs && *ct && n-- > 0)
    {
        cs++;
        ct++;
    }
    return (n <= 0 || *cs == *ct ? 0 : (*cs < *ct ? -1 : 1));
}
#endif // HAVE_WCSNCMP


//
// If the system runs out of memory, this function is invoked
// This is tricky because VC++ on windows uses a non standard
// implementation of the set_new_handler function.
//
#ifdef HAVE_VCPP_SET_NEW_HANDLER
int OutOfMemory(size_t)
#else
void OutOfMemory()
#endif // ! HAVE_VCPP_SET_NEW_HANDLER
{
    fprintf(stderr, "***System Failure: Out of memory\n");
    exit(1);
#ifdef HAVE_VCPP_SET_NEW_HANDLER
    return 0;
#endif // HAVE_VCPP_SET_NEW_HANDLER
}

void SetNewHandler()
{
#ifdef HAVE_VCPP_SET_NEW_HANDLER
    _set_new_handler(OutOfMemory);
#else
    set_new_handler(OutOfMemory);
#endif // ! HAVE_VCPP_SET_NEW_HANDLER
}


//
// When using the ICC compiler on Win95 or OS/2, we need to disable
// testing for various floating point exceptions. Default behavior
// was causing problems reading some standard class files.
//
// We obviously don't need this on AIX (non x86), which uses xlC/ICC
//
void FloatingPointCheck()
{
#ifdef HAVE_ICC_FP_BUGS
    _control87(EM_UNDERFLOW, EM_UNDERFLOW);
    _control87(EM_ZERODIVIDE, EM_ZERODIVIDE);
    _control87(EM_OVERFLOW, EM_OVERFLOW);
    _control87(EM_INVALID, EM_INVALID);
#endif // HAVE_ICC_FP_BUGS
}


//
// This next set of functions may need some porting to work on various systems
//
int SystemStat(const char* name, struct stat* stat_struct)
{
    int result = ::stat(name, stat_struct);
#ifdef HAVE_SYS_CYGWIN_H
    //
    // Up through cygwin 1.3.10, the hash function which determines inodes
    // was not strong enough, so java/net and java/nio occasionally get the
    // same inode without this hack.
    //
    if (result == 0)
        stat_struct -> st_ino += name[strlen(name) - 1];
#endif // HAVE_SYS_CYGWIN_H
    return result;
}

FILE* SystemFopen(const char* name, const char* mode)
{
    return fopen(name, mode);
}

size_t SystemFread(char* ptr, size_t element_size, size_t count, FILE* stream)
{
    return fread(ptr, element_size, count, stream);
}

int SystemIsDirectory(char* name)
{
    struct stat status;
    return ((SystemStat(name, &status) == 0 &&
             (status.st_mode & JOPA_STAT_S_IFDIR)) ? 1 : 0);
}

int SystemMkdirhier(char* dirname)
{
    if (SystemIsDirectory(dirname))
        return 0;

    for (char* ptr = dirname; *ptr; ptr++)
    {
        char delimiter = *ptr;
        if (delimiter == U_SLASH)
        {
            *ptr = U_NULL;
            if (! SystemIsDirectory(dirname))
                SystemMkdir(dirname);
            *ptr = delimiter;
        }
    }
    SystemMkdir(dirname);
    return (! SystemIsDirectory(dirname));
}


// Create the specified directory and also any missing parent directories.
int SystemMkdirhierForFile(char* filename)
{
    for (int i = strlen(filename); i >= 0; i--)
    {
        if (filename[i] == U_SLASH)
        {
            int result = 0;
            filename[i] = U_NULL;
            if (! SystemIsDirectory(filename))
            {
                Ostream() << "making directory " << filename << "\n";
                result = SystemMkdirhier(filename);
            }
            filename[i] = U_SLASH;
            return result;
        }
    }
    return 0;
}


// FIXME: These next two should definitely be inlined; but when I
// add the "inline" keyword , I get linker problems.


// Given three strings, return a newly-allocated string which is their
// concatenation.
char* strcat3(const char* prefix, const char* middle, const char* suffix)
{
    int prefix_len = strlen(prefix);
    int prefix_middle_len = prefix_len + strlen(middle);

    char* result = new char[prefix_middle_len + strlen(suffix) + 1];
    strcpy(result, prefix);
    // The below is more efficient than this commented-out code.
    // strcat(result, middle);
    // strcat(result, suffix);
    strcpy(result + prefix_len, middle);
    strcpy(result + prefix_middle_len, suffix);
    return result;
}

// It's inconceivable that this is the right way to go about this.
// One alternative is to use ConvertUnicodeToUtf8.
char* wstring2string(wchar_t* in)
{
    char* result = new char[wcslen(in) + 1];
    result[wcslen(in)] = 0;
    for (size_t i=0; i<wcslen(in); i++) {
        wchar_t ch = in[i];
        result[i] = (ch >> 8 == 0 ? (char)ch : '?');
    }
    return result;
}


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// End of platform specific defines in this file, the rest of the code
// in this file should work on any system
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


IntToString::IntToString(i4 num)
{
    str = &info[TAIL_INDEX];
    *str = U_NULL;
    u4 n = num < 0 ? - num : num;
    do
    {
        *--str = U_0 + n % 10;
        n /= 10;
    } while (n != 0);
    if (num < 0)
        *--str = U_MINUS;
}

IntToString::IntToString(u4 num, int width)
{
    str = &info[width];
    *str = U_NULL;
    do
    {
        char c = num & 0xf;
        *--str = c < 10 ? U_0 + c : U_a - 10 + c;
        num >>= 4;
    } while (str != info);
}


IntToWstring::IntToWstring(i4 num)
{
    wstr = &winfo[TAIL_INDEX];
    *wstr = U_NULL;
    u4 n = num < 0 ? - num : num;
    do
    {
        *--wstr = U_0 + n % 10;
        n /= 10;
    } while (n != 0);
    if (num < 0)
        *--wstr = U_MINUS;
}


LongToString::LongToString(const LongInt& num)
{
    str = &info[TAIL_INDEX];
    *str = U_NULL;
    ULongInt n = num < 0 ? (ULongInt) - num : (ULongInt) num;
    do
    {
        *--str = U_0 + (n % 10).LowWord();
        n /= 10;
    } while (n != 0);
    if (num.HighWord() & 0x80000000)
        *--str = U_MINUS;
    base = str;
}

LongToString::LongToString(const ULongInt& num)
{
    str = &info[TAIL_INDEX];
    *str = U_NULL;
    ULongInt n = num;
    do
    {
        *--str = U_0 + (n % 10).LowWord();
        n /= 10;
    } while (n != 0);
    base = str;
}

LongToString::LongToString(const BaseLong& num, bool octal)
{
    str = &info[TAIL_INDEX];
    *str = U_NULL;
    ULongInt value = num;
    do
    {
        char c = value.LowWord() & (octal ? 7 : 0xf);
        *--str = c < 10 ? U_0 + c : U_a - 10 + c;
        value >>= (octal ? 3 : 4);
    } while (value != 0);
    base = str - 1;
    if (! octal)
        *base-- = U_x;
    *base = U_0;
}


//
// Convert an double to its character string representation.
//
FloatToString::FloatToString(const IEEEfloat& f)
{
    int bbits, b2, b5, be, i,
        j, j1, k, m2, m5, s2, s5;
    bool neg,      // f is negative
        k_check,   // need to check if k is near power of ten
        spec_case, // f is normalized power of two
        denorm,    // f is denormalized
        round;     // round trailing 9's up
    IEEEfloat fs, f1;
    char* s;
    char dig;

    //
    // Start with exceptional cases: zero, infinity, NaN
    //
    neg = f.IsNegative();
    if (f.IsNaN())
    {
        strcpy(str, StringConstant::U8S_NaN);
        length = strlen(str);
        return;
    }
    else if (f.IsInfinite())
    {
        if (neg)
            strcpy(str, StringConstant::U8S_neg_Infinity);
        else
            strcpy(str, StringConstant::U8S_pos_Infinity);
        length = strlen(str);
        return;
    }
    else if (f.IsZero())
    {
        if (neg)
            strcpy(str, StringConstant::U8S_neg_Zero);
        else
            strcpy(str, StringConstant::U8S_pos_Zero);
        length = strlen(str);
        return;
    }

    //
    // Create BigInt holding f.
    // bbits = # significant bits in f
    // be = log2(least significant bit)
    // i = log2(most significant bit)
    // f1 = mantissa of f
    // Therefore, f == f1 * 2**i, and i == be + bbits - 1.
    //
    s = str;
    BigInt b(f, be, bbits);
    u4 x;
    i = f.SplitInto(x);
    f1 = IEEEfloat((i4) x) / (1 << IEEEfloat::FractSize());
    denorm = i <= -IEEEfloat::Bias();
    //
    // log(x)   ~=~ log(1.5) + (x-1.5)/1.5
    // log10(x)  =  log(x) / log(10)
    //          ~=~ log(1.5)/log(10) + (x-1.5)/(1.5*log(10))
    // log10(f)  =  log10(f1 * 2**i)
    //           =  i*log10(2) + log10(f1)
    //
    // This suggests computing an approximation k to log10(f) by
    //
    // k = i*0.30103 + ( 0.17609125 + (f1-1.5)*0.28952965 );
    //
    // We want k to be too large rather than too small.
    // The error in the first-order Taylor series approximation
    // is in our favor, so we just round up the constant enough
    // to compensate for any error in the multiplication of
    // i by 0.30103; since |i| <= 152,
    // and 152 * 0.30103 * 2^-23 ~=~ 5.5e-6,
    // adding 1e-5 to the constant term more than suffices.
    // Hence we adjust the constant term to 0.1761.
    // (We could get a more accurate k by invoking log10,
    //  but this is probably not worthwhile.)
    //
    fs = IEEEfloat(i) * 0.30103f + 0.1761f + (f1 - 1.5f) * 0.28952965f;
    k = fs.IntValue();
    f1 = f.IsNegative() ? -f : f;
    k_check = true;
    if (fs < 0 && fs != k)
        k--;
    else if (k >= 0 && k <= 10)
    {
        if (f1 < IEEEfloat::tens[k])
            k--;
        k_check = false;
    }

    //
    // We have an integer (no fraction) represented in 24 bits.
    // For this special case, math on floats has no rounding errors.
    //
    if (be >= 0 && k <= 6)
    {
        fs = IEEEfloat::tens[k];
        do
        {
            dig = (char) (f1 / fs).IntValue();
            f1 -= fs * (i4) dig;
            *s++ = U_0 + dig;
        } while ((f1 *= 10) != 0);
        Format(s, k, neg);
        return;
    }

    //
    // Begin work. Find S = 2**s2 * 5**s5, and b adjustment 2**b2 * 5**b5,
    // that will be needed later on.
    //
    if (be <= 0)
    {
        b2 = 0;
        s2 = -be;
    }
    else
    {
        b2 = be;
        s2 = 0;
    }
    if (k >= 0)
    {
        b5 = 0;
        s5 = k;
        s2 += k;
    }
    else
    {
        b2 -= k;
        b5 = -k;
        s5 = 0;
    }

    m2 = b2;
    m5 = b5;
    i = denorm ? be + IEEEfloat::Bias() + IEEEfloat::FractSize()
               : 2 + IEEEfloat::FractSize() - bbits;
    b2 += i;
    s2 += i;
    BigInt mhi(1);
    if (m2 > 0 && s2 > 0)
    {
        i = m2 < s2 ? m2 : s2;
        b2 -= i;
        m2 -= i;
        s2 -= i;
    }
    if (b5 > 0)
    {
        if (m5 > 0)
        {
            mhi.pow5mult(m5);
            b *= mhi;
        }
        if ((j = b5 - m5) != 0)
            b.pow5mult(j);
    }
    BigInt S(1);
    if (s5 > 0)
        S.pow5mult(s5);
    spec_case = false;
    if (! (f.FractBits()) && f.Exponent())
    {
        b2++;
        s2++;
        spec_case = true;
    }

    // Arrange for convenient computation of quotients:
    // shift left if necessary so divisor has 4 leading 0 bits.
    //
    // Perhaps we should just compute leading 28 bits of S once
    // and for all and pass them and a shift to quorem, so it
    // can do shifts and ors to compute the numerator for q.
    //
    if ((i = ((s5 ? 32 - S.hi0bits() : 1) + s2) & 0x1f) != 0)
        i = 32 - i;
    if (i > 4)
    {
        i -= 4;
        b2 += i;
        m2 += i;
        s2 += i;
    }
    else if (i < 4)
    {
        i += 28;
        b2 += i;
        m2 += i;
        s2 += i;
    }
    if (b2 > 0)
        b <<= b2;
    if (s2 > 0)
        S <<= s2;
    if (k_check && b.compareTo(S) < 0)
    {
        k--;
        b *= 10;
        mhi *= 10;
    }
    if (m2 > 0)
        mhi <<= m2;
    BigInt mlo(mhi);
    if (spec_case)
        mhi = mlo << 1;
    round = false;
    while (true)
    {
        dig = (char) b.quorem(S) + U_0;
        //
        // Do we have the shortest decimal string that will round to f?
        //
        j = b.compareTo(mlo);
        BigInt delta = S - mhi;
        j1 = delta.IsNegative() ? 1 : b.compareTo(delta);
        if (j1 == 0 && ! (f.value.word & 1))
        {
            if (dig == U_9)
                round = true;
            else if (j > 0)
                dig++;
            *s++ = dig;
            break;
        }
        if ((j < 0 || j == 0 && ! (f.value.word & 1)) && s != str)
        {
            if (! b.IsZero() && j1 > 0)
            {
                b <<= 1;
                j1 = b.compareTo(S);
                if ((j1 > 0 || j1 == 0 && dig & 1) && dig++ == U_9)
                {
                    *s++ = U_9;
                    round = true;
                    break;
                }
            }
            *s++ = dig;
            break;
        }
        if (j1 > 0 && s != str)
        {
            if (dig == U_9)
            {
                *s++ = U_9;
                round = true;
            }
            else
                *s++ = dig + 1;
            break;
        }
        *s++ = dig;
        b *= 10;
        mlo *= 10;
        mhi *= 10;
    }
    if (round)
    {
        while (*--s == U_9)
            if (s == str)
            {
                k++;
                *s = U_0;
                break;
            }
        ++*s++;
    }
    Format(s, k, neg);
}

void FloatToString::Format(char* s, int exp, bool neg)
{
    //
    // at this point, str contains just the precise digits in the answer,
    // and s points to the slot just after the last digit
    //
    length = s - str + 1; // strlen(str) + '.'
    bool eneg;
    int i;
    switch (exp)
    {
    case -3: case -2: case -1:
        // remove final trailing 0, not needed in this format
        if (*(s - 1) == U_0)
        {
            length--;
            s--;
        }
        s--;
        // add enough of leading "0.00"
        length += -exp;
        do
            *(s + (neg ? 2 : 1) - exp) = *s;
        while (s-- != str);
        for (i = (neg ? 1 : 0); i < (neg ? 2 : 1) - exp; i++)
            str[i] = U_0;
        if (neg)
            str[0] = U_MINUS;
        str[neg ? 2 : 1] = U_DOT;
        break;
    case 0: case 1: case 2: case 3:
    case 4: case 5: case 6:
        while (length < exp + 3)
            // add trailing '0's
            str[length++ - 1] = U_0;
        s = &str[length - 2];
        do
            *(s + (neg ? 2 : 1)) = *s;
        while (s-- != str + exp + 1);
        if (neg)
        {
            do
                *(s + 1) = *s;
            while (s-- != str);
            str[0] = U_MINUS;
        }
        str[exp + (neg ? 2 : 1)] = U_DOT;
        break;
    default:
        if (length == 2)
            // add trailing '0', so at least one digit follows '.'
            str[length++ - 1] = U_0;
        eneg = exp < 0;
        if (eneg)
        {
            length++; // exponent '-'
            exp = -exp;
        }
        if (exp < 10)
            length += 2; // 'E' + 1 digit exponent
        else if (exp < 100)
            length += 3; // 'E' + 2 digit exponent
        else
            assert (! "unexpected exponent");
        s = &str[length + (neg ? 1 : 0)];
        do
            *--s = exp % 10 + U_0;
        while ((exp /= 10) != 0);
        if (eneg)
            *--s = U_MINUS;
        *--s = U_E;
        --s;
        do
            *s = *(s - (neg ? 2 : 1)); // shift digits right, to add '.'
        while (--s != str + (neg ? 2 : 1));
        if (neg)
        {
            str[1] = str[0];
            str[0] = U_MINUS;
        }
        str[neg ? 2 : 1] = U_DOT;
    }
    if (neg)
        length++;
    str[length] = U_NULL;
    assert(length <= MAXIMUM_STR_LENGTH);
}


//
// Convert an double to its character string representation.
//
DoubleToString::DoubleToString(const IEEEdouble& d)
{
    int bbits, b2, b5, be, i,
        j, j1, k, m2, m5, s2, s5;
    bool neg,      // f is negative
        k_check,   // need to check if k is near power of ten
        spec_case, // f is normalized power of two
        denorm,    // f is denormalized
        round;     // round trailing 9's up
    IEEEdouble ds, d1;
    char* s;
    char dig;

    //
    // Start with exceptional cases: zero, infinity, NaN
    //
    neg = d.IsNegative();
    if (d.IsNaN())
    {
        strcpy(str, StringConstant::U8S_NaN);
        length = strlen(str);
        return;
    }
    else if (d.IsInfinite())
    {
        if (neg)
            strcpy(str, StringConstant::U8S_neg_Infinity);
        else
            strcpy(str, StringConstant::U8S_pos_Infinity);
        length = strlen(str);
        return;
    }
    else if (d.IsZero())
    {
        if (neg)
            strcpy(str, StringConstant::U8S_neg_Zero);
        else
            strcpy(str, StringConstant::U8S_pos_Zero);
        length = strlen(str);
        return;
    }

    //
    // Create BigInt holding d.
    // bbits = # significant bits in d
    // be = log2(least significant bit)
    // i = log2(most significant bit)
    // d1 = mantissa of d
    // Therefore, d == d1 * 2**i, and i == be + bbits - 1.
    //
    s = str;
    BigInt b(d, be, bbits);
    LongInt x;
    i = d.SplitInto(x);
    d1 = IEEEdouble(x) /
        IEEEdouble(LongInt(LongInt(1) << IEEEdouble::FractSize()));
    denorm = i <= -IEEEdouble::Bias();
    //
    // log(x)   ~=~ log(1.5) + (x-1.5)/1.5
    // log10(x)  =  log(x) / log(10)
    //          ~=~ log(1.5)/log(10) + (x-1.5)/(1.5*log(10))
    // log10(d)  =  log10(d2 * 2**i)
    //           =  i*log10(2) + log10(d2)
    //
    // This suggests computing an approximation k to log10(d) by
    //
    // k = i*0.301029995663981
    //   + ( 0.176091259055681 + (d2-1.5)*0.289529654602168 );
    //
    // We want k to be too large rather than too small.
    // The error in the first-order Taylor series approximation
    // is in our favor, so we just round up the constant enough
    // to compensate for any error in the multiplication of
    // i by 0.301029995663981; since |i| <= 1077,
    // and 1077 * 0.30103 * 2^-52 ~=~ 7.2e-14,
    // adding 1e-13 to the constant term more than suffices.
    // Hence we adjust the constant term to 0.1760912590558.
    // (We could get a more accurate k by invoking log10,
    //  but this is probably not worthwhile.)
    //
    ds = IEEEdouble(i) * 0.301029995663981 + 0.1760912590558
        + (d1 - 1.5) * 0.289529654602168;
    k = ds.IntValue();
    d1 = d.IsNegative() ? -d : d;
    k_check = true;
    if (ds < 0 && ds != k)
        k--;
    else if (k >= 0 && k <= 22)
    {
        if (d1 < IEEEdouble::tens[k])
            k--;
        k_check = false;
    }

    //
    // We have an integer (no fraction) represented in 53 bits.
    // For this special case, math on doubles has no rounding errors.
    //
    if (be >= 0 && k <= 14)
    {
        ds = IEEEdouble::tens[k];
        do
        {
            dig = (char) (d1 / ds).IntValue();
            d1 -= ds * (i4) dig;
            *s++ = U_0 + dig;
        } while ((d1 *= 10) != 0);
        Format(s, k, neg);
        return;
    }

    //
    // Begin work. Find S = 2**s2 * 5**s5, and b adjustment 2**b2 * 5**b5,
    // that will be needed later on.
    //
    if (be <= 0)
    {
        b2 = 0;
        s2 = -be;
    }
    else
    {
        b2 = be;
        s2 = 0;
    }
    if (k >= 0)
    {
        b5 = 0;
        s5 = k;
        s2 += k;
    }
    else
    {
        b2 -= k;
        b5 = -k;
        s5 = 0;
    }

    m2 = b2;
    m5 = b5;
    i = denorm ? be + IEEEdouble::Bias() + IEEEdouble::FractSize()
               : 2 + IEEEdouble::FractSize() - bbits;
    b2 += i;
    s2 += i;
    BigInt mhi(1);
    if (m2 > 0 && s2 > 0)
    {
        i = m2 < s2 ? m2 : s2;
        b2 -= i;
        m2 -= i;
        s2 -= i;
    }
    if (b5 > 0)
    {
        if (m5 > 0)
        {
            mhi.pow5mult(m5);
            b *= mhi;
        }
        if ((j = b5 - m5) != 0)
            b.pow5mult(j);
    }
    BigInt S(1);
    if (s5 > 0)
        S.pow5mult(s5);
    spec_case = false;
    if (! (d.FractBits()) && d.Exponent())
    {
        b2++;
        s2++;
        spec_case = true;
    }

    // Arrange for convenient computation of quotients:
    // shift left if necessary so divisor has 4 leading 0 bits.
    //
    // Perhaps we should just compute leading 28 bits of S once
    // and for all and pass them and a shift to quorem, so it
    // can do shifts and ors to compute the numerator for q.
    //
    if ((i = ((s5 ? 32 - S.hi0bits() : 1) + s2) & 0x1f) != 0)
        i = 32 - i;
    if (i > 4)
    {
        i -= 4;
        b2 += i;
        m2 += i;
        s2 += i;
    }
    else if (i < 4)
    {
        i += 28;
        b2 += i;
        m2 += i;
        s2 += i;
    }
    if (b2 > 0)
        b <<= b2;
    if (s2 > 0)
        S <<= s2;
    if (k_check && b.compareTo(S) < 0)
    {
        k--;
        b *= 10;
        mhi *= 10;
    }
    if (m2 > 0)
        mhi <<= m2;
    BigInt mlo(mhi);
    if (spec_case)
        mhi = mlo << 1;
    round = false;
    while (true)
    {
        dig = (char) b.quorem(S) + U_0;
        //
        // Do we have the shortest decimal string that will round to d?
        //
        j = b.compareTo(mlo);
        BigInt delta = S - mhi;
        j1 = delta.IsNegative() ? 1 : b.compareTo(delta);
        if (j1 == 0 && ! (d.LowWord() & 1))
        {
            if (dig == U_9)
                round = true;
            else if (j > 0)
                dig++;
            *s++ = dig;
            break;
        }
        if ((j < 0 || j == 0 && ! (d.LowWord() & 1)) && s != str)
        {
            if (! b.IsZero() && j1 > 0)
            {
                b <<= 1;
                j1 = b.compareTo(S);
                if ((j1 > 0 || j1 == 0 && dig & 1) && dig++ == U_9)
                {
                    *s++ = U_9;
                    round = true;
                    break;
                }
            }
            *s++ = dig;
            break;
        }
        if (j1 > 0 && s != str)
        {
            if (dig == U_9)
            {
                *s++ = U_9;
                round = true;
            }
            else
                *s++ = dig + 1;
            break;
        }
        *s++ = dig;
        b *= 10;
        mlo *= 10;
        mhi *= 10;
    }
    if (round)
    {
        while (*--s == U_9)
            if (s == str)
            {
                k++;
                *s = U_0;
                break;
            }
        ++*s++;
    }
    Format(s, k, neg);
}

void DoubleToString::Format(char* s, int exp, bool neg)
{
    //
    // at this point, str contains just the precise digits in the answer,
    // and s points to the slot just after the last digit
    //
    length = s - str + 1; // strlen(str) + '.'
    bool eneg;
    int i;
    switch (exp)
    {
    case -3: case -2: case -1:
        // remove final trailing 0, not needed in this format
        if (*(s - 1) == U_0)
        {
            length--;
            s--;
        }
        s--;
        // add enough of leading "0.00"
        length += -exp;
        do
            *(s + (neg ? 2 : 1) - exp) = *s;
        while (s-- != str);
        for (i = (neg ? 1 : 0); i < (neg ? 2 : 1) - exp; i++)
            str[i] = U_0;
        if (neg)
            str[0] = U_MINUS;
        str[neg ? 2 : 1] = U_DOT;
        break;
    case 0: case 1: case 2: case 3:
    case 4: case 5: case 6:
        while (length < exp + 3)
            // add trailing '0's
            str[length++ - 1] = U_0;
        s = &str[length - 2];
        do
            *(s + (neg ? 2 : 1)) = *s;
        while (s-- != str + exp + 1);
        if (neg)
        {
            do
                *(s + 1) = *s;
            while (s-- != str);
            str[0] = U_MINUS;
        }
        str[exp + (neg ? 2 : 1)] = U_DOT;
        break;
    default:
        if (length == 2)
            // add trailing '0', so at least one digit follows '.'
            str[length++ - 1] = U_0;
        eneg = exp < 0;
        if (eneg)
        {
            length++; // exponent '-'
            exp = -exp;
        }
        if (exp < 10)
            length += 2; // 'E' + 1 digit exponent
        else if (exp < 100)
            length += 3; // 'E' + 2 digit exponent
        else if (exp < 1000)
            length += 4; // 'E' + 3 digit exponent
        else
            assert (! "unexpected exponent");
        s = &str[length + (neg ? 1 : 0)];
        do
            *--s = exp % 10 + U_0;
        while ((exp /= 10) != 0);
        if (eneg)
            *--s = U_MINUS;
        *--s = U_E;
        --s;
        do
            *s = *(s - (neg ? 2 : 1)); // shift digits right, to add '.'
        while (--s != str + (neg ? 2 : 1));
        if (neg)
        {
            str[1] = str[0];
            str[0] = U_MINUS;
        }
        str[neg ? 2 : 1] = U_DOT;
    }
    if (neg)
        length++;
    str[length] = U_NULL;
    assert(length <= MAXIMUM_STR_LENGTH);
}


Ostream& Ostream::operator<<(LongInt a)
{
    if (os -> flags() & os -> dec)
    {
        LongToString long_int(a);
        *os << long_int.String();
    }
    else if (os -> flags() & os -> oct)
    {
        LongToString long_int(a, true);
        *os << (os -> flags() & os -> showbase
                ? long_int.StringWithBase() : long_int.String());
    }
    else if (os -> flags() & os -> hex)
    {
        LongToString long_int(a, false);
        *os << (os -> flags() & os -> showbase
                ? long_int.StringWithBase() : long_int.String());
    }
    else
    {
         os -> flush();
         assert(false && "invalid format for printing signed long");
    }

    return *this;
}

Ostream& Ostream::operator<<(ULongInt a)
{
    if (os -> flags() & os -> dec)
    {
        LongToString ulong_int(a);
        *os << ulong_int.String();
    }
    else if (os -> flags() & os -> oct)
    {
        LongToString ulong_int(a, true);
        *os << (os -> flags() & os -> showbase
                ? ulong_int.StringWithBase() : ulong_int.String());
    }
    else if (os -> flags() & os -> hex)
    {
        LongToString ulong_int(a, false);
        *os << (os -> flags() & os -> showbase
                ? ulong_int.StringWithBase() : ulong_int.String());
    }
    else
    {
        os -> flush();
        assert(false && "invalid format for printing unsigned long");
    }

    return *this;
}

//
// Punctuation and operators
//
const wchar_t StringConstant::US_AND[] = {U_AM, U_NU}; // L"&"
const wchar_t StringConstant::US_AND_AND[] = {U_AM, U_AM, U_NU}; // L"&&"
const wchar_t StringConstant::US_AND_EQUAL[] = {U_AM, U_EQ, U_NU}; // L"&="
const wchar_t StringConstant::US_AT[] = {U_AT, U_NU}; // L"@"
const wchar_t StringConstant::US_COLON[] = {U_CO, U_NU}; // L":"
const wchar_t StringConstant::US_COMMA[] = {U_CM, U_NU}; // L","
const wchar_t StringConstant::US_DIVIDE[] = {U_SL, U_NU}; // L"/"
const wchar_t StringConstant::US_DIVIDE_EQUAL[] = {U_SL, U_EQ, U_NU}; // L"/="
const wchar_t StringConstant::US_DOT[] = {U_DO, U_NU}; // L"."
const wchar_t StringConstant::US_DOT_DOT_DOT[] = {
    U_DO, U_DO, U_DO, U_NU}; // L"..."
const wchar_t StringConstant::US_EMPTY[] = {U_NU}; // L""
const wchar_t StringConstant::US_EOF[] = {U_E, U_O, U_F, U_NU}; // L"EOF"
const wchar_t StringConstant::US_EQUAL[] = {U_EQ, U_NU}; // L"="
const wchar_t StringConstant::US_EQUAL_EQUAL[] = {U_EQ, U_EQ, U_NU}; // L"=="
const wchar_t StringConstant::US_GREATER[] = {U_GT, U_NU}; // L">"
const wchar_t StringConstant::US_GREATER_EQUAL[] = {U_GT, U_EQ, U_NU}; // L">="
const wchar_t StringConstant::US_LBRACE[] = {U_OS, U_NU}; // L"{"
const wchar_t StringConstant::US_LBRACKET[] = {U_LB, U_NU}; // L"["
const wchar_t StringConstant::US_LEFT_SHIFT[] = {U_LT, U_LT, U_NU}; // L"<<"
const wchar_t StringConstant::US_LEFT_SHIFT_EQUAL[] = {
    U_LT, U_LT, U_EQ, U_NU}; // L"<<="
const wchar_t StringConstant::US_LESS[] = {U_LT, U_NU}; // L"<"
const wchar_t StringConstant::US_LESS_EQUAL[] = {U_LT, U_EQ, U_NU}; // L"<="
const wchar_t StringConstant::US_LPAREN[] = {U_LP, U_NU}; // L"("
const wchar_t StringConstant::US_MINUS[] = {U_MI, U_NU}; // L"-"
const wchar_t StringConstant::US_MINUS_EQUAL[] = {U_MI, U_EQ, U_NU}; // L"-="
const wchar_t StringConstant::US_MINUS_MINUS[] = {U_MI, U_MI, U_NU}; // L"--"
const wchar_t StringConstant::US_MULTIPLY[] = {U_ST, U_NU}; // L"*"
const wchar_t StringConstant::US_MULTIPLY_EQUAL[] = {
    U_ST, U_EQ, U_NU}; // L"*="
const wchar_t StringConstant::US_NOT[] = {U_EX, U_NU}; // L"!"
const wchar_t StringConstant::US_NOT_EQUAL[] = {U_EX, U_EQ, U_NU}; // L"!="
const wchar_t StringConstant::US_OR[] = {U_BA, U_NU}; // L"|"
const wchar_t StringConstant::US_OR_EQUAL[] = {U_BA, U_EQ, U_NU}; // L"|="
const wchar_t StringConstant::US_OR_OR[] = {U_BA, U_BA, U_NU}; // L"||"
const wchar_t StringConstant::US_PLUS[] = {U_PL, U_NU}; // L"+"
const wchar_t StringConstant::US_PLUS_EQUAL[] = {U_PL, U_EQ, U_NU}; // L"+="
const wchar_t StringConstant::US_PLUS_PLUS[] = {U_PL, U_PL, U_NU}; // L"++"
const wchar_t StringConstant::US_QUESTION[] = {U_QU, U_NU}; // L"?"
const wchar_t StringConstant::US_RBRACE[] = {U_CS, U_NU}; // L"}"
const wchar_t StringConstant::US_RBRACKET[] = {U_RB, U_NU}; // L"]"
const wchar_t StringConstant::US_REMAINDER[] = {U_PE, U_NU}; // L"%"
const wchar_t StringConstant::US_REMAINDER_EQUAL[] = {
    U_PE, U_EQ, U_NU}; // L"%="
const wchar_t StringConstant::US_RIGHT_SHIFT[] = {U_GT, U_GT, U_NU}; // L">>"
const wchar_t StringConstant::US_RIGHT_SHIFT_EQUAL[] = {
    U_GT, U_GT, U_EQ, U_NU}; // L">>="
const wchar_t StringConstant::US_RPAREN[] = {U_RP, U_NU}; // L")"
const wchar_t StringConstant::US_SEMICOLON[] = {U_SC, U_NU}; // L";"
const wchar_t StringConstant::US_TWIDDLE[] = {U_TI, U_NU}; // L"~"
const wchar_t StringConstant::US_UNSIGNED_RIGHT_SHIFT[] = {
    U_GT, U_GT, U_GT, U_NU}; // L">>>"
const wchar_t StringConstant::US_UNSIGNED_RIGHT_SHIFT_EQUAL[] = {
    U_GT, U_GT, U_GT, U_EQ, U_NU}; // L">>>="
const wchar_t StringConstant::US_XOR[] = {U_CA, U_NU}; // L"^"
const wchar_t StringConstant::US_XOR_EQUAL[] = {U_CA, U_EQ, U_NU}; // L"^="

//
// Common constant pool entries
//
const wchar_t StringConstant::US_DS[] = {U_DS, U_NU}; // L"$"
const wchar_t StringConstant::US_LB_RB[] = {U_LB, U_RB, U_NU}; // L"[]"
const wchar_t StringConstant::US_MI[] = {U_MI, U_NU}; // L"-"
const wchar_t StringConstant::US_SC[] = {U_SC, U_NU}; // L";"
const wchar_t StringConstant::US_SL[] = {U_SL, U_NU}; // L"/"
const wchar_t StringConstant::US_jar[] = {U_j, U_a, U_r, U_NU}; // L"jar"
const wchar_t StringConstant::US_java_SL_io[] = {
    U_j, U_a, U_v, U_a, U_SL, U_i, U_o, U_NU}; // L"java/io"
const wchar_t StringConstant::US_java_SL_lang[] = {
    U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_NU}; // L"java/lang"
const wchar_t StringConstant::US_java_SL_lang_SL_annotation[] = {
    U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL,
    U_a, U_n, U_n, U_o, U_t, U_a, U_t, U_i, U_o, U_n,
    U_NU}; // L"java/lang/annotation"
const wchar_t StringConstant::US_java_SL_util[] = {
    U_j, U_a, U_v, U_a, U_SL, U_u, U_t, U_i, U_l, U_NU}; // L"java/util"
const wchar_t StringConstant::US_zip[] = {U_z, U_i, U_p, U_NU}; // L"zip"

//
// Java keywords.
//
const wchar_t StringConstant::US_abstract[] = {
    U_a, U_b, U_s, U_t, U_r, U_a, U_c, U_t, U_NU}; // L"abstract"
const wchar_t StringConstant::US_assert[] = {
    U_a, U_s, U_s, U_e, U_r, U_t, U_NU}; // L"assert"
const wchar_t StringConstant::US_boolean[] = {
    U_b, U_o, U_o, U_l, U_e, U_a, U_n, U_NU}; // L"boolean"
const wchar_t StringConstant::US_break[] = {
    U_b, U_r, U_e, U_a, U_k, U_NU}; // L"break"
const wchar_t StringConstant::US_byte[] = {
    U_b, U_y, U_t, U_e, U_NU}; // L"byte"
const wchar_t StringConstant::US_case[] = {
    U_c, U_a, U_s, U_e, U_NU}; // L"case"
const wchar_t StringConstant::US_catch[] = {
    U_c, U_a, U_t, U_c, U_h, U_NU}; // L"catch"
const wchar_t StringConstant::US_char[] = {
    U_c, U_h, U_a, U_r, U_NU}; // L"char"
const wchar_t StringConstant::US_class[] = {
    U_c, U_l, U_a, U_s, U_s, U_NU}; // L"class"
const wchar_t StringConstant::US_const[] = {
    U_c, U_o, U_n, U_s, U_t, U_NU}; // L"const"
const wchar_t StringConstant::US_continue[] = {
    U_c, U_o, U_n, U_t, U_i, U_n, U_u, U_e, U_NU}; // L"continue"
const wchar_t StringConstant::US_default[] = {
    U_d, U_e, U_f, U_a, U_u, U_l, U_t, U_NU}; // L"default"
const wchar_t StringConstant::US_do[] = {U_d, U_o, U_NU}; // L"do"
const wchar_t StringConstant::US_double[] = {
    U_d, U_o, U_u, U_b, U_l, U_e, U_NU}; // L"double"
const wchar_t StringConstant::US_else[] = {
    U_e, U_l, U_s, U_e, U_NU}; // L"else"
const wchar_t StringConstant::US_enum[] = {
    U_e, U_n, U_u, U_m, U_NU}; // L"enum"
const wchar_t StringConstant::US_extends[] = {
    U_e, U_x, U_t, U_e, U_n, U_d, U_s, U_NU}; // L"extends"
const wchar_t StringConstant::US_false[] = {
    U_f, U_a, U_l, U_s, U_e, U_NU}; // L"false"
const wchar_t StringConstant::US_final[] = {
    U_f, U_i, U_n, U_a, U_l, U_NU}; // L"final"
const wchar_t StringConstant::US_finally[] = {
    U_f, U_i, U_n, U_a, U_l, U_l, U_y, U_NU}; // L"finally"
const wchar_t StringConstant::US_float[] = {
    U_f, U_l, U_o, U_a, U_t, U_NU}; // L"float"
const wchar_t StringConstant::US_for[] = {U_f, U_o, U_r, U_NU}; // L"for"
const wchar_t StringConstant::US_goto[] = {
    U_g, U_o, U_t, U_o, U_NU}; // L"goto"
const wchar_t StringConstant::US_if[] = {U_i, U_f, U_NU}; // L"if"
const wchar_t StringConstant::US_implements[] = {
    U_i, U_m, U_p, U_l, U_e, U_m, U_e, U_n, U_t, U_s, U_NU}; // L"implements"
const wchar_t StringConstant::US_import[] = {
    U_i, U_m, U_p, U_o, U_r, U_t, U_NU}; // L"import"
const wchar_t StringConstant::US_instanceof[] = {
    U_i, U_n, U_s, U_t, U_a, U_n, U_c, U_e, U_o, U_f, U_NU}; // L"instanceof"
const wchar_t StringConstant::US_int[] = {U_i, U_n, U_t, U_NU}; // L"int"
const wchar_t StringConstant::US_interface[] = {
    U_i, U_n, U_t, U_e, U_r, U_f, U_a, U_c, U_e, U_NU}; // L"interface"
const wchar_t StringConstant::US_long[] = {
    U_l, U_o, U_n, U_g, U_NU}; // L"long"
const wchar_t StringConstant::US_native[] = {
    U_n, U_a, U_t, U_i, U_v, U_e, U_NU}; // L"native"
const wchar_t StringConstant::US_new[] = {U_n, U_e, U_w, U_NU}; // L"new"
const wchar_t StringConstant::US_null[] = {
    U_n, U_u, U_l, U_l, U_NU}; // L"null"
const wchar_t StringConstant::US_package[] = {
    U_p, U_a, U_c, U_k, U_a, U_g, U_e, U_NU}; // L"package"
const wchar_t StringConstant::US_private[] = {
    U_p, U_r, U_i, U_v, U_a, U_t, U_e, U_NU}; // L"private"
const wchar_t StringConstant::US_protected[] = {
    U_p, U_r, U_o, U_t, U_e, U_c, U_t, U_e, U_d, U_NU}; // L"protected"
const wchar_t StringConstant::US_public[] = {
    U_p, U_u, U_b, U_l, U_i, U_c, U_NU}; // L"public"
const wchar_t StringConstant::US_return[] = {
    U_r, U_e, U_t, U_u, U_r, U_n, U_NU}; // L"return"
const wchar_t StringConstant::US_short[] = {
    U_s, U_h, U_o, U_r, U_t, U_NU}; // L"short"
const wchar_t StringConstant::US_static[] = {
    U_s, U_t, U_a, U_t, U_i, U_c, U_NU}; // L"static"
const wchar_t StringConstant::US_strictfp[] = {
    U_s, U_t, U_r, U_i, U_c, U_t, U_f, U_p, U_NU}; // L"strictfp"
const wchar_t StringConstant::US_super[] = {
    U_s, U_u, U_p, U_e, U_r, U_NU}; // L"super"
const wchar_t StringConstant::US_switch[] = {
    U_s, U_w, U_i, U_t, U_c, U_h, U_NU}; // L"switch"
const wchar_t StringConstant::US_synchronized[] = {
    U_s, U_y, U_n, U_c, U_h, U_r, U_o, U_n, U_i, U_z, U_e, U_d,
    U_NU}; // L"synchronized"
const wchar_t StringConstant::US_this[] = {
    U_t, U_h, U_i, U_s, U_NU}; // L"this"
const wchar_t StringConstant::US_throw[] = {
    U_t, U_h, U_r, U_o, U_w, U_NU}; // L"throw"
const wchar_t StringConstant::US_throws[] = {
    U_t, U_h, U_r, U_o, U_w, U_s, U_NU}; // L"throws"
const wchar_t StringConstant::US_transient[] = {
    U_t, U_r, U_a, U_n, U_s, U_i, U_e, U_n, U_t, U_NU}; // L"transient"
const wchar_t StringConstant::US_true[] = {
    U_t, U_r, U_u, U_e, U_NU}; // L"true"
const wchar_t StringConstant::US_try[] = {U_t, U_r, U_y, U_NU}; // L"try"
const wchar_t StringConstant::US_void[] = {
    U_v, U_o, U_i, U_d, U_NU}; // L"void"
const wchar_t StringConstant::US_volatile[] = {
    U_v, U_o, U_l, U_a, U_t, U_i, U_l, U_e, U_NU}; // L"volatile"
const wchar_t StringConstant::US_while[] = {
    U_w, U_h, U_i, U_l, U_e, U_NU}; // L"while"

//
// Miscellaneous strings.
//
const char StringConstant::U8S_help_header[] =
    "Jopa Compiler (based on Jikes) - Version " JOPA_VERSION_STRING
    "\nCopyright (C) IBM Corporation 1997-2003, 2004.\n"
    "Licensed Materials. Program Property of IBM. All Rights Reserved.\n"
    "Dragons. Cluster lizards. Hippity Hoppity of 7mind. Sanity not preserved.\n";
const char StringConstant::U8S_command_format[] =
    "use: jopa [options] [@files] file.java...\n";

//
// Constant pool entries.
//
const char StringConstant::U8S_AnnotationDefault[] = {
    U_A, U_n, U_n, U_o, U_t, U_a, U_t, U_i, U_o, U_n,
    U_D, U_e, U_f, U_a, U_u, U_l, U_t, U_NU}; // "AnnotationDefault
const char StringConstant::U8S_Code[] = {U_C, U_o, U_d, U_e, U_NU}; // "Code"
const char StringConstant::U8S_ConstantValue[] = {
    U_C, U_o, U_n, U_s, U_t, U_a, U_n, U_t,
    U_V, U_a, U_l, U_u, U_e, U_NU}; // "ConstantValue"
const char StringConstant::U8S_Deprecated[] = {
    U_D, U_e, U_p, U_r, U_e, U_c, U_a, U_t, U_e, U_d, U_NU}; // "Deprecated"
const char StringConstant::U8S_EnclosingMethod[] = {
    U_E, U_n, U_c, U_l, U_o, U_s, U_i, U_n, U_g,
    U_M, U_e, U_t, U_h, U_o, U_d, U_NU}; // "EnclosingMethod"
const char StringConstant::U8S_Exceptions[] = {
    U_E, U_x, U_c, U_e, U_p, U_t, U_i, U_o, U_n, U_s, U_NU}; // "Exceptions"
const char StringConstant::U8S_InnerClasses[] = {
    U_I, U_n, U_n, U_e, U_r,
    U_C, U_l, U_a, U_s, U_s, U_e, U_s, U_NU}; // "InnerClasses"
const char StringConstant::U8S_LineNumberTable[] = {
    U_L, U_i, U_n, U_e, U_N, U_u, U_m, U_b, U_e, U_r,
    U_T, U_a, U_b, U_l, U_e, U_NU}; // "LineNumberTable"
const char StringConstant::U8S_LocalVariableTable[] = {
    U_L, U_o, U_c, U_a, U_l, U_V, U_a, U_r, U_i, U_a, U_b, U_l, U_e,
    U_T, U_a, U_b, U_l, U_e, U_NU}; // "LocalVariableTable"
const char StringConstant::U8S_LocalVariableTypeTable[] = {
    U_L, U_o, U_c, U_a, U_l, U_V, U_a, U_r, U_i, U_a, U_b, U_l, U_e,
    U_T, U_y, U_p, U_e, U_T, U_a, U_b, U_l, U_e,
    U_NU}; // "LocalVariableTypeTable"
const char StringConstant::U8S_RuntimeInvisibleAnnotations[] = {
    U_R, U_u, U_n, U_t, U_i, U_m, U_e,
    U_I, U_n, U_v, U_i, U_s, U_i, U_b, U_l, U_e,
    U_A, U_n, U_n, U_o, U_t, U_a, U_t, U_i, U_o, U_n, U_s,
    U_NU}; // 'RuntimeInvisibleAnnotations"
const char StringConstant::U8S_RuntimeVisibleAnnotations[] = {
    U_R, U_u, U_n, U_t, U_i, U_m, U_e, U_V, U_i, U_s, U_i, U_b, U_l, U_e,
    U_A, U_n, U_n, U_o, U_t, U_a, U_t, U_i, U_o, U_n, U_s,
    U_NU}; // 'RuntimeVisibleAnnotations"
const char StringConstant::U8S_RuntimeInvisibleParameterAnnotations[] = {
    U_R, U_u, U_n, U_t, U_i, U_m, U_e,
    U_I, U_n, U_v, U_i, U_s, U_i, U_b, U_l, U_e,
    U_P, U_a, U_r, U_a, U_m, U_e, U_t, U_e, U_r,
    U_A, U_n, U_n, U_o, U_t, U_a, U_t, U_i, U_o, U_n, U_s,
    U_NU}; // 'RuntimeInvisibleParameterAnnotations"
const char StringConstant::U8S_RuntimeVisibleParameterAnnotations[] = {
    U_R, U_u, U_n, U_t, U_i, U_m, U_e, U_V, U_i, U_s, U_i, U_b, U_l, U_e,
    U_P, U_a, U_r, U_a, U_m, U_e, U_t, U_e, U_r,
    U_A, U_n, U_n, U_o, U_t, U_a, U_t, U_i, U_o, U_n, U_s,
    U_NU}; // 'RuntimeVisibleParameterAnnotations"
const char StringConstant::U8S_Signature[] = {
    U_S, U_i, U_g, U_n, U_a, U_t, U_u, U_r, U_e, U_NU}; // "Signature"
const char StringConstant::U8S_SourceFile[] = {
    U_S, U_o, U_u, U_r, U_c, U_e, U_F, U_i, U_l, U_e, U_NU}; // "SourceFile"
const char StringConstant::U8S_StackMap[] = {
    U_S, U_t, U_a, U_c, U_k, U_M, U_a, U_p, U_NU}; // "StackMap"
const char StringConstant::U8S_Synthetic[] = {
    U_S, U_y, U_n, U_t, U_h, U_e, U_t, U_i, U_c, U_NU}; // "Synthetic"

//
// ASCII file names.
//
const char StringConstant::U8S_DO_class[] = {
    U_DO, U_c, U_l, U_a, U_s, U_s, U_NU}; // ".class"
const char StringConstant::U8S_DO_java[] = {
    U_DO, U_j, U_a, U_v, U_a, U_NU}; // ".java"
const char StringConstant::U8S_DO_tok[] = {
    U_DO, U_t, U_o, U_k, U_NU}; // ".tok"
const char StringConstant::U8S_DO_u[] = {U_DO, U_u, U_NU}; // ".u"
const char StringConstant::U8S_LP[] = {U_LP, U_NU}; // "("
const char StringConstant::U8S_RP[] = {U_RP, U_NU}; // ")"
const char StringConstant::U8S_SL[] = {U_SL, U_NU}; // "/"

//
// Convert number to string.
//
const char StringConstant::U8S_NaN[] = {U_N, U_a, U_N, U_NU}; // "NaN"
const char StringConstant::U8S_pos_Infinity[] = {
    U_I, U_n, U_f, U_i, U_n, U_i, U_t, U_y, U_NU}; // "Infinity"
const char StringConstant::U8S_neg_Infinity[] = {
    U_MINUS, U_I, U_n, U_f, U_i, U_n, U_i, U_t, U_y, U_NU}; // "-Infinity"
const char StringConstant::U8S_pos_Zero[] = {U_0, U_DOT, U_0, U_NU}; // "0.0"
const char StringConstant::U8S_neg_Zero[] = {
    U_MINUS, U_0, U_DOT, U_0, U_NU}; // "-0.0"

Ostream Coutput;


} // Close namespace Jopa block
