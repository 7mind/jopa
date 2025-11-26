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

#pragma once


#include <config.h>

/*
   undefine any symbols defined by the autotools
   build process that can conflict with our symbols
*/

#undef PACKAGE
#undef PACKAGE_NAME
#undef VERSION

// A macro for generating <cname> vs. <name.h> as appropriate.
#ifdef HAVE_STD
# define STD_LIB_NAME(name) <c ## name>
#else
# define STD_LIB_NAME(name) <name.h>
#endif // ! HAVE_STD

/* Boilerplate autoconf checking */

#include <sys/types.h>
//FIXME: all stat stuff should be included in the platform.cpp file!
#include <sys/stat.h>
#ifdef STDC_HEADERS
# include STD_LIB_NAME(stdlib)
# include STD_LIB_NAME(stddef)
#else // ! STDC_HEADERS
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif // ! STDC_HEADERS

#ifdef HAVE_STRING_H
# ifndef STDC_HEADERS
#  ifdef HAVE_MEMORY_H
#   include <memory.h>
#  endif
# endif // ! STDC_HEADERS
# include STD_LIB_NAME(string)
#else // ! HAVE_STRING_H
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif // ! HAVE_STRING_H

#ifdef HAVE_SYS_CYGWIN_H
#include <sys/cygwin.h>
#endif

#if defined(HAVE_LIBICU_UC)
# include <unicode/ucnv.h>
#elif defined(HAVE_ICONV_H)
# include <iconv.h>
# include STD_LIB_NAME(errno)
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif



#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#ifdef HAVE_ERRNO_H
# include STD_LIB_NAME(errno)
#endif

#ifndef HAVE_WINT_T
/* On some systems the type wint_t is not defined in wchar.h */
typedef unsigned int wint_t;
#endif

#ifdef HAVE_WCHAR_H
# include STD_LIB_NAME(wchar)
#endif

#ifdef HAVE_CTYPE_H
# include STD_LIB_NAME(ctype)
#endif

#ifdef HAVE_ASSERT_H
# include STD_LIB_NAME(assert)
#endif

#ifdef HAVE_STDIO_H
# include STD_LIB_NAME(stdio)
#endif

#ifdef HAVE_LIMITS_H
# include STD_LIB_NAME(limits)
#endif

#ifdef HAVE_MATH_H
# include STD_LIB_NAME(math)
#endif

#ifdef HAVE_FLOAT_H
# include STD_LIB_NAME(float)
#endif

#ifdef HAVE_TIME_H
# include STD_LIB_NAME(time)
#endif

// C++ standard support

#ifdef HAVE_STD
# include <iostream>
# include <fstream>
#else
# include <iostream.h>
# include <fstream.h>
#endif // ! HAVE_STD

// VC++ pretends to support the C++ standard, but it does not.
// The set_new_handler method in <new> is not implemented so
// the _set_new_handler method in <new.h> must be used.

# ifdef HAVE_STD
#  include <new>
# else
#  include <new.h>
# endif // ! HAVE_STD

#ifdef HAVE_STD
# ifdef HAVE_NAMESPACES
   using namespace std;
# endif
#endif


//
// In the compiler, we want EXACT control over signed and unsigned values
// of certain bit widths. Configure should have already found us what we need.
//
#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif // HAVE_INTTYPES_H

typedef uint8_t u1;
typedef int8_t i1;
typedef uint16_t u2;
typedef int16_t i2;
typedef uint32_t u4;
typedef int32_t i4;
#ifdef HAVE_64BIT_TYPES
typedef uint64_t u8;
typedef int64_t i8;
#endif // HAVE_64BIT_TYPES
typedef u4 TokenIndex;
static const TokenIndex BAD_TOKEN = (TokenIndex) 0;

// Rename for readability in double.h.
#ifdef TYPE_INT32_T_IS_INT
#define TYPE_I4_IS_INT TYPE_INT32_T_IS_INT
#endif

