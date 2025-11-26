#include "config.h"
#include "long.h"
#include "double.h"

IntToString::IntToString(int num)
{
    if (num == 0x80000000)
    {
        str = info;
        strcpy(str, StringConstant::U8S_smallest_int);
    }
    else
    {
        str = &info[TAIL_INDEX];
        *str = U_NULL;
        int n = (num < 0 ? -num : num);
        do
        {
            *--str = (U_0 + n % 10);
            n /= 10;
        } while (n != 0);

        if (num < 0)
            *--str = U_MINUS;
    }

    return;
}


IntToWstring::IntToWstring(int num)
{
    if (num == 0x80000000)
    {
        wstr = winfo;
        wcscpy(wstr,  StringConstant::US_smallest_int);
    }
    else
    {
        wstr = &winfo[TAIL_INDEX];
        *wstr = U_NULL;
        int n = (num < 0 ? -num : num);
        do
        {
            *--wstr = (U_0 + n % 10);
            n /= 10;
        } while (n != 0);

        if (num < 0)
            *--wstr = U_MINUS;
    }

    return;
}


ULongToDecString::ULongToDecString(ULongInt &num)
{
    str = &info[TAIL_INDEX];
    *str = U_NULL;

    ULongInt n = num; // make a copy in order to not destroy reference argument
    do
    {
        *--str = U_0 + (n % 10).LowWord();
        n /= 10;
    } while (n != 0);

    return;
}


LongToOctString::LongToOctString(BaseLong &num)
{
    str = &info[TAIL_INDEX];
    *str = U_NULL;

    ULongInt n = num; // make a copy in order to not destroy reference argument
    do
    {
        *--str = U_0 + (n % 8).LowWord();
        n /= 8;
    } while (n != 0);

    *--str = U_0;

    return;
}


LongToHexString::LongToHexString(BaseLong &num)
{
    str = &info[TAIL_INDEX];
    *str = U_NULL;

    ULongInt n = num; // make a copy in order to not destroy reference argument
    do
    {
        *--str = U_0 + (n % 16).LowWord();
        n /= 16;
    } while (n != 0);

    *--str = U_x;
    *--str = U_0;

    return;
}


LongToDecString::LongToDecString(LongInt &num)
{
    if (num.HighWord() == 0x80000000 && num.LowWord() == 0x00000000)
    {
        str = info;
        strcpy(str,  StringConstant::U8S_smallest_long_int);
    }
    else
    {
        str = &info[TAIL_INDEX];
        *str = U_NULL;

        ULongInt n = (num.HighWord() == 0x80000000 ? (LongInt) (-num) : num); // compute the absolute value

        do
        {
            *--str = U_0 + (n % 10).LowWord();
            n /= 10;
        } while (n != 0);

        if (num.HighWord() & 0x80000000)
            *--str = U_MINUS;
    }

    return;
}


//
// Convert an double to its character string representation.
//
FloatToString::FloatToString(IEEEfloat &num)
{
    if (num.IsNaN())
    {
         strcpy(str,"NaN");
         length = strlen("NaN");
    }
    else if (num.IsNegativeInfinity())
    {
         strcpy(str,"-Infinity");
         length = strlen("-Infinity");
    }
    else if (num.IsPositiveInfinity())
    {
         strcpy(str,"Infinity");
         length = strlen("Infinity");
    }
    else if (num.IsNegativeZero())
    {
         strcpy(str,"-0.0");
         length = strlen("-0.0");
    }
    else if (num.IsPositiveZero())
    {
         strcpy(str,"0.0");
         length = strlen("0.0");
    }
    else
    {
        //
        // TODO: This conversion is a temporary patch. Need volunteer to implement
        //       better algorithm:
        //
        //            Burger/Dybvig, PLDI 1996
        //            Steele/White,  PLDI 1990
        //
        // If the absolute value f of the number in question is outside of the range 
        //
        //         1E-3 <= f < 1E+7
        //
        // we write the number out in "computerized scientific notation".
        // Otherwise, we write it out in standard form.
        //
        int decimal_exponent = 0;
        float f = (num.IsNegative() ? -num.FloatValue() : num.FloatValue());
        if (f < 1E-3 || f >= 1E+7)
        {
            //
            // The value of the number f can be expressed as
            //
            //     mantissa * (2 ** num.Exponent())
            //
            // But we really need to express it as:
            //
            //     mantissa * (10 ** decimal_exponent)
            //
            decimal_exponent = (int) ceil(num.Exponent() * log10(2.0));
            f *= pow(10.0, -decimal_exponent); // shift f until it has precisely one decimal digit before the dot
            if (floor(f) == 0.0)
            {
                decimal_exponent--;
                f *= 10.0;
            }

            assert(floor(f) > 0.0f && floor(f) < 10.0f); // make sure there is only one digit !!!
        }

        char *s = str;
        if (num.IsNegative()) // if the number is negative, add the minus sign
            *s++ = U_MINUS;

        //
        // convert whole part into its string representation
        //
        IntToString whole((int) f);

        char *ptr = whole.String();
        while(*ptr)
            *s++ = *ptr++;
        *s++ = U_DOT;

        //
        // Convert fractional part to its string representation
        //
        int limit = MAXIMUM_PRECISION + (num.IsNegative() ? 2 : 1);
        float fraction = f - ((int) f);
        do
        {
            fraction *= 10.0;
            *s++ = U_0 + ((int) fraction);
            fraction -= ((int) fraction);
        } while((fraction > 0.0) && (s - str) < limit);

        //
        // For each leading 0, add a little more precision.
        // There can be at most 3.
        // 
        char *last = s - 1;
        for (int i = 0; i < 3 && floor(f) == 0.0 && *last != U_0; i++)
        {
            f *= 10.0;
            fraction *= 10.0;
            last = s;
            *s++ = U_0 + ((int) fraction);
            fraction -= ((int) fraction);
        }

        //
        // Round if necessary
        //
        if (fraction >= 0.5)
        {
            while (*last == U_9)
                *last-- = U_0;
            char *dot_character = (*last == U_DOT ? last : (char *) NULL);
            if (dot_character)
            {
                last--;
                while (last >= str && *last == U_9)
                    *last-- = U_0;
            }

            if (last < str)
            {
                *++dot_character = U_DOT; // move dot over 1 place
                *str = 1;                 // place a 1 in the first position
                *s++ = U_0;               // add an extra zero at the end.
            }
            else (*last)++; // increment the number in the last position
        }

        //
        // Remove all excess trailing zeroes
        //
        while (*--s == U_0)
            ;
        s += (*s == U_DOT ? 2 : 1); // need at least one digit after the dot.

        //
        // If the number is to be written out in scientific notation, add the exponent
        //
        if (decimal_exponent != 0)
        {
            *s++ = U_E;
            IntToString exponent(decimal_exponent);
            char *ptr = exponent.String();
            while (*ptr)
                *s++ = *ptr++;
        }

        *s = U_NULL;      // close string
        length = s - str; // compute length
    }

    assert(length <= MAXIMUM_STR_LENGTH);

    return;
}


