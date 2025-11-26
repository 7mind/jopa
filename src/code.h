#include "platform.h"


namespace Jopa { // Open namespace Jopa block

#pragma once

class Code
{
    //
    // To facilitate the scanning, the character set is partitioned into
    // categories using the array CODE. These are described below together
    // with some self-explanatory functions defined on CODE.
    //
    enum {
        SHIFT = 9,
        SPACE_CODE = 0,
        BAD_CODE = 1,
        DIGIT_CODE = 2,
        ID_PART_CODE = 3,
        LOWER_CODE = 4,
        UPPER_CODE = 5,
        ID_START_CODE = 6
    };

    static char codes[13558];
    static u2 blocks[2176];


public:
#ifdef JOPA_DEBUG
    static inline void CodeCheck(u4 c)
    {
        assert((u2) (blocks[c >> SHIFT] + c) < 13558);
    }

    static inline bool CodeCheck(void)
    {
        for (u4 c = 0; c <= 1114111; c++)
            CodeCheck(c);
        return true;
    }
#endif // JOPA_DEBUG

//
// These methods test for Unicode surrogate pairs.
//
    static inline bool IsHighSurrogate(wchar_t c)
    {
        return c >= 0xd800 && c <= 0xdbff;
    }
    static inline bool IsLowSurrogate(wchar_t c)
    {
        return c >= 0xdc00 && c <= 0xdfff;
    }

    static inline u4 Codepoint(wchar_t hi, wchar_t lo)
    {
        assert(IsHighSurrogate(hi) && IsLowSurrogate(lo));
        return (hi << 10) + lo + (0x10000 - (0xd800 << 10) - 0xdc00);
    }
    static inline u4 Codepoint(const wchar_t* p)
    {
        u4 result = (u4) *p;
        if (IsHighSurrogate(result) && IsLowSurrogate(p[1]))
            result = Codepoint(result, p[1]);
        return result;
    }
    static inline int Codelength(const wchar_t* p)
    {
        return (IsHighSurrogate(*p) && IsLowSurrogate(p[1])) ? 2 : 1;
    }

//
// These methods test for ASCII characteristics. Since it is strictly ASCII,
// there is no need to check for Unicode surrogate pairs.
//
    static inline bool IsNewline(wchar_t c)
    {
        return c == U_LF || c == U_CR;
    }
    static inline bool IsSpaceButNotNewline(wchar_t c)
    {
        return c == U_SP || c == U_FF || c == U_HT;
    }
    static inline bool IsSpace(wchar_t c)
    {
        return c == U_SP || c == U_CR || c == U_LF ||
            c == U_HT || c == U_FF;
    }

    static inline bool IsDecimalDigit(wchar_t c)
    {
        return c <= U_9 && c >= U_0;
    }
    static inline bool IsOctalDigit(wchar_t c)
    {
        return c <= U_7 && c >= U_0;
    }
    static inline bool IsBinaryDigit(wchar_t c)
    {
        return c == U_0 || c == U_1;
    }
    static inline bool IsHexDigit(wchar_t c)
    {
        return c <= U_f && (c >= U_a ||
                            (c >= U_A && c <= U_F) ||
                            (c >= U_0 && c <= U_9));
    }
    static inline int Value(wchar_t c)
    {
        assert(IsHexDigit(c));
        return c - (c <= U_9 ? U_0 : c < U_a ? U_A - 10 : U_a - 10);
    }
    static inline bool IsSign(wchar_t c)
    {
        return c == U_MINUS || c == U_PLUS;
    }

    static inline bool IsAsciiUpper(wchar_t c)
    {
        return c <= U_Z && c >= U_A;
    }
    static inline bool IsAsciiLower(wchar_t c)
    {
        return c <= U_z && c >= U_a;
    }

//
// The following methods recognize Unicode surrogate pairs, hence the need to
// pass a pointer. Use Codelength() to determine if one or two characters
// were used in the formation of a character.
//
    static inline bool IsWhitespace(const wchar_t* p)
    {
        u4 c = Codepoint(p);
        return codes[(u2) (blocks[c >> SHIFT] + c)] == SPACE_CODE;
    }
    static inline bool IsDigit(const wchar_t* p)
    {
        u4 c = Codepoint(p);
        return codes[(u2) (blocks[c >> SHIFT] + c)] == DIGIT_CODE;
    }
    static inline bool IsUpper(const wchar_t* p)
    {
        u4 c = Codepoint(p);
        return codes[(u2) (blocks[c >> SHIFT] + c)] == UPPER_CODE;
    }
    static inline bool IsLower(const wchar_t* p)
    {
        u4 c = Codepoint(p);
        return codes[(u2) (blocks[c >> SHIFT] + c)] == LOWER_CODE;
    }
    static inline bool IsAlpha(const wchar_t* p)
    {
        u4 c = Codepoint(p);
        return codes[(u2) (blocks[c >> SHIFT] + c)] >= LOWER_CODE;
    }
    static inline bool IsAlnum(const wchar_t* p)
    {
        u4 c = Codepoint(p);
        return codes[(u2) (blocks[c >> SHIFT] + c)] >= DIGIT_CODE;
    }
};



} // Close namespace Jopa block