//
// Some compilers do not correctly predefine the primitive type "bool"
// and its possible values: "false" and "true"
//
#ifndef HAVE_BOOL
// We define the type "bool" and the constants "false" and "true".
// The type bool as well as the constants false and true are expected
// to become standard C++. When that happens, these declarations should
// be removed.
typedef unsigned char bool;
enum { false = 0, true = 1 };
#endif


//
// Define a templatized function for dynamic_cast<> operator.
// This is slightly scary, but we need to do it so that we
// can continue to support older compilers that don't implement
// the dynamic_cast<> operator. We also do extra checking
// of the result when RTTI is supported. This does add some
// overhead, but if we catch a downcast bug as a result it
// is worth it. Downcast bugs were to blame for a number of
// core dumps in Jikes.
//
#ifdef HAVE_RTTI
# include <typeinfo>
#endif


namespace Jopa { // Open namespace Jopa block
template <typename TO, typename FROM>
inline TO DYNAMIC_CAST(FROM f)
{
#ifndef HAVE_DYNAMIC_CAST
    return (TO) f;
#else
    // If NULL, return NULL to support dynamic_cast semantics
    if (!f)
        return (TO) NULL;
    TO ptr = dynamic_cast<TO> (f);

    if (! ptr)
    {
# ifdef HAVE_RTTI
        const type_info& t = typeid(f);
        const char* name = t.name();
        fprintf(stderr, "DYNAMIC_CAST argument type was \"%s\"\n", name);
# endif // HAVE_RTTI
        assert(ptr && "Failed dynamic_cast<> in DYNAMIC_CAST");
    }
    return ptr;
#endif // HAVE_DYNAMIC_CAST
}

//
// The configure scripts check each of these to see if we need our own
// implementation
//

#ifndef HAVE_WCSLEN
    extern size_t wcslen(const wchar_t*);
#endif

#ifndef HAVE_WCSCPY
    extern wchar_t* wcscpy(wchar_t*, const wchar_t*);
#endif

#ifndef HAVE_WCSNCPY
    extern wchar_t* wcsncpy(wchar_t*, const wchar_t*, size_t);
#endif

#ifndef HAVE_WCSCAT
    extern wchar_t* wcscat(wchar_t*, const wchar_t*);
#endif

#ifndef HAVE_WCSCMP
    extern int wcscmp(const wchar_t*, const wchar_t*);
#endif

#ifndef HAVE_WCSNCMP
    extern int wcsncmp(const wchar_t*, const wchar_t*, size_t);
#endif


//
// If the system runs out of memory, this function is invoked.
//
void SetNewHandler();

extern void OutOfMemory();

//
// When using the ICC compiler on Win95 or OS/2, we need to disable
// testing for various floating point exceptions. Default behavior
// was causing problems reading some standard class files.
//
extern void FloatingPointCheck();


//
// variants of system functions
// are declared here and defined in code.cpp
//
extern int SystemStat(const char* name, struct stat* stat_struct);
extern FILE* SystemFopen(const char* name, const char* mode);
extern size_t SystemFread(char* ptr, size_t element_size, size_t count,
                          FILE* stream);
extern int SystemIsDirectory(char* name);

//
// The symbol used in this environment for separating argument in a system
// string. E.g., in a unix system directories specified in the CLASSPATH
// are separated by a ':', whereas in win95 it is ';'.
//
extern char PathSeparator();
extern int SystemMkdir(char*);
extern int SystemMkdirhier(char*);
extern int SystemMkdirhierForFile(char*);

extern char* strcat3(const char*, const char*, const char*);
extern char* wstring2string(wchar_t* in);


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// End of platform specific defines in this file, the rest of the code
// in this file should work on any system
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


// The following comment was part of the original config.h, it explains why
// some parts of the guts of the compiler are written the way they are. This
// comment was removed when the ifdef'd EBCDIC code was initially removed.
// I've added it back for its "why do they do it that way" value, hopefully
// its presence will help prevent people from making assumptions and breaking
// the code for non-ascii platforms. At some point we'll want to be able to
// compile on EBCDIC systems again, using the API and iconv/ICU instead of
// ifdefs scattered all over creation, if the core code hasn't been
// compromised this should be easier. For those that have no idea what EBCDIC
// is... it is like ASCII in that it is an encoding system, but it predates
// ASCII by a number of years (think punchcards). It is still the default
// encoding on some mainframes and minicomputers such as the IBM zSeries and
// iSeries (aka the 390 and the 400). To see what the old EBCDIC code looked
// like, pull a cvs tree with the tag v1-06.
//
//
// Jikes can be compiled on systems using the EBCDIC character set, for which
// character and string literals are translated by the compiler to EBCDIC and
// not ASCII, and so cannot be used to represent ASCII/UNICODE values. Such
// values are constructed using the U_ values defined below. Thus 'a' is
// represented using U_a, and ".java" is represented by an explicit literal:
//    {U_DOT, U_j, U_a, U_v, U_a, U_NULL}
// Variables associated with such literals have names beginning with US_ if
// the value are 16-bits or U8S_ for 8 bits. The initial underscore is
// followed by the characters being represented, with letters and digits
// representing themselves, and other values, all of which have a two
// character code, surrounded by underscore. Thus the name used for the
// literal above is US_DO_java.
//
// All string-related values are represented internally in ASCII/UNICODE
// using the U_ values defined below. EBCDIC systems also require that
// arguments to system functions representing file names be converted from
// the internal form used by Jikes to EBCDIC, and such functions are referred
// to using their usual name prefixed with "system_"; for example, a call to
// "fopen(..." is written "system_fopen(..." The "system_" procedures are
// define in the file code.cpp.
//

enum U_chars
{
    U_NULL = 0,               U_NU = U_NULL,            // L'\0'
    U_BACKSPACE = 8,          U_BS = U_BACKSPACE,       // L'\b'
    U_HORIZONTAL_TAB = 9,     U_HT = U_HORIZONTAL_TAB,  // L'\t'
    U_LINE_FEED = 10,         U_LF = U_LINE_FEED,       // L'\n'
    U_FORM_FEED = 12,         U_FF = U_FORM_FEED,       // L'\f'
    U_CARRIAGE_RETURN = 13,   U_CR = U_CARRIAGE_RETURN, // L'\r'