//
// Convert an double to its character string representation.
//
DoubleToString::DoubleToString(IEEEdouble &num)
{
    if (num.IsNaN())
    {
         strcpy(str,"NaN");
         length = strlen("NaN");
    }
    else if (num.IsNegativeInfinity())
    {
         strcpy(str,"-Infinity");
         length = strlen("-Infinity");
    }
    else if (num.IsPositiveInfinity())
    {
         strcpy(str,"Infinity");
         length = strlen("Infinity");
    }
    else if (num.IsNegativeZero())
    {
         strcpy(str,"-0.0");
         length = strlen("-0.0");
    }
    else if (num.IsPositiveZero())
    {
         strcpy(str,"0.0");
         length = strlen("0.0");
    }
    else
    {
        //
        // TODO: This conversion is a temporary patch. Need volunteer to implement
        //       better algorithm:
        //
        //            Burger/Dybvig, PLDI 1996
        //            Steele/White,  PLDI 1990
        //
        // If the absolute value d of the number in question is outside of the range 
        //
        //         1E-3 <= d < 1E+7
        //
        // we write the number out in "computerized scientific notation".
        // Otherwise, we write it out in standard form.
        //
        int decimal_exponent = 0;
        double d = (num.IsNegative() ? -num.DoubleValue() : num.DoubleValue());
        if (d < 1E-3 || d >= 1E+7)
        {
            //
            // The value of the number f can be expressed as
            //
            //     mantissa * (2 ** num.Exponent())
            //
            // But we really need to express it as:
            //
            //     mantissa * (10 ** decimal_exponent)
            //
            decimal_exponent = (int) ceil(num.Exponent() * log10(2.0));
            d *= pow(10.0, -decimal_exponent); // shift f until it has precisely one decimal digit before the dot
            if (floor(d) == 0.0)
            {
                decimal_exponent--;
                d *= 10.0;
            }

            assert(floor(d) > 0.0 && floor(d) < 10.0); // make sure there is only one digit !!!
        }

        char *s = str;
        if (num.IsNegative()) // if the number is negative, add the minus sign
            *s++ = U_MINUS;

        //
        // convert whole part into its string representation
        //
        IntToString whole((int) d);

        char *ptr = whole.String();
        while(*ptr)
            *s++ = *ptr++;
        *s++ = U_DOT;

        //
        // Convert fractional part to its string representation
        //
        int limit = MAXIMUM_PRECISION + (num.IsNegative() ? 2 : 1);
        double fraction = d - ((int) d);
        do
        {
            fraction *= 10.0;
            *s++ = U_0 + ((int) fraction);
            fraction -= ((int) fraction);
        } while((fraction > 0.0) && (s - str) < limit);

        //
        // For each leading 0, add a little more precision.
        // There can be at most 3.
        // 
        char *last = s - 1;
        for (int i = 0; i < 3 && floor(d) == 0.0 && *last != U_0; i++)
        {
            d *= 10.0;
            fraction *= 10.0;
            last = s;
            *s++ = U_0 + ((int) fraction);
            fraction -= ((int) fraction);
        }

        //
        // Round if necessary
        //
        if (fraction >= 0.5)
        {
            while (*last == U_9)
                *last-- = U_0;
            char *dot_character = (*last == U_DOT ? last : (char *) NULL);
            if (dot_character)
            {
                last--;
                while (last >= str && *last == U_9)
                    *last-- = U_0;
            }

            if (last < str)
            {
                *++dot_character = U_DOT; // move dot over 1 place
                *str = 1;                 // place a 1 in the first position
                *s++ = U_0;               // add an extra zero at the end.
            }
            else (*last)++; // increment the number in the last position
        }

        //
        // Remove all excess trailing zeroes
        //
        while (*--s == U_0)
            ;
        s += (*s == U_DOT ? 2 : 1); // need at least one digit after the dot.

        //
        // If the number is to be written out in scientific notation, add the exponent
        //
        if (decimal_exponent != 0)
        {
            *s++ = U_E;
            IntToString exponent(decimal_exponent);
            char *ptr = exponent.String();
            while (*ptr)
                *s++ = *ptr++;
        }

        *s = U_NULL;      // close string
        length = s - str; // compute length
    }

    assert(length <= MAXIMUM_STR_LENGTH);

    return;
}


Ostream &Ostream::operator<<(LongInt a)
{
    if (os -> flags() & os -> dec)
    {
        LongToDecString long_int(a);
        *os << long_int.String();
    }
    else if (os -> flags() & os -> oct)
    {
        LongToOctString long_int(a);
        *os << (os -> flags() & os -> showbase ? long_int.StringWithBase() : long_int.String());
    }
    else if (os -> flags() & os -> hex)
    {
        LongToHexString long_int(a);
        *os << (os -> flags() & os -> showbase ? long_int.StringWithBase() : long_int.String());
    }
    else
    {
         os -> flush();
         assert(! "know how to print signed long value in specified format yet !!!");
    }

    return *this;
}

Ostream &Ostream::operator<<(ULongInt a)
{
    if (os -> flags() & os -> dec)
    {
        ULongToDecString ulong_int(a);
        *os << ulong_int.String();
    }
    else if (os -> flags() & os -> oct)
    {
        LongToOctString ulong_int(a);
        *os << (os -> flags() & os -> showbase ? ulong_int.StringWithBase() : ulong_int.String());
    }
    else if (os -> flags() & os -> hex)
    {
        LongToHexString ulong_int(a);
        *os << (os -> flags() & os -> showbase ? ulong_int.StringWithBase() : ulong_int.String());
    }
    else
    {
        os -> flush();
        assert(! "know how to print unsigned long value in specified format yet !!!");
    }

    return *this;
}

