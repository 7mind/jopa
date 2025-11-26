#pragma once

#include "platform.h"


namespace Jopa { // Open namespace Jopa block
//
// NOTE that this class is hard-wired to work on an ASCII machine.
// To make it universal, one should uncomment the constructor and
// make the array "lower" non-static. In such a case, each object
// of type Case that is declared will allocate its own "lower" array.
// This would need to be done for i.e. an EBCDIC machine.
//
class Case
{
    static char lower[128];
    static char upper[128];

public:

    static inline bool IsAsciiLower(char c)
    {
        return c == lower[(int) c];
    }
    static inline char ToAsciiLower(char c)
    {
        return (c & (char) 0x80) ? c : lower[(int) c];
    }
    static inline wchar_t ToAsciiLower(wchar_t c)
    {
        return (c < 128 ? (wchar_t) lower[(int) c] : c);
    }

    static inline bool IsAsciiUpper(char c)
    {
        return c == upper[(int) c];
    }
    static inline char ToAsciiUpper(char c)
    {
        return (c & (char) 0x80) ? c : upper[(int) c];
    }
    static inline wchar_t ToAsciiUpper(wchar_t c)
    {
        return (c < 128 ? (wchar_t) upper[(int) c] : c);
    }

    static inline bool IsAsciiAlpha(char c)
    {
        return (c == lower[(int) c] || c == upper[(int) c]);
    }
    static inline bool IsAsciiAlpha(wchar_t c)
    {
        return (c == (wchar_t) lower[(int) c] ||
                c == (wchar_t) upper[(int) c]);
    }

    //
    // Find the position of the first occurrence of a character within a
    // string. If the character is not found, return -1.
    //
    static inline int Index(char *s, wchar_t c)
    {
        for (int i = 0; *s != U_NULL; i++, s++)
        {
            if ((wchar_t) *s == c)
                return i;
        }
        return -1;
    }

    static inline int Index(wchar_t *s, wchar_t c)
    {
        for (int i = 0; *s != U_NULL; i++, s++)
        {
            if ((wchar_t) *s == c)
                return i;
        }
        return -1;
    }

    //
    // Compare two character strings segments of length n in the strings
    // s1 and s2 to check whether or not they are equal. Note that unlike
    // the builtin function "strncmp" the comparison always checks n characters
    // and does not terminate if it encounters a NULL character.
    //
    static inline bool StringSegmentEqual(char *s1, const char *s2, int n)
    {
        for (int i = 0; i < n; i++)
        {
            if (ToAsciiLower(s1[i]) != ToAsciiLower(s2[i]))
                return false;
        }
        return true;
    }

    static inline bool StringSegmentEqual(wchar_t *s1, const char *s2, int n)
    {
        for (int i = 0; i < n; i++)
        {
            if (ToAsciiLower(s1[i]) != (wchar_t) ToAsciiLower(s2[i]))
                return false;
        }
        return true;
    }

    static inline bool StringSegmentEqual(char *s1, const wchar_t *s2, int n)
    {
        for (int i = 0; i < n; i++)
        {
            if ((wchar_t) ToAsciiLower(s1[i]) != ToAsciiLower(s2[i]))
                return false;
        }
        return true;
    }

    static inline bool StringSegmentEqual(wchar_t *s1, const wchar_t *s2,
                                          int n)
    {
        for (int i = 0; i < n; i++)
        {
            if (ToAsciiLower(s1[i]) != (wchar_t) ToAsciiLower(s2[i]))
                return false;
        }
        return true;
    }


    //
    // Compare two null-terminated character strings, s1 and s2
    // to check whether or not they are equal.
    //
    static inline bool StringEqual(char *s1, const char *s2)
    {
        int i;
        for (i = 0; s1[i] && s2[i]; i++)
        {
            if (ToAsciiLower(s1[i]) != ToAsciiLower(s2[i]))
                return false;
        }
        return (s1[i] == s2[i]);
    }

    static inline bool StringEqual(wchar_t *s1, const char *s2)
    {
        int i;
        for (i = 0; s1[i] && s2[i]; i++)
        {
            if (ToAsciiLower(s1[i]) != (wchar_t) ToAsciiLower(s2[i]))
                return false;
        }
        return (s1[i] == (wchar_t) s2[i]);
    }

    static inline bool StringEqual(char *s1, const wchar_t *s2)
    {
        int i;
        for (i = 0; s1[i] && s2[i]; i++)
        {
            if ((wchar_t) ToAsciiLower(s1[i]) != ToAsciiLower(s2[i]))
                return false;
        }
        return ((wchar_t) s1[i] == s2[i]);
    }

    static inline bool StringEqual(wchar_t *s1, const wchar_t *s2)
    {
        int i;
        for (i = 0; s1[i] && s2[i]; i++)
        {
            if (ToAsciiLower(s1[i]) != (wchar_t) ToAsciiLower(s2[i]))
                return false;
        }
        return (s1[i] == (wchar_t) s2[i]);
    }

// see comment above.
//
//  Lcase()
//  {
//      for (int c = 0; c < 256; c++)
//      {
//          if (isupper(c))
//               lower[c] = tolower(c);
//          else if (islower(c))
//               upper[c] = toupper(c);
//          else lower[c] = upper[c] = c;
//      }
//  }
};


} // Close namespace Jopa block