    U_CTL_Z = 26,
    U_ESCAPE = 27,

    U_SPACE = 32,             U_SP = U_SPACE,             // L' '
    U_EXCLAMATION = 33,       U_EX = U_EXCLAMATION,       // L'!'
    U_DOUBLE_QUOTE = 34,      U_DQ = U_DOUBLE_QUOTE,      // L'"'
    U_POUND = 35,             U_SH = U_POUND,             // L'#'
    U_DOLLAR = 36,            U_DS = U_DOLLAR,            // L'$'
    U_PERCENT = 37,           U_PE = U_PERCENT,           // L'%'
    U_AMPERSAND = 38,         U_AM = U_AMPERSAND,         // L'&'
    U_SINGLE_QUOTE = 39,      U_SQ = U_SINGLE_QUOTE,      // L'\''
    U_LEFT_PARENTHESIS = 40,  U_LP = U_LEFT_PARENTHESIS,  // L'('
    U_RIGHT_PARENTHESIS = 41, U_RP = U_RIGHT_PARENTHESIS, // L')'
    U_STAR = 42,              U_ST = U_STAR,              // L'*'
    U_PLUS = 43,              U_PL = U_PLUS,              // L'+'
    U_MINUS = 45,             U_MI = U_MINUS,             // L'-'
    U_COMMA = 44,             U_CM = U_COMMA,             // L','
    U_DOT = 46,               U_DO = U_DOT,               // L'.'
    U_SLASH = 47,             U_SL = U_SLASH,             // L'/'

    U_0 = 48, // L'0'
    U_1 = 49, // L'1'
    U_2 = 50, // L'2'
    U_3 = 51, // L'3'
    U_4 = 52, // L'4'
    U_5 = 53, // L'5'
    U_6 = 54, // L'6'
    U_7 = 55, // L'7'
    U_8 = 56, // L'8'
    U_9 = 57, // L'9'