wchar_t StringConstant::US_AND[]                        = {U_AM, U_NU}, // "&"
        StringConstant::US_AND_AND[]                    = {U_AM, U_AM, U_NU}, // "&&"
        StringConstant::US_AND_EQUAL[]                  = {U_AM, U_EQ, U_NU}, // "&="
        StringConstant::US_COLON[]                      = {U_CO, U_NU}, // ":"
        StringConstant::US_COMMA[]                      = {U_CM, U_NU}, // ","
        StringConstant::US_DIVIDE[]                     = {U_SL, U_NU}, // "/"
        StringConstant::US_DIVIDE_EQUAL[]               = {U_SL, U_EQ, U_NU}, // "/="
        StringConstant::US_DOT[]                        = {U_DO, U_NU}, // "."
        StringConstant::US_EMPTY[]                      = {U_NU}, // ""
        StringConstant::US_EQUAL[]                      = {U_EQ, U_NU}, // "="
        StringConstant::US_EQUAL_EQUAL[]                = {U_EQ, U_EQ, U_NU}, // "=="
        StringConstant::US_GREATER[]                    = {U_GT, U_NU}, // ">"
        StringConstant::US_GREATER_EQUAL[]              = {U_GT, U_EQ, U_NU}, // ">="
        StringConstant::US_LBRACE[]                     = {U_OS, U_NU}, // "{"
        StringConstant::US_LBRACKET[]                   = {U_LB, U_NU}, // "["
        StringConstant::US_LEFT_SHIFT[]                 = {U_LT, U_LT, U_NU}, // "<<"
        StringConstant::US_LEFT_SHIFT_EQUAL[]           = {U_LT, U_LT, U_EQ, U_NU}, // "<<="
        StringConstant::US_LESS[]                       = {U_LT, U_NU}, // "<"
        StringConstant::US_LESS_EQUAL[]                 = {U_LT, U_EQ, U_NU}, // "<="
        StringConstant::US_LPAREN[]                     = {U_LP, U_NU}, // "("
        StringConstant::US_MINUS[]                      = {U_MI, U_NU}, // "-"
        StringConstant::US_MINUS_EQUAL[]                = {U_MI, U_EQ, U_NU}, // "-="
        StringConstant::US_MINUS_MINUS[]                = {U_MI, U_MI, U_NU}, // "--"
        StringConstant::US_MULTIPLY[]                   = {U_ST, U_NU}, // "*"
        StringConstant::US_MULTIPLY_EQUAL[]             = {U_ST, U_EQ, U_NU}, // "*="
        StringConstant::US_NOT[]                        = {U_EX, U_NU}, // "!"
        StringConstant::US_NOT_EQUAL[]                  = {U_EX, U_EQ, U_NU}, // "!="
        StringConstant::US_OR[]                         = {U_BA, U_NU}, // "|"
        StringConstant::US_OR_EQUAL[]                   = {U_BA, U_EQ, U_NU}, // "|="
        StringConstant::US_OR_OR[]                      = {U_BA, U_BA, U_NU}, // "||"
        StringConstant::US_PLUS[]                       = {U_PL, U_NU}, // "+"
        StringConstant::US_PLUS_EQUAL[]                 = {U_PL, U_EQ, U_NU}, // "+="
        StringConstant::US_PLUS_PLUS[]                  = {U_PL, U_PL, U_NU}, // "++"
        StringConstant::US_QUESTION[]                   = {U_QU, U_NU}, // "?"
        StringConstant::US_RBRACE[]                     = {U_CS, U_NU}, // "}"
        StringConstant::US_RBRACKET[]                   = {U_RB, U_NU}, // "]"
        StringConstant::US_REMAINDER[]                  = {U_PE, U_NU}, // "%"
        StringConstant::US_REMAINDER_EQUAL[]            = {U_PE, U_EQ, U_NU}, // "%="
        StringConstant::US_RIGHT_SHIFT[]                = {U_GT, U_GT, U_NU}, // ">>"
        StringConstant::US_RIGHT_SHIFT_EQUAL[]          = {U_GT, U_GT, U_EQ, U_NU}, // ">>="
        StringConstant::US_RPAREN[]                     = {U_RP, U_NU}, // ")"
        StringConstant::US_SEMICOLON[]                  = {U_SC, U_NU}, // ";"
        StringConstant::US_TWIDDLE[]                    = {U_TI, U_NU}, // "~"
        StringConstant::US_UNSIGNED_RIGHT_SHIFT[]       = {U_GT, U_GT, U_GT, U_NU}, // ">>>"
        StringConstant::US_UNSIGNED_RIGHT_SHIFT_EQUAL[] = {U_GT, U_GT, U_GT, U_EQ, U_NU}, // ">>>="
        StringConstant::US_XOR[]                        = {U_CA, U_NU}, // "^"
        StringConstant::US_XOR_EQUAL[]                  = {U_CA, U_EQ, U_NU}, // "^="

        StringConstant::US_Boolean[] = {U_B, U_o, U_o, U_l, U_e, U_a, U_n, U_NU}, // "Boolean"
        StringConstant::US_Byte[] = {U_B, U_y, U_t, U_e, U_NU}, // "Byte"
        StringConstant::US_Character[] = {U_C, U_h, U_a, U_r, U_a, U_c, U_t, U_e, U_r, U_NU}, // "Character"
        StringConstant::US_Class[] = {U_C, U_l, U_a, U_s, U_s, U_NU}, // "Class"
        StringConstant::US_ClassNotFoundException[] = {U_C, U_l, U_a, U_s, U_s, U_N, U_o, U_t, U_F, U_o, U_u, U_n, U_d, U_E, U_x, U_c, U_e, U_p, U_t, U_i, U_o, U_n, U_NU}, // "ClassNotFoundException"
        StringConstant::US_Cloneable[] = {U_C, U_l, U_o, U_n, U_e, U_a, U_b, U_l, U_e, U_NU}, // "Cloneable"
        StringConstant::US_Double[] = {U_D, U_o, U_u, U_b, U_l, U_e, U_NU}, // "Double"
        StringConstant::US_Error[] = {U_E, U_r, U_r, U_o, U_r, U_NU}, // "Error"
        StringConstant::US_Exception[] = { U_E, U_x, U_c, U_e, U_p, U_t, U_i, U_o, U_n, U_NU}, // "Exception"
        StringConstant::US_Float[] = {U_F, U_l, U_o, U_a, U_t, U_NU},  // "Float"
        StringConstant::US_Integer[] = {U_I, U_n, U_t, U_e, U_g, U_e, U_r, U_NU}, // "Integer"
        StringConstant::US_L[] = {U_L, U_NU}, // "L"
        StringConstant::US_Long[]  = {U_L, U_o, U_n, U_g, U_NU}, // "Long"
        StringConstant::US_NoClassDefFoundError[] = {U_N, U_o, U_C, U_l, U_a, U_s, U_s, U_D, U_e, U_f, U_F, U_o, U_u, U_n, U_d, U_E, U_r, U_r, U_o, U_r, U_NU}, // "NoClassDefFoundError"
        StringConstant::US_Object[] = {U_O, U_b, U_j, U_e, U_c, U_t, U_NU}, // "Object"
        StringConstant::US_PObject[] = {U_P, U_O, U_b, U_j, U_e, U_c, U_t, U_NU}, // "PObject"
        StringConstant::US_RuntimeException[] = {U_R, U_u, U_n, U_t, U_i, U_m, U_e, U_E, U_x, U_c, U_e, U_p, U_t, U_i, U_o, U_n, U_NU}, // RuntimeException
        StringConstant::US_Serializable[] = {U_S, U_e, U_r, U_i, U_a, U_l, U_i, U_z, U_a, U_b, U_l, U_e, U_NU}, // Serializable
        StringConstant::US_Short[] = {U_S, U_h, U_o, U_r, U_t, U_NU}, // Short
        StringConstant::US_StringBuffer[] = {U_S, U_t, U_r, U_i, U_n, U_g, U_B, U_u, U_f, U_f, U_e, U_r, U_NU}, // StringBuffer
        StringConstant::US_String[] = {U_S, U_t, U_r, U_i, U_n, U_g, U_NU}, // String
        StringConstant::US_TYPE[] = {U_T, U_Y, U_P, U_E, U_NU}, // "TYPE"
        StringConstant::US_Throwable[] = {U_T, U_h, U_r, U_o, U_w, U_a, U_b, U_l, U_e, U_NU}, // Throwable
        StringConstant::US_Void[] = {U_V, U_o, U_i, U_d, U_NU}, // Void
        StringConstant::US_Vector[] = {U_V, U_e, U_c, U_t, U_o, U_r, U_NU}, // Vector
        StringConstant::US__DO[] = {U_DO, U_NU}, // "."
        StringConstant::US__DO__DO[] = {U_DO, U_DO, U_NU}, // ".."
        StringConstant::US__DS[] = {U_DS, U_NU}, // "$"
        StringConstant::US__LB__RB[] = {U_LB, U_RB, U_NU}, // "[]"
        StringConstant::US__LT_clinit_GT[] = {U_LT, U_c, U_l, U_i, U_n, U_i, U_t, U_GT, U_NU}, // "<clinit>"
        StringConstant::US__LT_init_GT[] = {U_LT, U_i, U_n, U_i, U_t, U_GT, U_NU}, // "<init>"
        StringConstant::US__QU__QU[] = {U_QU, U_QU, U_NU},  // "??"
        StringConstant::US__SC[] = {U_SC, U_NU}, // ";"
        StringConstant::US__SL[] = {U_SL, U_NU}, // "/"

        StringConstant::US__zip[] = {U_z, U_i, U_p, U_NU}, // "zip"
        StringConstant::US__jar[] = {U_j, U_a, U_r, U_NU}, // "jar"

        StringConstant::US__array[] = {U_a, U_r, U_r, U_a, U_y, U_NU}, // "array"
        StringConstant::US__access_DOLLAR[] = {U_a, U_c, U_c, U_e, U_s, U_s, U_DS, U_NU}, // "access$"
        StringConstant::US__class_DOLLAR[] = {U_c, U_l, U_a, U_s, U_s, U_DS, U_NU}, // "class$"
        StringConstant::US__constructor_DOLLAR[] = {U_c, U_o, U_n, U_s, U_t, U_r, U_u, U_c, U_t, U_o, U_r, U_DS, U_NU}, // "constructor$"
        StringConstant::US__this_DOLLAR[] = {U_t, U_h, U_i, U_s, U_DS, U_NU}, // "this$"
        StringConstant::US__val_DOLLAR[] = {U_v, U_a, U_l, U_DS, U_NU}, // "val$"

        StringConstant::US_abstract[] = {U_a, U_b, U_s, U_t, U_r, U_a, U_c, U_t, U_NU}, // "abstract"
        StringConstant::US_append[] = {U_a, U_p, U_p, U_e, U_n, U_d, U_NU}, // "append"
        StringConstant::US_block_DOLLAR[] = {U_b, U_l, U_o, U_c, U_k, U_DS, U_NU}, // "block$"
        StringConstant::US_boolean[] = {U_b, U_o, U_o, U_l, U_e, U_a, U_n, U_NU}, // "boolean"
        StringConstant::US_break[] = {U_b, U_r, U_e, U_a, U_k, U_NU}, // "break"
        StringConstant::US_byte[] = {U_b, U_y, U_t, U_e, U_NU}, // "byte"
        StringConstant::US_case[] = {U_c, U_a, U_s, U_e, U_NU}, // "case"
        StringConstant::US_catch[] = {U_c, U_a, U_t, U_c, U_h, U_NU}, // "catch"
        StringConstant::US_char[] = {U_c, U_h, U_a, U_r, U_NU}, // "char"
        StringConstant::US_class[] = {U_c, U_l, U_a, U_s, U_s, U_NU}, // "class"
        StringConstant::US_clone[] = {U_c, U_l, U_o, U_n, U_e, U_NU}, // "clone"
        StringConstant::US_const[] = {U_c, U_o, U_n, U_s, U_t, U_NU}, // "const"
        StringConstant::US_continue[] = {U_c, U_o, U_n, U_t, U_i, U_n, U_u, U_e, U_NU}, // "continue"
        StringConstant::US_default[] = {U_d, U_e, U_f, U_a, U_u, U_l, U_t, U_NU}, // "default"
        StringConstant::US_do[] = {U_d, U_o, U_NU}, // "do"
        StringConstant::US_double[] = {U_d, U_o, U_u, U_b, U_l, U_e, U_NU}, // "double"
        StringConstant::US_else[] = {U_e, U_l, U_s, U_e, U_NU}, // "else"
        StringConstant::US_extends[] = {U_e, U_x, U_t, U_e, U_n, U_d, U_s, U_NU}, // "extends"
        StringConstant::US_false[] = {U_f, U_a, U_l, U_s, U_e, U_NU}, // "false"
        StringConstant::US_final[] = {U_f, U_i, U_n, U_a, U_l, U_NU}, // "final"
        StringConstant::US_finally[] = {U_f, U_i, U_n, U_a, U_l, U_l, U_y, U_NU}, // "finally"
        StringConstant::US_float[] = {U_f, U_l, U_o, U_a, U_t, U_NU}, // "float"
        StringConstant::US_for[] = {U_f, U_o, U_r, U_NU}, // "for"
        StringConstant::US_forName[] = {U_f, U_o, U_r, U_N, U_a, U_m, U_e, U_NU}, // "forName"
        StringConstant::US_getMessage[] = {U_g, U_e, U_t, U_M, U_e, U_s, U_s, U_a, U_g, U_e, U_NU}, // "getMessage"
        StringConstant::US_goto[] = {U_g, U_o, U_t, U_o, U_NU}, // "goto"
        StringConstant::US_if[] = {U_i, U_f, U_NU}, // "if"
        StringConstant::US_implements[] = {U_i, U_m, U_p, U_l, U_e, U_m, U_e, U_n, U_t, U_s, U_NU}, // "implements"
        StringConstant::US_import[] = {U_i, U_m, U_p, U_o, U_r, U_t, U_NU}, // "import"
        StringConstant::US_instanceof[] = {U_i, U_n, U_s, U_t, U_a, U_n, U_c, U_e, U_o, U_f, U_NU}, // "instanceof"
        StringConstant::US_int[] = {U_i, U_n, U_t, U_NU}, // "int"
        StringConstant::US_interface[] = {U_i, U_n, U_t, U_e, U_r, U_f, U_a, U_c, U_e, U_NU}, // "interface"
        StringConstant::US_java_SL_io[] =  {U_j, U_a, U_v, U_a, U_SL, U_i, U_o, U_NU}, // "java/io"
        StringConstant::US_java_SL_lang[] = {U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_NU}, // "java/lang"
        StringConstant::US_java_SL_util[] = {U_j, U_a, U_v, U_a, U_SL, U_u, U_t, U_i, U_l, U_NU}, // "java/lang"
        StringConstant::US_length[] = {U_l, U_e, U_n, U_g, U_t, U_h, U_NU}, // "length"
        StringConstant::US_long[] = {U_l, U_o, U_n, U_g, U_NU}, // "long"
        StringConstant::US_native[] = {U_n, U_a, U_t, U_i, U_v, U_e, U_NU}, // "native"
        StringConstant::US_new[] = {U_n, U_e, U_w, U_NU}, // "new"
        StringConstant::US_null[] = {U_n, U_u, U_l, U_l, U_NU}, // "null"
        StringConstant::US_package[] = {U_p, U_a, U_c, U_k, U_a, U_g, U_e, U_NU}, // "package"
        StringConstant::US_private[] = {U_p, U_r, U_i, U_v, U_a, U_t, U_e, U_NU}, // "private"
        StringConstant::US_protected[] = {U_p, U_r, U_o, U_t, U_e, U_c, U_t, U_e, U_d, U_NU}, // "protected"
        StringConstant::US_public[] = {U_p, U_u, U_b, U_l, U_i, U_c, U_NU}, // "public"
        StringConstant::US_return[] = {U_r, U_e, U_t, U_u, U_r, U_n, U_NU}, // "return"
        StringConstant::US_short[] = {U_s, U_h, U_o, U_r, U_t, U_NU}, // "short"
        StringConstant::US_static[] = {U_s, U_t, U_a, U_t, U_i, U_c, U_NU}, // "static"
        StringConstant::US_strictfp[] = {U_s, U_t, U_r, U_i, U_c, U_t, U_f, U_p, U_NU}, // "strictfp"
        StringConstant::US_super[] = {U_s, U_u, U_p, U_e, U_r, U_NU}, // "super"
        StringConstant::US_switch[] = {U_s, U_w, U_i, U_t, U_c, U_h, U_NU}, // "switch"
        StringConstant::US_synchronized[] = {U_s, U_y, U_n, U_c, U_h, U_r, U_o, U_n, U_i, U_z, U_e, U_d, U_NU}, // "synchronized"
        StringConstant::US_this0[] = {U_t, U_h, U_i, U_s, U_DS, U_0, U_NU}, // "this$0"
        StringConstant::US_this[] = {U_t, U_h, U_i, U_s, U_NU}, // "this"
        StringConstant::US_throw[] = {U_t, U_h, U_r, U_o, U_w, U_NU}, // "throw"
        StringConstant::US_throws[] = {U_t, U_h, U_r, U_o, U_w, U_s, U_NU}, // "throws"
        StringConstant::US_toString[] = {U_t, U_o, U_S, U_t, U_r, U_i, U_n, U_g, U_NU}, // "toString"
        StringConstant::US_transient[] = {U_t, U_r, U_a, U_n, U_s, U_i, U_e, U_n, U_t, U_NU}, // "transient"
        StringConstant::US_true[] = {U_t, U_r, U_u, U_e, U_NU}, // "true"
        StringConstant::US_try[] = {U_t, U_r, U_y, U_NU}, // "try"
        StringConstant::US_void[] = {U_v, U_o, U_i, U_d, U_NU}, // "void"
        StringConstant::US_volatile[] = {U_v, U_o, U_l, U_a, U_t, U_i, U_l, U_e, U_NU}, // "volatile"
        StringConstant::US_while[] = {U_w, U_h, U_i, U_l, U_e, U_NU}, // "while"

        StringConstant::US_EOF[] = {U_E, U_O, U_F, U_NU}; // "EOF"

