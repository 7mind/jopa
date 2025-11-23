#ifndef spell_INCLUDED
#define spell_INCLUDED

#include "platform.h"
#include "case.h"


namespace Jikes { // Open namespace Jikes block
//
// This is a helper class which hueristically determines the probability
// that one string is a match for another.  The static function Index
// returns an integer from 0 to 10; for example, a return of 6 indicates
// a 60% chance that one string is a misspelled match of the other.
//
class Spell
{
    static inline int Min(int x, int y) { return (x < y ? x : y); }

public:
    static int Index(const wchar_t* str1, const wchar_t* str2)
    {
        int len1 = wcslen(str1),
            len2 = wcslen(str2);

        wchar_t *s1 = new wchar_t[len1 + 1],
                *s2 = new wchar_t[len2 + 1];

        for (int i = 0; i < len1; i++)
            s1[i] = Case::ToAsciiLower(str1[i]);
        s1[len1] = U_NULL;

        for (int j = 0; j < len2; j++)
            s2[j] = Case::ToAsciiLower(str2[j]);
        s2[len2] = U_NULL;

        if (len1 == 1 && len2 == 1)
        {
            //
            //  Singleton mispellings:
            //
            //  ;      <---->     ,
            //
            //  ;      <---->     :
            //
            //  .      <---->     ,
            //
            //  '      <---->     "
            //
            if ((s1[0] == U_SEMICOLON    &&  s2[0] == U_COMMA)  ||
                (s1[0] == U_COMMA        &&  s2[0] == U_SEMICOLON)  ||
                (s1[0] == U_SEMICOLON    &&  s2[0] == U_COLON)  ||
                (s1[0] == U_COLON        &&  s2[0] == U_SEMICOLON)  ||
                (s1[0] == U_DOT          &&  s2[0] == U_COMMA)  ||
                (s1[0] == U_COMMA        &&  s2[0] == U_DOT)  ||
                (s1[0] == U_SINGLE_QUOTE &&  s2[0] == U_DOUBLE_QUOTE)  ||
                (s1[0] == U_DOUBLE_QUOTE &&  s2[0] == U_SINGLE_QUOTE))
                    return 3;
        }

        //
        // Scan the two strings. Increment "match" count for each match.
        // When a transposition is encountered, increase "match" count
        // by two but count it as an error. When a typo is found, skip
        // it and count it as an error. Otherwise we have a mismatch; if
        // one of the strings is longer, increment its index, otherwise,
        // increment both indices and continue.
        //
        // This algorithm is an adaptation of a boolean misspelling
        // algorithm proposed by Juergen Uhl.
        //
        int count = 0,
            prefix_length = 0,
            num_errors = 0,
            i1 = 0,
            i2 = 0;

        while ((i1 < len1)  &&  (i2 < len2))
        {
            if (s1[i1] == s2[i2])
            {
                count++;
                i1++;
                i2++;
                if (num_errors == 0)
                    prefix_length++;
            }
            else if (s1[i1 + 1] == s2[i2]  &&  s1[i1] == s2[i2 + 1])
            {
                count += 2;
                i1 += 2;
                i2 += 2;
                num_errors++;
            }
            else if (s1[i1 + 1] == s2[i2 + 1])
            {
                i1++;
                i2++;
                num_errors++;
            }
            else
            {
                if ((len1 - i1) > (len2 - i2))
                     i1++;
                else if ((len2 - i2) > (len1 - i1))
                     i2++;
                else
                {
                    i1++;
                    i2++;
                }
                num_errors++;
            }
        }

        if (i1 < len1  ||  i2 < len2)
            num_errors++;

        if (num_errors > (Min(len1, len2) / 6 + 1))
             count = prefix_length;

        delete [] s1;
        delete [] s2;

        return (count * 10 / (len1 + num_errors));
    }
};


} // Close namespace Jikes block

#endif // spell_INCLUDED