    U_COLON = 58,             U_CO = U_COLON,     // L':'
    U_SEMICOLON = 59,         U_SC = U_SEMICOLON, // L';'
    U_LESS = 60,              U_LT = U_LESS,      // L'<'
    U_EQUAL = 61,             U_EQ = U_EQUAL,     // L'='
    U_GREATER = 62,           U_GT = U_GREATER,   // L'>'
    U_QUESTION = 63,          U_QU = U_QUESTION,  // L'?'
    U_AT = 64,                                    // L'@'

    U_A = 65, // L'A'
    U_B = 66, // L'B'
    U_C = 67, // L'C'
    U_D = 68, // L'D'
    U_E = 69, // L'E'
    U_F = 70, // L'F'
    U_G = 71, // L'G'
    U_H = 72, // L'H'
    U_I = 73, // L'I'
    U_J = 74, // L'J'
    U_K = 75, // L'K'
    U_L = 76, // L'L'
    U_M = 77, // L'M'
    U_N = 78, // L'N'
    U_O = 79, // L'O'
    U_P = 80, // L'P'
    U_Q = 81, // L'Q'
    U_R = 82, // L'R'
    U_S = 83, // L'S'
    U_T = 84, // L'T'
    U_U = 85, // L'U'
    U_V = 86, // L'V'
    U_W = 87, // L'W'
    U_X = 88, // L'X'
    U_Y = 89, // L'Y'
    U_Z = 90, // L'Z'

    U_LEFT_BRACKET = 91,      U_LB = U_LEFT_BRACKET,  // L'['
    U_BACKSLASH = 92,         U_RS = U_BACKSLASH,     // L'\\'
    U_RIGHT_BRACKET = 93,     U_RB = U_RIGHT_BRACKET, // L']'
    U_CARET = 94,             U_CA = U_CARET,         // L'^'
    U_UNDERSCORE = 95,        U_UN = U_UNDERSCORE,    // L'_'

    U_a = 97, // L'a'
    U_b = 98, // L'b'
    U_c = 99, // L'c'
    U_d = 100, // L'd'
    U_e = 101, // L'e'
    U_f = 102, // L'f'
    U_g = 103, // L'g'
    U_h = 104, // L'h'
    U_i = 105, // L'i'
    U_j = 106, // L'j'
    U_k = 107, // L'k'
    U_l = 108, // L'l'
    U_m = 109, // L'm'
    U_n = 110, // L'n',
    U_o = 111, // L'o'
    U_p = 112, // L'p'
    U_q = 113, // L'q'
    U_r = 114, // L'r'
    U_s = 115, // L's'
    U_t = 116, // L't'
    U_u = 117, // L'u'
    U_v = 118, // L'v'
    U_w = 119, // L'w'
    U_x = 120, // L'x'
    U_y = 121, // L'y'
    U_z = 122, // L'z'

    U_LEFT_BRACE = 123,       U_OS = U_LEFT_BRACE,  // L'{'
    U_BAR = 124,              U_BA = U_BAR,         // L'|'
    U_RIGHT_BRACE = 125,      U_CS = U_RIGHT_BRACE, // L'}'
    U_TILDE = 126,            U_TI = U_TILDE,       // L'~'