wchar_t StringConstant::US_smallest_int[] = {U_MINUS, U_2, U_1, U_4, U_7, U_4, U_8, U_3, U_6, U_4, U_8, U_NU}; // "-2147483648"

char StringConstant::U8S_command_format[] = "use: jikes [-classpath path][-d dir][-debug][-depend|-Xdepend][-deprecation]"
                                            "[-g][-nowarn][-nowrite][-O][-verbose][-Xstdout]"
                                            "[+1.0][++][+B][+D][+E][+F][+K][+M][+P][+T][+U][+Z]"
                                            " file.java...";

char StringConstant::U8S_B[] = {U_B,U_NU}, // "B"
     StringConstant::U8S_C[] = {U_C,U_NU}, // "C"
     StringConstant::U8S_Code[] = {U_C,U_o,U_d,U_e,U_NU}, // "Code"
     StringConstant::U8S_ConstantValue[] = {U_C,U_o,U_n,U_s,U_t,U_a,U_n,U_t,U_V,U_a,U_l,U_u,U_e,U_NU}, // "ConstantValue"
     StringConstant::U8S_D[] = {U_D,U_NU}, // "D"
     StringConstant::U8S_Exceptions[] = {U_E,U_x,U_c,U_e,U_p,U_t,U_i,U_o,U_n,U_s,U_NU}, // "Exceptions"
     StringConstant::U8S_F[] = {U_F,U_NU}, // "F"
     StringConstant::U8S_I[] = {U_I,U_NU}, // "I"
     StringConstant::U8S_InnerClasses[] = {U_I,U_n,U_n,U_e,U_r,U_C,U_l,U_a,U_s,U_s,U_e,U_s,U_NU}, // "InnerClasses"
     StringConstant::U8S_J[] = {U_J,U_NU},  // "J"
     StringConstant::U8S_LP_Ljava_SL_lang_SL_String_SC_RP_Ljava_SL_lang_SL_Class_SC[] = {U_LP,U_L,U_j,U_a,U_v,U_a,U_SL,U_l,U_a,U_n,U_g,U_SL,U_S,U_t,U_r,U_i,U_n,U_g,U_SC,U_RP,U_L,U_j,U_a,U_v,U_a,U_SL,U_l,U_a,U_n,U_g,U_SL,U_C,U_l,U_a,U_s,U_s,U_SC,U_NU}, // "(Ljava/lang/String;)Ljava/lang/Class;"
     StringConstant::U8S_LP_Ljava_SL_lang_SL_String_SC_RP_V[] = {U_LP,U_L,U_j,U_a,U_v,U_a,U_SL,U_l,U_a,U_n,U_g,U_SL,U_S,U_t,U_r,U_i,U_n,U_g,U_SC,U_RP,U_V,U_NU}, // "(Ljava/lang/String;)V"
     StringConstant::U8S_LP_RP_Ljava_SL_lang_SL_String_SC[] = {U_LP,U_RP,U_L,U_j,U_a,U_v,U_a,U_SL,U_l,U_a,U_n,U_g,U_SL,U_S,U_t,U_r,U_i,U_n,U_g,U_SC,U_NU}, // "()_Ljava/lang/String;"
     StringConstant::U8S_LP_RP_V[] = {U_LP,U_RP,U_V,U_NU}, // "()V"
     StringConstant::U8S_LineNumberTable[] = {U_L,U_i,U_n,U_e,U_N,U_u,U_m,U_b,U_e,U_r,U_T,U_a,U_b,U_l,U_e,U_NU}, // "LineNumberTable"
     StringConstant::U8S_LocalVariableTable[] = {U_L,U_o,U_c,U_a,U_l,U_V,U_a,U_r,U_i,U_a,U_b,U_l,U_e,U_T,U_a,U_b,U_l,U_e,U_NU}, // "LocalVariableTable"
     StringConstant::U8S_S[] = {U_S,U_NU}, // "S"
     StringConstant::U8S_Sourcefile[] = {U_S,U_o,U_u,U_r,U_c,U_e,U_F,U_i,U_l,U_e,U_NU}, // "Sourcefile"
     StringConstant::U8S_Synthetic[] = {U_S,U_y,U_n,U_t,U_h,U_e,U_t,U_i,U_c,U_NU}, // "Synthetic"
     StringConstant::U8S_Deprecated[] = {U_D,U_e,U_p,U_r,U_e,U_c,U_a,U_t,U_e,U_d,U_NU}, // "Deprecated"
     StringConstant::U8S_V[] = {U_V,U_NU}, // "V"
     StringConstant::U8S_Z[] = {U_Z,U_NU}, // "Z"

     StringConstant::U8S__DO[] = {U_DO,U_NU}, // "."
     StringConstant::U8S__DO_class[] = {U_DO,U_c,U_l,U_a,U_s,U_s,U_NU}, // ".class"
     StringConstant::U8S__DO_java[] = {U_DO,U_j,U_a,U_v,U_a,U_NU}, // ".java"
     StringConstant::U8S__DO_tok[] = {U_DO,U_t,U_o,U_k,U_NU}, // ".tok"
     StringConstant::U8S__DO_u[] = {U_DO,U_u,U_NU}, // ".u"
     StringConstant::U8S__LP[] = {U_LP,U_NU}, // "("
     StringConstant::U8S__RP[] = {U_RP,U_NU}, // ")"
     StringConstant::U8S__SL[] = {U_SL,U_NU}, // "/"
     StringConstant::U8S__ST[] = {U_ST,U_NU}, // "*"

     StringConstant::U8S_class[] = {U_c,U_l,U_a,U_s,U_s,U_NU}, // "class"
     StringConstant::U8S_java[] = {U_j,U_a,U_v,U_a,U_NU}, // "java"
     StringConstant::U8S_java_SL_lang_SL_ClassNotFoundException[] = {U_j,U_a,U_v,U_a,U_SL,U_l,U_a,U_n,U_g,U_SL,U_C,U_l,U_a,U_s,U_s,U_N,U_o,U_t,U_F,U_o,U_u,U_n,U_d,U_E,U_x,U_c,U_e,U_p,U_t,U_i,U_o,U_n,U_NU}, // "java/lang/ClassNotFoundException"
     StringConstant::U8S_java_SL_lang_SL_Class[] = {U_j,U_a,U_v,U_a,U_SL,U_l,U_a,U_n,U_g,U_SL,U_C,U_l,U_a,U_s,U_s,U_NU}, // "java/lang/Class"
     StringConstant::U8S_java_SL_lang_SL_InternalError[] = {U_j,U_a,U_v,U_a,U_SL,U_l,U_a,U_n,U_g,U_SL,U_I,U_n,U_t,U_e,U_r,U_n,U_a,U_l,U_E,U_r,U_r,U_o,U_r,U_NU}, // "java/lang/InternalError"
     StringConstant::U8S_java_SL_lang_SL_NoClassDefFoundError[] = {U_j,U_a,U_v,U_a,U_SL,U_l,U_a,U_n,U_g,U_SL,U_N,U_o,U_C,U_l,U_a,U_s,U_s,U_D,U_e,U_f,U_F,U_o,U_u,U_n,U_d,U_E,U_r,U_r,U_o,U_r,U_NU}, // "java/lang/NoClassDefFoundError"
     StringConstant::U8S_java_SL_lang_SL_StringBuffer[] = {U_j,U_a,U_v,U_a,U_SL,U_l,U_a,U_n,U_g,U_SL,U_S,U_t,U_r,U_i,U_n,U_g,U_B,U_u,U_f,U_f,U_e,U_r,U_NU}, // "java/lang/StringBuffer"
     StringConstant::U8S_java_SL_lang_SL_Throwable[] = {U_j,U_a,U_v,U_a,U_SL,U_l,U_a,U_n,U_g,U_SL,U_T,U_h,U_r,U_o,U_w,U_a,U_b,U_l,U_e,U_NU}, // "java/lang/Throwable"
     StringConstant::U8S_null[] = {U_n,U_u,U_l,U_l,U_NU}, // "null"
     StringConstant::U8S_quit[] = {U_q,U_u,U_i,U_t,U_NU}, // "quit"
     StringConstant::U8S_this[] = {U_t,U_h,U_i,U_s,U_NU}, // "this"

     StringConstant::U8S_LP_LB_C_RP_Ljava_SL_lang_SL_StringBuffer_SC[] = {U_LP, U_LB, U_C, U_RP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_S, U_t, U_r, U_i, U_n, U_g, U_B, U_u, U_f, U_f, U_e, U_r, U_SC, U_NU},
     StringConstant::U8S_LP_C_RP_Ljava_SL_lang_SL_StringBuffer_SC[] = {U_LP, U_C, U_RP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_S, U_t, U_r, U_i, U_n, U_g, U_B, U_u, U_f, U_f, U_e, U_r, U_SC, U_NU},
     StringConstant::U8S_LP_Z_RP_Ljava_SL_lang_SL_StringBuffer_SC[] = {U_LP, U_Z, U_RP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_S, U_t, U_r, U_i, U_n, U_g, U_B, U_u, U_f, U_f, U_e, U_r, U_SC, U_NU},
     StringConstant::U8S_LP_I_RP_Ljava_SL_lang_SL_StringBuffer_SC[] = {U_LP, U_I, U_RP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_S, U_t, U_r, U_i, U_n, U_g, U_B, U_u, U_f, U_f, U_e, U_r, U_SC, U_NU},
     StringConstant::U8S_LP_J_RP_Ljava_SL_lang_SL_StringBuffer_SC[] = {U_LP, U_J, U_RP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_S, U_t, U_r, U_i, U_n, U_g, U_B, U_u, U_f, U_f, U_e, U_r, U_SC, U_NU},
     StringConstant::U8S_LP_F_RP_Ljava_SL_lang_SL_StringBuffer_SC[] = {U_LP, U_F, U_RP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_S, U_t, U_r, U_i, U_n, U_g, U_B, U_u, U_f, U_f, U_e, U_r, U_SC, U_NU},
     StringConstant::U8S_LP_D_RP_Ljava_SL_lang_SL_StringBuffer_SC[] = {U_LP, U_D, U_RP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_S, U_t, U_r, U_i, U_n, U_g, U_B, U_u, U_f, U_f, U_e, U_r, U_SC, U_NU},
     StringConstant::U8S_LP_Ljava_SL_lang_SL_String_SC_RP_Ljava_SL_lang_SL_StringBuffer_SC[] = {U_LP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_S, U_t, U_r, U_i, U_n, U_g, U_SC, U_RP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_S, U_t, U_r, U_i, U_n, U_g, U_B, U_u, U_f, U_f, U_e, U_r, U_SC, U_NU},
     StringConstant::U8S_LP_Ljava_SL_lang_SL_Object_SC_RP_Ljava_SL_lang_SL_StringBuffer_SC[] = {U_LP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_O, U_b, U_j, U_e, U_c, U_t, U_SC, U_RP, U_L, U_j, U_a, U_v, U_a, U_SL, U_l, U_a, U_n, U_g, U_SL, U_S, U_t, U_r, U_i, U_n, U_g, U_B, U_u, U_f, U_f, U_e, U_r, U_SC, U_NU};