    U_BOM         = 0xfeff,
    U_REVERSE_BOM = 0xfffe
};


//
// Constant strings used in the program. Those prefixed with US_ are
// Unicode strings in wchar_t, and those with U8S are UTF8 strings in char.
//
class StringConstant
{
public:
    //
    // Symbols and operators.
    //
    static const wchar_t US_AND[]; // L"&"
    static const wchar_t US_AND_AND[]; // L"&&"
    static const wchar_t US_AND_EQUAL[]; // L"&="
    static const wchar_t US_AT[]; // L"@"
    static const wchar_t US_COLON[]; // L":"
    static const wchar_t US_COMMA[]; // L";"
    static const wchar_t US_DIVIDE[]; // L"/"
    static const wchar_t US_DIVIDE_EQUAL[]; // L"/="
    static const wchar_t US_DOT[]; // L"."
    static const wchar_t US_DOT_DOT_DOT[]; // L"..."
    static const wchar_t US_EMPTY[]; // L""
    static const wchar_t US_EOF[]; // L"EOF"
    static const wchar_t US_EQUAL[]; // L"="
    static const wchar_t US_EQUAL_EQUAL[]; // L"=="
    static const wchar_t US_GREATER[]; // L">"
    static const wchar_t US_GREATER_EQUAL[]; // L">="
    static const wchar_t US_LBRACE[]; // L"{"
    static const wchar_t US_LBRACKET[]; // L"["
    static const wchar_t US_LEFT_SHIFT[]; // L"<<"
    static const wchar_t US_LEFT_SHIFT_EQUAL[]; // L"<<="
    static const wchar_t US_LESS[]; // L"<"
    static const wchar_t US_LESS_EQUAL[]; // L"<="
    static const wchar_t US_LPAREN[]; // L"("
    static const wchar_t US_MINUS[]; // L"-"
    static const wchar_t US_MINUS_EQUAL[]; // L"-="
    static const wchar_t US_MINUS_MINUS[]; // L"--"
    static const wchar_t US_MULTIPLY[]; // L"*"
    static const wchar_t US_MULTIPLY_EQUAL[]; // L"*="
    static const wchar_t US_NOT[]; // L"!"
    static const wchar_t US_NOT_EQUAL[]; // L"!="
    static const wchar_t US_OR[]; // L"|"
    static const wchar_t US_OR_EQUAL[]; // L"|="
    static const wchar_t US_OR_OR[]; // L"||"
    static const wchar_t US_PLUS[]; // L"+"
    static const wchar_t US_PLUS_EQUAL[]; // L"+="
    static const wchar_t US_PLUS_PLUS[]; // L"++"
    static const wchar_t US_QUESTION[]; // L"?"
    static const wchar_t US_RBRACE[]; // L"}"
    static const wchar_t US_RBRACKET[]; // L"]"
    static const wchar_t US_REMAINDER[]; // L"%"
    static const wchar_t US_REMAINDER_EQUAL[]; // L"%="
    static const wchar_t US_RIGHT_SHIFT[]; // L">>"
    static const wchar_t US_RIGHT_SHIFT_EQUAL[]; // L">>="
    static const wchar_t US_RPAREN[]; // L")"
    static const wchar_t US_SEMICOLON[]; // L";"
    static const wchar_t US_TWIDDLE[]; // L"~"
    static const wchar_t US_UNSIGNED_RIGHT_SHIFT[]; // L">>>"
    static const wchar_t US_UNSIGNED_RIGHT_SHIFT_EQUAL[]; // L">>>="
    static const wchar_t US_XOR[]; // L"^"
    static const wchar_t US_XOR_EQUAL[]; // L"^="


    //
    // Unicode strings for files and packages.
    //
    static const wchar_t US_DS[]; // L"$"
    static const wchar_t US_LB_RB[]; // L"[]"
    static const wchar_t US_MI[]; // L"-"
    static const wchar_t US_SC[]; // L";"
    static const wchar_t US_SL[]; // L"/"
    static const wchar_t US_jar[]; // L"jar"
    static const wchar_t US_java_SL_io[]; // L"java/io"
    static const wchar_t US_java_SL_lang[]; // L"java/lang"
    // L"java/lang/annotation"
    static const wchar_t US_java_SL_lang_SL_annotation[];
    static const wchar_t US_java_SL_util[]; // L"java/util"
    static const wchar_t US_zip[]; // L"zip"