char StringConstant::U8S_smallest_int[] = {U_MINUS, U_2, U_1, U_4, U_7, U_4, U_8, U_3, U_6, U_4, U_8, U_NU}, // "-2147483648"
     StringConstant::U8S_smallest_long_int[] = {U_MINUS, U_9, U_2, U_2, U_3, U_3, U_7, U_2, U_0, U_3, U_6,
                                                         U_8, U_5, U_4, U_7, U_7, U_5, U_8, U_0, U_8, U_NU}; // "-9223372036854775808"

int StringConstant::U8S_ConstantValue_length = strlen(U8S_ConstantValue),
    StringConstant::U8S_Exceptions_length = strlen(U8S_Exceptions),
    StringConstant::U8S_InnerClasses_length = strlen(U8S_InnerClasses),
    StringConstant::U8S_Synthetic_length = strlen(U8S_Synthetic),
    StringConstant::U8S_Deprecated_length = strlen(U8S_Deprecated),
    StringConstant::U8S_LineNumberTable_length = strlen(U8S_LineNumberTable),
    StringConstant::U8S_LocalVariableTable_length = strlen(U8S_LocalVariableTable),
    StringConstant::U8S_Code_length = strlen(U8S_Code),
    StringConstant::U8S_Sourcefile_length = strlen(U8S_Sourcefile),

    StringConstant::U8S_null_length = strlen(U8S_null),
    StringConstant::U8S_this_length = strlen(U8S_this);