    //
    // Java keywords.
    //
    static const wchar_t US_abstract[]; // L"abstract"
    static const wchar_t US_assert[]; // L"assert"
    static const wchar_t US_boolean[]; // L"boolean"
    static const wchar_t US_break[]; // L"break"
    static const wchar_t US_byte[]; // L"byte"
    static const wchar_t US_case[]; // L"case"
    static const wchar_t US_catch[]; // L"catch"
    static const wchar_t US_char[]; // L"char"
    static const wchar_t US_class[]; // L"class"
    static const wchar_t US_const[]; // L"const"
    static const wchar_t US_continue[]; // L"continue"
    static const wchar_t US_default[]; // L"default"
    static const wchar_t US_do[]; // L"do"
    static const wchar_t US_double[]; // L"double"
    static const wchar_t US_else[]; // L"else"
    static const wchar_t US_enum[]; // L"enum"
    static const wchar_t US_extends[]; // L"extends"
    static const wchar_t US_false[]; // L"false"
    static const wchar_t US_final[]; // L"final"
    static const wchar_t US_finally[]; // L"finally"
    static const wchar_t US_float[]; // L"float"
    static const wchar_t US_for[]; // L"for"
    static const wchar_t US_goto[]; // L"goto"
    static const wchar_t US_if[]; // L"if"
    static const wchar_t US_implements[]; // L"implements"
    static const wchar_t US_import[]; // L"import"
    static const wchar_t US_instanceof[]; // L"instanceof"
    static const wchar_t US_int[]; // L"int"
    static const wchar_t US_interface[]; // L"interface"
    static const wchar_t US_long[]; // L"long"
    static const wchar_t US_native[]; // L"native"
    static const wchar_t US_new[]; // L"new"
    static const wchar_t US_null[]; // L"null"
    static const wchar_t US_package[]; // L"package"
    static const wchar_t US_private[]; // L"private"
    static const wchar_t US_protected[]; // L"protected"
    static const wchar_t US_public[]; // L"public"
    static const wchar_t US_return[]; // L"return"
    static const wchar_t US_short[]; // L"short"
    static const wchar_t US_static[]; // L"static"
    static const wchar_t US_strictfp[]; // L"strictfp"
    static const wchar_t US_super[]; // L"super"
    static const wchar_t US_switch[]; // L"switch"
    static const wchar_t US_synchronized[]; // L"synchronized"
    static const wchar_t US_this[]; // L"this"
    static const wchar_t US_throw[]; // L"throw"
    static const wchar_t US_throws[]; // L"throws"
    static const wchar_t US_transient[]; // L"transient"
    static const wchar_t US_true[]; // L"true"
    static const wchar_t US_try[]; // L"try"
    static const wchar_t US_void[]; // L"void"
    static const wchar_t US_volatile[]; // L"volatile"
    static const wchar_t US_while[]; // L"while"

    //
    // Miscellaneous strings.
    //
    static const char U8S_help_header[];
    static const char U8S_command_format[];

    //
    // Constant pool entries.
    //
    static const char U8S_AnnotationDefault[]; // "AnnotationDefault"
    static const char U8S_Code[]; // "Code"
    static const char U8S_ConstantValue[]; // "ConstantValue"
    static const char U8S_Deprecated[]; // "Deprecated"
    static const char U8S_EnclosingMethod[]; // "EnclosingMethod"
    static const char U8S_Exceptions[]; // "Exceptions"
    static const char U8S_InnerClasses[]; // "InnerClasses"
    static const char U8S_LineNumberTable[]; // "LineNumberTable"
    static const char U8S_LocalVariableTable[]; // "LocalVariableTable"
    static const char U8S_LocalVariableTypeTable[]; // "LocalVariableTypeTable"
    // "RuntimeInvisibleAnnotations"
    static const char U8S_RuntimeInvisibleAnnotations[];
    // "RuntimeInvisibleParameterAnnotations"
    static const char U8S_RuntimeInvisibleParameterAnnotations[];
    // "RuntimeVisibleAnnotations"
    static const char U8S_RuntimeVisibleAnnotations[];
    // "RuntimeVisibleParameterAnnotations"
    static const char U8S_RuntimeVisibleParameterAnnotations[];
    static const char U8S_Signature[]; // "Signature"
    static const char U8S_SourceFile[]; // "SourceFile"
    static const char U8S_StackMap[]; // "StackMap"
    static const char U8S_StackMapTable[]; // "StackMapTable"
    static const char U8S_Synthetic[]; // "Synthetic"

    //
    // ASCII file names.
    //
    static const char U8S_DO_class[]; // ".class"
    static const char U8S_DO_java[]; // ".java"
    static const char U8S_DO_tok[]; // ".tok"
    static const char U8S_DO_u[]; // ".u"
    static const char U8S_LP[]; // "("
    static const char U8S_RP[]; // ")"
    static const char U8S_SL[]; // "/"

    //
    // Convert number to string.
    //
    static const char U8S_NaN[]; // "NaN"
    static const char U8S_pos_Infinity[]; // "Infinity"
    static const char U8S_neg_Infinity[]; // "-Infinity"
    static const char U8S_pos_Zero[]; // "0.0"
    static const char U8S_neg_Zero[]; // "-0.0"
};


//
// Convert an integer to its character string representation.
//
class IntToString
{
public:
    IntToString(i4); // Signed decimal conversion.
    IntToString(u4, int width); // Unsigned zero-padded hexadecimal.

    const char* String() { return str; }
    int Length() { return (&info[TAIL_INDEX]) - str; }

private:
    enum { TAIL_INDEX = 1 + 10 }; // 1 for sign, +10 significant digits

    char info[TAIL_INDEX + 1]; // +1 for '\0'
    char* str;
};


//
// Same as IntToString for wide strings.
//
class IntToWstring
{
public:
    IntToWstring(i4);

    const wchar_t* String() { return wstr; }
    int Length() { return (&winfo[TAIL_INDEX]) - wstr; }

private:
    enum { TAIL_INDEX = 1 + 10 }; // 1 for sign, +10 significant digits

    wchar_t winfo[TAIL_INDEX + 1]; // 1 for sign, +10 significant digits + '\0'
    wchar_t* wstr;
};


//
// Convert a Long integer to its character string representation.
//
class BaseLong;
class ULongInt;
class LongInt;
class LongToString
{
public:
    LongToString(const LongInt&); // Signed decimal conversion.
    LongToString(const ULongInt&); // Unsigned decimal conversion.
    // Unsigned hexadecimal or octal, with optional base designator.
    LongToString(const BaseLong&, bool octal = false);

    const char* String() { return str; }
    const char* StringWithBase() { return base; }
    int Length() { return (&info[TAIL_INDEX]) - str; }

private:
    enum { TAIL_INDEX = 23 }; // 22 octal digits + base designator

    char info[TAIL_INDEX + 1]; // +1 for '\0'
    char* str;
    char* base;
};


//
// Convert an double to its character string representation.
//
class IEEEdouble;
class DoubleToString
{
public:
    DoubleToString(const IEEEdouble&);

    const char* String() const { return str; }
    int Length() const { return length; }

private:
    void Format(char*, int, bool);

    enum
    {
        MAXIMUM_PRECISION = 17,
        // +1 for sign, +17 significant digits +1 for ".", +5 for exponent
        MAXIMUM_STR_LENGTH = 1 + MAXIMUM_PRECISION + 1 + 5
    };

    char str[MAXIMUM_STR_LENGTH + 1]; // +1 for '\0'
    int length;
};

//
// Convert an float to its character string representation.
//
class IEEEfloat;
class FloatToString
{
public:
    FloatToString(const IEEEfloat&);

    const char* String() const { return str; }
    int Length() const   { return length; }

private:
    void Format(char*, int, bool);

    enum
    {
        MAXIMUM_PRECISION = 9,
        // +1 for sign, +9 significant digits +1 for ".", +4 for exponent
        MAXIMUM_STR_LENGTH = 1 + MAXIMUM_PRECISION + 1 + 4
    };

    char str[MAXIMUM_STR_LENGTH + 1]; // +1 for '\0'
    int length;
};


class LongInt;
class ULongInt;
//
//
//
class Ostream
{
    ostream* os;