//
// If the system runs out of memory, this function is invoked
//
#ifdef MICROSOFT
    int OutOfMemory(size_t)
    {
        fprintf(stderr, "***System Failure: Out of memory\n");
        exit(1);

        return 0;
    }

    void SetNewHandler()
    {
        _set_new_handler(OutOfMemory);
    }
#else
    void OutOfMemory()
    {
        fprintf(stderr, "***System Failure: Out of memory\n");
        exit(1);
    }

    void SetNewHandler()
    {
        set_new_handler(OutOfMemory);
    }
#endif

//
// When using the ICC compiler on Win95 or OS/2, we need to disable
// testing for various floating point exceptions. Default behavior
// was causing problems reading some standard class files.
//
#ifdef ICC
#include <float.h>
    void FloatingPointCheck()
    {
        _control87(EM_UNDERFLOW, EM_UNDERFLOW);
        _control87(EM_ZERODIVIDE, EM_ZERODIVIDE);
        _control87(EM_OVERFLOW, EM_OVERFLOW);
        _control87(EM_INVALID, EM_INVALID);

        return;
    }
#else
    void FloatingPointCheck() {}
#endif

    int SystemStat(const char *name, struct stat *stat_struct)
    {
        return stat(name, stat_struct);
    }
    FILE *SystemFopen(char *name, char *mode)
    {
        return fopen(name, mode);
    }
    size_t SystemFread(char *ptr, size_t element_size, size_t count, FILE *stream)
    {
        return fread(ptr, element_size, count, stream);
    }
    int SystemIsDirectory(char *name)
    {
        struct stat status;
        return (((::SystemStat(name, &status) == 0) && (status.st_mode & STAT_S_IFDIR)) ? 1 : 0);
    }