    bool expand_wchar;

public:

    Ostream() : os(&cerr),
                expand_wchar(false)
    {}

    Ostream(ostream* _os)
        : os(_os)
        , expand_wchar(false)
    {}

    void StandardOutput() { os = &cout; }
    void StandardError() { os = &cerr; }
    void SetExpandWchar(bool val = true) { expand_wchar = val; }
    bool ExpandWchar() { return expand_wchar; }

    Ostream& operator<<(wchar_t ch)
    {
        // output only printable characters directly
        if (ch == U_CARRIAGE_RETURN || ch == U_LINE_FEED)
            *os << (char) U_LINE_FEED;
        else if (ch >= U_SPACE && ch < 0x0ff)
            *os << (char) ch;
        else
        {
            if (expand_wchar == 0)
                *os << (char) U_QUESTION;
            else
            {
                *os << (char) U_BACKSLASH
                    << (char) U_u;
                char str[4];
                for (int i = 3; i >= 0; i--)
                {
                    int d = ch & 0x0f;
                    switch (d)
                    {
                    case 10: case 11: case 12: case 13: case 14: case 15:
                        str[i] = U_A - 10 + d;
                        break;
                    default:
                        str[i] = U_0 + d;
                        break;
                    }
                    ch >>= 4;
                }
                *os << str[0] << str[1] << str[2] << str[3];
            }
        }

        return *this;
    }

    Ostream& operator<<(const wchar_t* str)
    {
        for ( ; *str; str++)
            (*this) << *str;
        return *this;
    }


    Ostream& operator<<(char c)
    {
        *os << c;
        return *this;
    }

    Ostream& operator<<(signed char c)
    {
        *os << c;
        return *this;
    }

    Ostream& operator<<(unsigned char c)
    {
        *os << c;
        return *this;
    }

    Ostream& operator<<(const char* c)
    {
        *os << c;
        return *this;
    }

    Ostream& operator<<(const signed char* c)
    {
        *os << c;
        return *this;
    }

    Ostream& operator<<(const unsigned char* c)
    {
#ifndef HAVE_OSTREAM_CONST_UNSIGNED_CHAR_PTR
# ifdef HAVE_CONST_CAST
        *os << const_cast<unsigned char*> (c);
# else
        *os << (unsigned char*) c;
# endif
#else
        *os << c;
#endif
        return *this;
    }

    Ostream& operator<<(int a)
    {
        *os << a;
        return *this;
    }

    Ostream& operator<<(unsigned int a)
    {
        *os << a;
        return *this;
    }

    Ostream& operator<<(long a)
    {
        *os << a;
        return *this;
    }

    Ostream& operator<<(unsigned long a)
    {
        *os << a;
        return *this;
    }

    Ostream& operator<<(LongInt);
    Ostream& operator<<(ULongInt);

    Ostream& operator<<(float f)
    {
        *os << f;
        return *this;
    }

    Ostream& operator<<(double d)
    {
        *os << d;
        return *this;
    }

    char fill(char c) { return os -> fill(c); }

    Ostream& flush()
    {
        os -> flush();
        return *this;
    }

    int width(int w)
    {
        return os -> width(w);
    }

    Ostream& operator<<(ios& (*f)(ios&))
    {
        (*f)(*os);
        return *this;
    }

    Ostream& operator<<(ostream& (*f)(ostream&))
    {
        (*f)(*os);
        return *this;
    }
};

extern Ostream Coutput;

//
// From now on, DO NOT USE cout or cerr !
// Instead, use Coutput, which wraps either cout or cerr as determined by
// command line flags, and translates unicode output properly.
// If you try to use cerr or cout this define will
// create an undefined symbol and give you a linker error.
//
#define cout Please_Do_Not_Use_cout_Directly_But_use_an_instance_of_Ostream_with_cout_as_argument
#define cerr Please_Do_Not_Use_cerr_Directly_But_use_an_instance_of_Ostream_with_cerr_as_argument


} // Close namespace Jopa block