#if defined(GNU_LIBC5)
#include <sys/stat.h>
#ifndef UNIX
    int SystemMkdir(char *dirname)
    {
        return mkdir(dirname, S_IRWXU);
    }
#endif

#endif


//
// The configure script check each of these to see if we need our own implementation
//

#ifndef HAVE_WCSLEN
    size_t wcslen(wchar_t *cs)
    {
        int n = 0;
        while (*cs++)
            n++;

        return n;
    }
#endif

#ifndef HAVE_WCSCPY
    wchar_t *wcscpy(wchar_t *s, wchar_t *ct)
    {
        wchar_t *ptr;
        for (ptr = s; *ct; ptr++, ct++)
            *ptr = *ct;
        *ptr = U_NULL;

        return s;
    }
#endif

#ifndef HAVE_WCSNCPY
    wchar_t *wcsncpy(wchar_t *s, wchar_t *ct, int n)
    {
        wchar_t *ptr;
        for (ptr = s; *ct && n-- > 0; ptr++, ct++)
            *ptr = *ct;
        while (n-- > 0)
            *ptr++ = U_NULL;

        return s;
    }
#endif

#ifndef HAVE_WCSCAT
    wchar_t *wcscat(wchar_t *s, wchar_t *ct)
    {
        wchar_t *ptr = s;

        while (*ptr)
            ptr++;
        wcscpy(ptr, ct);

        return s;
    }
#endif

#ifndef HAVE_WCSCMP
    int wcscmp(wchar_t *cs, wchar_t *ct)
    {
        while (*cs == *ct && *cs && *ct)
        {
            cs++;
            ct++;
        }

        return (*cs == *ct ? 0 : (*cs < *ct ? -1 : 1));
    }
#endif

#ifndef HAVE_WCSNCMP
    int wcsncmp(wchar_t *cs, wchar_t *ct, int n)
    {
        while (*cs == *ct && *cs && *ct && n-- > 0)
        {
            cs++;
            ct++;
        }

        return (n <= 0 || *cs == *ct ? 0 : (*cs < *ct ? -1 : 1));
    }
#endif

#ifdef __OS2__
// changes for OS/2 from John Price, jgprice@ozemail.com.au, 2/99
#include <direct.h>
    int SystemMkdir(char *dirname)
    {
        return mkdir(dirname);
    }
#endif

#ifdef UNIX
    char PathSeparator() { return U_COLON; } // ":"

    int SystemMkdir(char *dirname)
    {
        return mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO);
    }
#else
    char PathSeparator() { return U_SEMICOLON; } // ";"
#endif

int SystemMkdirhier(char *dirname)
{
    if (::SystemIsDirectory(dirname))
        return 0;

    for (char *ptr = dirname; *ptr; ptr++)
    {
        char delimiter = *ptr;
        if (delimiter == U_SLASH)
        {
            *ptr = U_NULL;

            if (! ::SystemIsDirectory(dirname))
                ::SystemMkdir(dirname);

            *ptr = delimiter;
        }
    }

    ::SystemMkdir(dirname);
    
    return (! ::SystemIsDirectory(dirname));
}


// These next two should definitely be inlined; but when I add "inline", I
// get linker problems.


// Given three strings, return a newly-allocated string which is their concatenation.
char * strcat3(const char * prefix, const char * middle, const char * suffix) {
  int prefix_len = strlen(prefix);
  int prefix_middle_len = prefix_len + strlen(middle);

  char * result = new char[prefix_middle_len + strlen(suffix) + 1];
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
char * wstring2string(wchar_t * in) {
  char * result = new char[wcslen(in) + 1];
  result[wcslen(in)] = 0;
  for (size_t i=0; i<wcslen(in); i++) {
    wchar_t ch = in[i];
    result[i] = (ch >> 8 == 0 ? (char)ch : '?');
  }
  return result;
}

Ostream Coutput;
