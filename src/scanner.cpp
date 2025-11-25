#include "scanner.h"
#include "control.h"
#include "error.h"
#include "grammar/javadef.h"
#include "grammar/javasym.h"
#include "option.h"
#include "code.h"


namespace Jopa { // Open namespace Jopa block
int (*Scanner::scan_keyword[13]) (const wchar_t* p1) =
{
    ScanKeyword0,
    ScanKeyword0,
    ScanKeyword2,
    ScanKeyword3,
    ScanKeyword4,
    ScanKeyword5,
    ScanKeyword6,
    ScanKeyword7,
    ScanKeyword8,
    ScanKeyword9,
    ScanKeyword10,
    ScanKeyword0,
    ScanKeyword12
};


//
// The constructor initializes all utility variables.
//
Scanner::Scanner(Control& control_)
    : control(control_),
      dollar_warning_given(false),
      deprecated(false)
{
    //
    // If this assertion fails, the Token structure in stream.h must be
    // redesigned !!!
    //
    assert(NUM_TERMINALS < 128);
    //
    // If this assertion fails, then gencode.java is at fault.
    //
#ifdef JOPA_DEBUG
    assert(Code::CodeCheck());
#endif // JOPA_DEBUG

    //
    // CLASSIFY_TOKEN is a mapping from each character into a
    // classification routine that is invoked when that character
    // is the first character encountered in a token.
    //
    for (int c = 0; c < 128; c++)
    {
        if (Code::IsAsciiUpper(c) || Code::IsAsciiLower(c) || c == U_DOLLAR ||
            c == U_UNDERSCORE)
        {
            classify_token[c] = &Scanner::ClassifyId;
        }
        else if (Code::IsDecimalDigit(c))
            classify_token[c] = &Scanner::ClassifyNumericLiteral;
        else if (Code::IsSpace(c))
            classify_token[c] = &Scanner::SkipSpaces;
        else classify_token[c] = &Scanner::ClassifyBadToken;
    }
    classify_token[128] = &Scanner::ClassifyNonAsciiUnicode;

    classify_token[U_a] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_b] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_c] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_d] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_e] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_f] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_g] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_i] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_l] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_n] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_p] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_r] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_s] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_t] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_v] = &Scanner::ClassifyIdOrKeyword;
    classify_token[U_w] = &Scanner::ClassifyIdOrKeyword;

    classify_token[U_SINGLE_QUOTE] = &Scanner::ClassifyCharLiteral;
    classify_token[U_DOUBLE_QUOTE] = &Scanner::ClassifyStringLiteral;

    classify_token[U_PLUS] = &Scanner::ClassifyPlus;
    classify_token[U_MINUS] = &Scanner::ClassifyMinus;
    classify_token[U_EXCLAMATION] = &Scanner::ClassifyNot;
    classify_token[U_PERCENT] = &Scanner::ClassifyMod;
    classify_token[U_CARET] = &Scanner::ClassifyXor;
    classify_token[U_AMPERSAND] = &Scanner::ClassifyAnd;
    classify_token[U_STAR] = &Scanner::ClassifyStar;
    classify_token[U_BAR] = &Scanner::ClassifyOr;
    classify_token[U_TILDE] = &Scanner::ClassifyComplement;
    classify_token[U_SLASH] = &Scanner::ClassifySlash;
    classify_token[U_GREATER] = &Scanner::ClassifyGreater;
    classify_token[U_LESS] = &Scanner::ClassifyLess;
    classify_token[U_LEFT_PARENTHESIS] = &Scanner::ClassifyLparen;
    classify_token[U_RIGHT_PARENTHESIS] = &Scanner::ClassifyRparen;
    classify_token[U_LEFT_BRACE] = &Scanner::ClassifyLbrace;
    classify_token[U_RIGHT_BRACE] = &Scanner::ClassifyRbrace;
    classify_token[U_LEFT_BRACKET] = &Scanner::ClassifyLbracket;
    classify_token[U_RIGHT_BRACKET] = &Scanner::ClassifyRbracket;
    classify_token[U_SEMICOLON] = &Scanner::ClassifySemicolon;
    classify_token[U_QUESTION] = &Scanner::ClassifyQuestion;
    classify_token[U_COLON] = &Scanner::ClassifyColon;
    classify_token[U_COMMA] = &Scanner::ClassifyComma;
    classify_token[U_DOT] = &Scanner::ClassifyPeriod;
    classify_token[U_EQUAL] = &Scanner::ClassifyEqual;
    classify_token[U_AT] = &Scanner::ClassifyAt;
}


//
// Associate a lexical stream with this file. Remember, we doctored the stream
// to start with \n so that we always start on a whitespace token, and so that
// the first source code line is line 1.
//
void Scanner::Initialize(FileSymbol* file_symbol)
{
    lex = new LexStream(control, file_symbol);
    current_token_index = lex -> GetNextToken(); // Get 0th token.
    current_token = &(lex -> token_stream[current_token_index]);
    current_token -> SetKind(0);

#ifdef JOPA_DEBUG
    if (control.option.debug_comments)
    {
        // Add 0th comment.
        LexStream::Comment* current_comment = &(lex -> comment_stream.Next());
        current_comment -> string = NULL;
        current_comment -> length = 0;
        current_comment -> previous_token = BAD_TOKEN;
        current_comment -> location = 0;
    }
#endif // JOPA_DEBUG

    lex -> line_location.Next() = 0; // Mark starting location of line # 0
}


//
// This is one of the main entry point for the Java lexical analyser. Its
// input is the name of a regular text file. Its output is a stream of tokens.
//
void Scanner::SetUp(FileSymbol* file_symbol)
{
    Initialize(file_symbol);
    lex -> CompressSpace();
    file_symbol -> lex_stream = lex;
}


//
// This is one of the main entry point for the Java lexical analyser. Its
// input is the name of a regular text file. Its output is a stream of tokens.
//
void Scanner::Scan(FileSymbol* file_symbol)
{
    Initialize(file_symbol);
    lex -> ReadInput();
    cursor = lex -> InputBuffer();
    if (cursor)
    {
        Scan();
        lex -> CompressSpace();

        if (control.option.dump_errors)
        {
            lex -> SortMessages();
            for (unsigned i = 0; i < lex -> bad_tokens.Length(); i++)
                JopaAPI::getInstance() ->
                    reportError(&(lex -> bad_tokens[i]));
        }
        lex -> DestroyInput(); // get rid of input buffer
    }
    else
    {
        delete lex;
        lex = NULL;
    }
    file_symbol -> lex_stream = lex;
}


//
// Scan the InputBuffer() and process all tokens and comments.
//
void Scanner::Scan()
{
    input_buffer_tail = &cursor[lex -> InputBufferLength()];

    //
    // CURSOR is assumed to point to the next character to be scanned.
    // Using CURSOR, we jump to the proper classification function
    // which scans and classifies the token and returns the location of
    // the character immediately following it.
    //
    do
    {
        //
        // Allocate space for next token and set its location.
        //
        if (! current_token_index || current_token -> Kind())
        {
            current_token_index =
                lex -> GetNextToken(cursor - lex -> InputBuffer());
            current_token = &(lex -> token_stream[current_token_index]);
        }
        else
        {
            current_token -> ResetInfoAndSetLocation(cursor -
                                                     lex -> InputBuffer());
        }
        if (deprecated)
        {
            current_token -> SetDeprecated();
            deprecated = false;
        }
        (this ->* classify_token[*cursor < 128 ? *cursor : 128])();
    } while (cursor < input_buffer_tail);

    //
    // Add a a gate after the last line.
    //
    lex -> line_location.Next() = input_buffer_tail - lex -> InputBuffer();
    current_token -> SetKind(TK_EOF);

    //
    // If the brace_stack is not empty, then there are unmatched left
    // braces in the input. Each unmatched left brace should point to
    // the EOF token as a substitute for a matching right brace.
    //
    assert(current_token_index == lex -> token_stream.Length() - 1);

    for (TokenIndex left_brace = brace_stack.Top();
         left_brace; left_brace = brace_stack.Top())
    {
        lex -> token_stream[left_brace].SetRightBrace(current_token_index);
        brace_stack.Pop();
    }
}


//
// CURSOR points to the first '*' in a /**/ comment.
//
void Scanner::ScanStarComment()
{
    const wchar_t* start = cursor - 1;
    current_token -> SetKind(0);
#ifdef JOPA_DEBUG
    LexStream::Comment* current_comment = NULL;
    if (control.option.debug_comments)
    {
        current_comment = &(lex -> comment_stream.Next());
        current_comment -> string = NULL;
        current_comment -> previous_token = current_token_index - 1;
        current_comment -> location = start - lex -> InputBuffer();
    }
#endif // JOPA_DEBUG

    //
    // If this comment starts with the prefix "/**" then it is a document
    // comment. Check whether or not it contains the deprecated tag and if so,
    // mark the token preceeding it. The @deprecated tag must appear at the
    // beginning of a line. According to Sun,
    // http://java.sun.com/j2se/1.4/docs/tooldocs/win32/javadoc.html#comments,
    // this means ignoring whitespace, *, and /** patterns. But in practice,
    // javac doesn't quite implement it this way, completely ignoring /**
    // separators, and rejecting \f and \t after *<space>*.
    // This implementation also ignores /**, but treats whitespace correctly.
    //
    // Note that we exploit the fact that the stream is doctored to always
    // end in U_CARRIAGE_RETURN, U_NULL; and that we changed all CR to LF
    // within the file.
    //
    if (*++cursor == U_STAR)
    {
        enum
        {
            HEADER,
            STAR,
            REMAINDER
        } state = HEADER;
        while (*cursor != U_CARRIAGE_RETURN)
        {
            switch (*cursor++)
            {
            case U_LINE_FEED:
                // Record new line.
                lex -> line_location.Next() = cursor - lex -> InputBuffer();
                state = HEADER;
                break;
            case U_SPACE:
            case U_FORM_FEED:
            case U_HORIZONTAL_TAB:
                if (state != REMAINDER)
                    state = HEADER;
                break;
            case U_STAR:
                if (state != REMAINDER || *cursor == U_SLASH)
                    state = STAR;
                break;
            case U_SLASH:
                if (state == STAR)
                {
#ifdef JOPA_DEBUG
                    if (control.option.debug_comments)
                        current_comment -> length = cursor - start;
#endif // JOPA_DEBUG
                    return;
                }
                // fallthrough
            default:
                if (state != REMAINDER)
                {
                    state = REMAINDER;
                    if (cursor[-1] == U_AT &&
                        cursor[0] == U_d &&
                        cursor[1] == U_e &&
                        cursor[2] == U_p &&
                        cursor[3] == U_r &&
                        cursor[4] == U_e &&
                        cursor[5] == U_c &&
                        cursor[6] == U_a &&
                        cursor[7] == U_t &&
                        cursor[8] == U_e &&
                        cursor[9] == U_d &&
                        (Code::IsWhitespace(cursor + 10) ||
                         cursor[10] == U_STAR))
                    {
                        deprecated = true;
                        cursor += 9;
                    }
                }
            }
        }
    }
    else // normal /* */ comment
    {
        // Normal comments do not affect deprecation.
        if (current_token -> Deprecated())
            deprecated = true;
        while (*cursor != U_CARRIAGE_RETURN)
        {
            if (*cursor == U_STAR) // Potential comment closer.
            {
                while (*++cursor == U_STAR)
                    ;
                if (*cursor == U_SLASH)
                {
                    cursor++;
#ifdef JOPA_DEBUG
                    if (control.option.debug_comments)
                        current_comment -> length = cursor - start;
#endif // JOPA_DEBUG
                    return;
                }
                if (*cursor == U_CARRIAGE_RETURN)
                    break;
            }
            if (Code::IsNewline(*cursor++)) // Record new line.
            {
                lex -> line_location.Next() = cursor - lex -> InputBuffer();
            }
        }
    }

    //
    // If we got here, we are in an unterminated comment. Discard the
    // U_CARRIAGE_RETURN that ends the stream.
    //
    lex -> ReportMessage(StreamError::UNTERMINATED_COMMENT,
                         start - lex -> InputBuffer(),
                         cursor - lex -> InputBuffer() - 1);

#ifdef JOPA_DEBUG
    if (control.option.debug_comments)
        current_comment -> length = cursor - 1 - start;
#endif // JOPA_DEBUG
}


//
// CURSOR points to the second '/' in a // comment.
//
void Scanner::ScanSlashComment()
{
    //
    // Note that we exploit the fact that the stream is doctored to always
    // end in U_CARRIAGE_RETURN, U_NULL; and that we changed all CR to LF
    // within the file. Normal comments do not affect deprecation.
    //
    if (current_token -> Deprecated())
        deprecated = true;
    current_token -> SetKind(0);
    while (! Code::IsNewline(*++cursor));  // Skip all until \n or EOF
#ifdef JOPA_DEBUG
    if (control.option.debug_comments)
    {
        LexStream::Comment* current_comment = &(lex -> comment_stream.Next());
        current_comment -> string = NULL;
        current_comment -> previous_token = current_token_index - 1;
        current_comment -> location = current_token -> Location();
        current_comment -> length = (cursor - lex -> InputBuffer()) -
            current_comment -> location;
    }
#endif // JOPA_DEBUG
}


//
// This procedure is invoked to skip useless spaces in the input.
// It assumes upon entry that CURSOR points to the next character to
// be scanned.  Before returning it sets CURSOR to the location of the
// first non-space character following its initial position.
//
inline void Scanner::SkipSpaces()
{
    //
    // We exploit the fact that the stream was doctored to end in
    // U_CARRIAGE_RETURN, U_NULL; and that all internal CR were changed to LF.
    // Normal comments do not affect deprecation.
    //
    if (current_token -> Deprecated())
        deprecated = true;
    current_token -> SetKind(0);
    do
    {
        if (Code::IsNewline(*cursor))  // Starting a new line?
            lex -> line_location.Next() = cursor + 1 - lex -> InputBuffer();
    } while (Code::IsSpace(*++cursor));
}


//
// scan_keyword(i):
// Scan an identifier of length I and determine if it is a keyword.
//
int Scanner::ScanKeyword0(const wchar_t*)
{
    return TK_Identifier;
}

int Scanner::ScanKeyword2(const wchar_t* p1)
{
    if (p1[0] == U_d && p1[1] == U_o)
        return TK_do;
    if (p1[0] == U_i && p1[1] == U_f)
        return TK_if;
    return TK_Identifier;
}

int Scanner::ScanKeyword3(const wchar_t* p1)
{
    switch (*p1)
    {
    case U_f:
        if (p1[1] == U_o && p1[2] == U_r)
            return TK_for;
        break;
    case U_i:
        if (p1[1] == U_n && p1[2] == U_t)
            return TK_int;
        break;
    case U_n:
        if (p1[1] == U_e && p1[2] == U_w)
            return TK_new;
        break;
    case U_t:
        if (p1[1] == U_r && p1[2] == U_y)
            return TK_try;
        break;
    }
    return TK_Identifier;
}

int Scanner::ScanKeyword4(const wchar_t* p1)
{
    switch (*p1)
    {
    case U_b:
        if (p1[1] == U_y && p1[2] == U_t && p1[3] == U_e)
            return TK_byte;
        break;
    case U_c:
        if (p1[1] == U_a && p1[2] == U_s && p1[3] == U_e)
            return TK_case;
        if (p1[1] == U_h && p1[2] == U_a && p1[3] == U_r)
            return TK_char;
        break;
    case U_e:
        if (p1[1] == U_l && p1[2] == U_s && p1[3] == U_e)
            return TK_else;
        if (p1[1] == U_n && p1[2] == U_u && p1[3] == U_m)
            return TK_enum;
        break;
    case U_g:
        if (p1[1] == U_o && p1[2] == U_t && p1[3] == U_o)
            return TK_goto;
        break;
    case U_l:
        if (p1[1] == U_o && p1[2] == U_n && p1[3] == U_g)
            return TK_long;
        break;
    case U_n:
        if (p1[1] == U_u && p1[2] == U_l && p1[3] == U_l)
            return TK_null;
        break;
    case U_t:
        if (p1[1] == U_h && p1[2] == U_i && p1[3] == U_s)
            return TK_this;
        if (p1[1] == U_r && p1[2] == U_u && p1[3] == U_e)
            return TK_true;
        break;
    case U_v:
        if (p1[1] == U_o && p1[2] == U_i && p1[3] == U_d)
            return TK_void;
        break;
    }
    return TK_Identifier;
}

int Scanner::ScanKeyword5(const wchar_t* p1)
{
    switch (*p1)
    {
    case U_b:
        if (p1[1] == U_r && p1[2] == U_e && p1[3] == U_a && p1[4] == U_k)
            return TK_break;
        break;
    case U_c:
        if (p1[1] == U_a && p1[2] == U_t && p1[3] == U_c && p1[4] == U_h)
            return TK_catch;
        if (p1[1] == U_l && p1[2] == U_a && p1[3] == U_s && p1[4] == U_s)
            return TK_class;
        if (p1[1] == U_o && p1[2] == U_n && p1[3] == U_s && p1[4] == U_t)
            return TK_const;
        break;
    case U_f:
        if (p1[1] == U_a && p1[2] == U_l && p1[3] == U_s && p1[4] == U_e)
            return TK_false;
        if (p1[1] == U_i && p1[2] == U_n && p1[3] == U_a && p1[4] == U_l)
            return TK_final;
        if (p1[1] == U_l && p1[2] == U_o && p1[3] == U_a && p1[4] == U_t)
            return TK_float;
        break;
    case U_s:
        if (p1[1] == U_h && p1[2] == U_o && p1[3] == U_r && p1[4] == U_t)
            return TK_short;
        if (p1[1] == U_u && p1[2] == U_p && p1[3] == U_e && p1[4] == U_r)
            return TK_super;
        break;
    case U_t:
        if (p1[1] == U_h && p1[2] == U_r && p1[3] == U_o && p1[4] == U_w)
            return TK_throw;
        break;
    case U_w:
        if (p1[1] == U_h && p1[2] == U_i && p1[3] == U_l && p1[4] == U_e)
            return TK_while;
        break;
    }
    return TK_Identifier;
}

int Scanner::ScanKeyword6(const wchar_t* p1)
{
    switch (*p1)
    {
    case U_a:
        if (p1[1] == U_s && p1[2] == U_s &&
            p1[3] == U_e && p1[4] == U_r && p1[5] == U_t)
            return TK_assert;
        break;
    case U_d:
        if (p1[1] == U_o && p1[2] == U_u &&
            p1[3] == U_b && p1[4] == U_l && p1[5] == U_e)
            return TK_double;
        break;
    case U_i:
        if (p1[1] == U_m && p1[2] == U_p &&
            p1[3] == U_o && p1[4] == U_r && p1[5] == U_t)
            return TK_import;
        break;
    case U_n:
        if (p1[1] == U_a && p1[2] == U_t &&
            p1[3] == U_i && p1[4] == U_v && p1[5] == U_e)
            return TK_native;
        break;
    case U_p:
        if (p1[1] == U_u && p1[2] == U_b &&
            p1[3] == U_l && p1[4] == U_i && p1[5] == U_c)
            return TK_public;
        break;
    case U_r:
        if (p1[1] == U_e && p1[2] == U_t &&
            p1[3] == U_u && p1[4] == U_r && p1[5] == U_n)
            return TK_return;
        break;
    case U_s:
        if (p1[1] == U_t && p1[2] == U_a &&
            p1[3] == U_t && p1[4] == U_i && p1[5] == U_c)
            return TK_static;
        if (p1[1] == U_w && p1[2] == U_i &&
            p1[3] == U_t && p1[4] == U_c && p1[5] == U_h)
            return TK_switch;
        break;
    case U_t:
        if (p1[1] == U_h && p1[2] == U_r &&
            p1[3] == U_o && p1[4] == U_w && p1[5] == U_s)
            return TK_throws;
        break;
    }
    return TK_Identifier;
}

int Scanner::ScanKeyword7(const wchar_t* p1)
{
    switch (*p1)
    {
    case U_b:
        if (p1[1] == U_o && p1[2] == U_o && p1[3] == U_l &&
            p1[4] == U_e && p1[5] == U_a && p1[6] == U_n)
            return TK_boolean;
        break;
    case U_d:
        if (p1[1] == U_e && p1[2] == U_f && p1[3] == U_a &&
            p1[4] == U_u && p1[5] == U_l && p1[6] == U_t)
            return TK_default;
        break;
    case U_e:
        if (p1[1] == U_x && p1[2] == U_t && p1[3] == U_e &&
            p1[4] == U_n && p1[5] == U_d && p1[6] == U_s)
            return TK_extends;
        break;
    case U_f:
        if (p1[1] == U_i && p1[2] == U_n && p1[3] == U_a &&
            p1[4] == U_l && p1[5] == U_l && p1[6] == U_y)
            return TK_finally;
        break;
    case U_p:
        if (p1[1] == U_a && p1[2] == U_c && p1[3] == U_k &&
            p1[4] == U_a && p1[5] == U_g && p1[6] == U_e)
            return TK_package;
        if (p1[1] == U_r && p1[2] == U_i && p1[3] == U_v &&
            p1[4] == U_a && p1[5] == U_t && p1[6] == U_e)
            return TK_private;
        break;
    }
    return TK_Identifier;
}

int Scanner::ScanKeyword8(const wchar_t* p1)
{
    switch (*p1)
    {
    case U_a:
        if (p1[1] == U_b && p1[2] == U_s &&
            p1[3] == U_t && p1[4] == U_r &&
            p1[5] == U_a && p1[6] == U_c && p1[7] == U_t)
            return TK_abstract;
        break;
    case U_c:
        if (p1[1] == U_o && p1[2] == U_n &&
            p1[3] == U_t && p1[4] == U_i &&
            p1[5] == U_n && p1[6] == U_u && p1[7] == U_e)
            return TK_continue;
        break;
    case U_s:
        if (p1[1] == U_t && p1[2] == U_r &&
            p1[3] == U_i && p1[4] == U_c &&
            p1[5] == U_t && p1[6] == U_f && p1[7] == U_p)
            return TK_strictfp;
        break;
    case U_v:
        if (p1[1] == U_o && p1[2] == U_l &&
            p1[3] == U_a && p1[4] == U_t &&
            p1[5] == U_i && p1[6] == U_l && p1[7] == U_e)
            return TK_volatile;
        break;
    }
    return TK_Identifier;
}

int Scanner::ScanKeyword9(const wchar_t* p1)
{
    if (p1[0] == U_i && p1[1] == U_n && p1[2] == U_t &&
        p1[3] == U_e && p1[4] == U_r && p1[5] == U_f &&
        p1[6] == U_a && p1[7] == U_c && p1[8] == U_e)
        return TK_interface;
    if (p1[0] == U_p && p1[1] == U_r && p1[2] == U_o &&
        p1[3] == U_t && p1[4] == U_e && p1[5] == U_c &&
        p1[6] == U_t && p1[7] == U_e && p1[8] == U_d)
        return TK_protected;
    if (p1[0] == U_t && p1[1] == U_r && p1[2] == U_a &&
        p1[3] == U_n && p1[4] == U_s && p1[5] == U_i &&
        p1[6] == U_e && p1[7] == U_n && p1[8] == U_t)
        return TK_transient;
    return TK_Identifier;
}

int Scanner::ScanKeyword10(const wchar_t* p1)
{
    if (p1[0] == U_i)
    {
        if (p1[1] == U_m && p1[2] == U_p && p1[3] == U_l &&
            p1[4] == U_e && p1[5] == U_m && p1[6] == U_e &&
            p1[7] == U_n && p1[8] == U_t && p1[9] == U_s)
            return TK_implements;
        if (p1[1] == U_n && p1[2] == U_s && p1[3] == U_t &&
            p1[4] == U_a && p1[5] == U_n && p1[6] == U_c &&
            p1[7] == U_e && p1[8] == U_o && p1[9] == U_f)
            return TK_instanceof;
    }
    return TK_Identifier;
}

int Scanner::ScanKeyword12(const wchar_t* p1)
{
    if (p1[0] == U_s && p1[1] == U_y && p1[2] == U_n &&
        p1[3] == U_c && p1[4] == U_h && p1[5] == U_r &&
        p1[6] == U_o && p1[7] == U_n && p1[8] == U_i &&
        p1[9] == U_z && p1[10] == U_e&& p1[11] == U_d)
        return TK_synchronized;
    return TK_Identifier;
}


//
// This procedure is invoked to scan a character literal. After the character
// literal has been scanned and classified, it is entered in the table with
// quotes intact.
//
void Scanner::ClassifyCharLiteral()
{
    //
    // We exploit the fact that the stream was doctored to end in
    // U_CARRIAGE_RETURN, U_NULL; and that all internal CR were changed to LF.
    //
    current_token -> SetKind(TK_CharacterLiteral);
    bool bad = false;
    const wchar_t* ptr = cursor + 1;
    switch (*ptr)
    {
    case U_SINGLE_QUOTE:
        bad = true;
        if (ptr[1] == U_SINGLE_QUOTE)
        {
            lex -> ReportMessage(StreamError::ESCAPE_EXPECTED,
                                 current_token -> Location() + 1,
                                 current_token -> Location() + 1);
        }
        else
        {
            lex -> ReportMessage(StreamError::EMPTY_CHARACTER_CONSTANT,
                                 current_token -> Location(),
                                 current_token -> Location() + 1);
            ptr--;
        }
        break;
    case U_BACKSLASH:
        switch (*++ptr)
        {
        case U_b:
        case U_f:
        case U_n:
        case U_r:
        case U_t:
        case U_DOUBLE_QUOTE:
        case U_BACKSLASH:
            break;
        case U_SINGLE_QUOTE:
            //
            // The user may have forgotten to do '\\'.
            //
            if (ptr[1] != U_SINGLE_QUOTE)
            {
                lex -> ReportMessage(StreamError::ESCAPE_EXPECTED,
                                     current_token -> Location() + 1,
                                     current_token -> Location() + 1);
                ptr--;
                bad = true;
            }
            break;
        case U_0:
        case U_1:
        case U_2:
        case U_3:
            if (! Code::IsOctalDigit(ptr[1]))
                break;
            ptr++;
            // fallthrough
        case U_4:
        case U_5:
        case U_6:
        case U_7:
            if (! Code::IsOctalDigit(ptr[1]))
                break;
            ptr++;
            break;
        case U_CARRIAGE_RETURN:
        case U_LINE_FEED:
            ptr--;
            // fallthrough
        case U_u:
            //
            // By now, Unicode escapes have already been flattened; and it is
            // illegal to try it twice (such as '\u005cu0000').
            //
        default:
            lex -> ReportMessage(StreamError::INVALID_ESCAPE_SEQUENCE,
                                 current_token -> Location() + 1,
                                 current_token -> Location() + ptr - cursor);
            bad = true;
        }
        break;
    case U_CARRIAGE_RETURN:
    case U_LINE_FEED:
        // Since the source is broken into lines before tokens (JLS 3.2), this
        // is an unterminated quote. We complain after this switch.
        ptr--;
        break;
    default:
        break;
    }

    if (*++ptr != U_SINGLE_QUOTE)
    {
        //
        // For generally better parsing and nicer error messages, see if the
        // user tried to do a multiple character alpha-numeric string.
        //
        while (Code::IsAlnum(ptr))
            ptr += Code::Codelength(ptr);
        if (Code::IsNewline(*ptr))
            ptr--;
        if (! bad)
        {
            lex -> ReportMessage((*ptr != U_SINGLE_QUOTE || ptr == cursor
                                  ? StreamError::UNTERMINATED_CHARACTER_CONSTANT
                                  : StreamError::MULTI_CHARACTER_CONSTANT),
                                 current_token -> Location(),
                                 ptr - lex -> InputBuffer());
        }
    }

    ptr++;
    current_token ->
        SetSymbol(control.char_table.FindOrInsertLiteral(cursor,
                                                         ptr - cursor));
    cursor = ptr;
}


//
// This procedure is invoked to scan a string literal. After the string
// literal has been scanned and classified, it is entered in the table with
// quotes intact.
//
void Scanner::ClassifyStringLiteral()
{
    //
    // We exploit the fact that the stream was doctored to end in
    // U_CARRIAGE_RETURN, U_NULL; and that all internal CR were changed to LF.
    //
    current_token -> SetKind(TK_StringLiteral);

    const wchar_t* ptr = cursor + 1;

    while (*ptr != U_DOUBLE_QUOTE && ! Code::IsNewline(*ptr))
    {
        if (*ptr++ == U_BACKSLASH)
        {
            switch (*ptr++)
            {
            case U_b:
            case U_f:
            case U_n:
            case U_r:
            case U_t:
            case U_SINGLE_QUOTE:
            case U_DOUBLE_QUOTE:
            case U_BACKSLASH:
            case U_0:
            case U_1:
            case U_2:
            case U_3:
            case U_4:
            case U_5:
            case U_6:
            case U_7:
                break;
            case U_u:
                //
                // By now, Unicode escapes have already been flattened; and it
                // is illegal to try it twice (such as "\u005cu0000").
                //
            default:
                ptr--;
                lex -> ReportMessage(StreamError::INVALID_ESCAPE_SEQUENCE,
                                     ptr - lex -> InputBuffer() - 1,
                                     (ptr - lex -> InputBuffer() -
                                      (Code::IsNewline(*ptr) ? 1 : 0)));
            }
        }
    }

    if (Code::IsNewline(*ptr))
    {
        ptr--;
        lex -> ReportMessage(StreamError::UNTERMINATED_STRING_CONSTANT,
                             current_token -> Location(),
                             ptr - lex -> InputBuffer());
    }

    ptr++;
    current_token ->
        SetSymbol(control.string_table.FindOrInsertLiteral(cursor,
                                                           ptr - cursor));
    cursor = ptr;
}


//
// This procedure is invoked when CURSOR points to a letter which starts a
// keyword. It scans the identifier and checks whether or not it is a keyword.
// Note that the use of that check is a time-optimization that is not
// required for correctness.
//
void Scanner::ClassifyIdOrKeyword()
{
    const wchar_t* ptr = cursor + 1;
    bool has_dollar = false;

    while (Code::IsAlnum(ptr))
    {
        has_dollar = has_dollar || (*ptr == U_DS);
        ptr += Code::Codelength(ptr);
    }
    int len = ptr - cursor;

    current_token -> SetKind(len < 13 ? (scan_keyword[len])(cursor)
                             : TK_Identifier);

    if (current_token -> Kind() == TK_assert &&
        control.option.source < JopaOption::SDK1_4)
    {
        lex -> ReportMessage(StreamError::DEPRECATED_IDENTIFIER_ASSERT,
                             current_token -> Location(),
                             current_token -> Location() + len - 1);
        current_token -> SetKind(TK_Identifier);
    }
    if (current_token -> Kind() == TK_enum &&
        control.option.source < JopaOption::SDK1_5)
    {
        lex -> ReportMessage(StreamError::DEPRECATED_IDENTIFIER_ENUM,
                             current_token -> Location(),
                             current_token -> Location() + len - 1);
        current_token -> SetKind(TK_Identifier);
    }
    // Java 8: Handle 'default' as a method modifier when not followed by ':'
    // For switch labels, 'default:' keeps TK_default
    // For default interface methods, 'default void foo()' becomes TK_abstract
    // (which is a valid modifier) and we track it for semantic analysis
    if (current_token -> Kind() == TK_default &&
        control.option.source >= JopaOption::SDK1_8)
    {
        // Skip whitespace to find next non-space character
        const wchar_t* lookahead = ptr;
        while (Code::IsSpace(*lookahead))
            lookahead++;

        // If not followed by ':', this is likely a default method modifier
        if (*lookahead != U_COLON)
        {
            // Transform to TK_abstract (a valid modifier) and track it
            current_token -> SetKind(TK_abstract);
            lex -> default_method_tokens.Next() = current_token_index;
        }
    }
    if (has_dollar && ! dollar_warning_given)
    {
        dollar_warning_given = true;
        lex -> ReportMessage(StreamError::DOLLAR_IN_IDENTIFIER,
                             current_token -> Location(),
                             current_token -> Location() + len - 1);
    }

    if (current_token -> Kind() == TK_Identifier)
    {
        current_token -> SetSymbol(control.FindOrInsertName(cursor, len));
        for (unsigned i = 0; i < control.option.keyword_map.Length(); i++)
        {
            if (control.option.keyword_map[i].length == len &&
                wcsncmp(cursor, control.option.keyword_map[i].name, len) == 0)
            {
                current_token -> SetKind(control.option.keyword_map[i].key);
            }
        }
    }
    else if (current_token -> Kind() == TK_class ||
             current_token -> Kind() == TK_enum ||
             current_token -> Kind() == TK_interface)
    {
        //
        // If this is a top-level type keyword (not in braces), we keep track
        // of it by adding it to a list.
        //
        if (brace_stack.Size() == 0)
            lex -> type_index.Next() = current_token_index;
    }
    else if (current_token -> Kind() == TK_package && ! lex -> package)
        lex -> package = current_token_index;
    cursor = ptr;
}

//
// This procedure is invoked when CURSOR points to an identifier start
// which cannot start a keyword.
//
void Scanner::ClassifyId()
{
    const wchar_t* ptr = cursor;
    bool has_dollar = false;

    while (Code::IsAlnum(ptr))
    {
        has_dollar = has_dollar || (*ptr == U_DS);
        ptr += Code::Codelength(ptr);
    }

    int len = ptr - cursor;

    if (has_dollar && ! dollar_warning_given)
    {
        dollar_warning_given = true;
        lex -> ReportMessage(StreamError::DOLLAR_IN_IDENTIFIER,
                             current_token -> Location(),
                             current_token -> Location() + len - 1);
    }

    current_token -> SetKind(TK_Identifier);
    current_token -> SetSymbol(control.FindOrInsertName(cursor, len));

    for (unsigned i = 0; i < control.option.keyword_map.Length(); i++)
    {
        if (control.option.keyword_map[i].length == len &&
            wcsncmp(cursor, control.option.keyword_map[i].name, len) == 0)
        {
            current_token -> SetKind(control.option.keyword_map[i].key);
        }
    }
    cursor = ptr;
}


//
// This procedure is invoked when CURSOR points directly to '0' - '9' or '.'.
// Such a token is classified as a numeric literal: TK_LongLiteral,
// TK_IntegerLiteral, TK_DoubleLiteral, or TK_FloatLiteral.
//
void Scanner::ClassifyNumericLiteral()
{
    //
    // Scan the initial sequence of digits, if any.
    // Java 7: Allow underscores in numeric literals
    //
    const wchar_t* ptr = cursor - 1;
    const wchar_t* tmp;
    while (Code::IsDecimalDigit(*++ptr) || *ptr == U_UNDERSCORE)
    {
        if (*ptr == U_UNDERSCORE)
        {
            // Skip underscore, validation happens later
        }
    }

    //
    // We now take an initial crack at classifying the numeric token.
    // We have three initial cases to consider, and stop parsing before any
    // exponent or type suffix:
    //
    // 1) If the initial (perhaps empty) sequence of digits is followed by
    //    '.', we have a floating-point constant. We scan the sequence of
    //    digits (if any) that follows the period. When '.' starts the number,
    //    we already checked that a digit follows before calling this method.
    // 2) If the initial sequence is "0x" or "0X", we have a hexadecimal
    //    literal, either integer or floating point.  To be floating point,
    //    the literal must contain an exponent with 'p' or 'P'; otherwise we
    //    parse the largest int literal.  There must be at least one hex
    //    digit after the prefix, and before the (possible) exponent.
    // 2) Otherwise, we have an integer literal. If the initial (non-empty)
    //    sequence of digits start with "0", we have an octal constant, and
    //    for nicer parsing, we simply complain about non-octal digits rather
    //    than strictly breaking 019 into the two tokens 01 and 9 (because
    //    it would be a guaranteed syntax error later on). However, it is
    //    still possible that 019 starts a valid floating point literal, which
    //    is checked later.
    //
    if (*ptr == U_DOT)
    {
        current_token -> SetKind(TK_DoubleLiteral);
        while (Code::IsDecimalDigit(*++ptr) || *ptr == U_UNDERSCORE);
    }
    else
    {
        current_token -> SetKind(TK_IntegerLiteral);
        if (*cursor == U_0)
        {
            if (*ptr == U_b || *ptr == U_B)
            {
                // Binary literal (Java 7)
                while (Code::IsBinaryDigit(*++ptr) || *ptr == U_UNDERSCORE)
                {
                    if (*ptr == U_UNDERSCORE)
                    {
                        // Skip underscore but check for consecutive/trailing underscores
                        const wchar_t* next = ptr + 1;
                        if (*next == U_UNDERSCORE ||
                            (!Code::IsBinaryDigit(*next) && *next != U_l && *next != U_L))
                        {
                            // Let the error be caught later if needed
                        }
                    }
                }
                // Skip the 'b'.
                if (ptr == cursor + 2) // Found a runt "0b".
                {
                    tmp = (*ptr == U_l || *ptr == U_L) ? ptr : ptr - 1;
                    lex -> ReportMessage(StreamError::INVALID_HEX_CONSTANT,
                                         current_token -> Location(),
                                         tmp - lex -> InputBuffer());
                }
            }
            else if (*ptr == U_x || *ptr == U_X)
            {
                // Don't use isxdigit, it's not platform independent.
                while (Code::IsHexDigit(*++ptr) || *ptr == U_UNDERSCORE)
                {
                    if (*ptr == U_UNDERSCORE)
                    {
                        // Skip underscore
                        const wchar_t* next = ptr + 1;
                        if (*next == U_UNDERSCORE ||
                            (!Code::IsHexDigit(*next) && *next != U_DOT &&
                             *next != U_p && *next != U_P &&
                             *next != U_l && *next != U_L))
                        {
                            // Let the error be caught later if needed
                        }
                    }
                }
                // Skip the 'x'.
                if (*ptr == U_DOT)
                {
                    current_token -> SetKind(TK_DoubleLiteral);
                    while (Code::IsHexDigit(*++ptr) || *ptr == U_UNDERSCORE);
                    if (*ptr != U_p && *ptr != U_P)
                    {
                        // Missing required 'p' exponent.
                        lex -> ReportMessage(StreamError::INVALID_FLOATING_HEX_EXPONENT,
                                             current_token -> Location(),
                                             ptr - 1 - lex -> InputBuffer());
                    }
                    else if (ptr == cursor + 3)
                    {
                        // Missing hex digits before exponent, with '.'.
                        tmp = ptr;
                        if (Code::IsSign(*++tmp)) // Skip the exponent letter.
                            tmp++; // Skip the '+' or '-'.
                        if (Code::IsHexDigit(*tmp))
                            while (Code::IsHexDigit(*++tmp) || *tmp == U_UNDERSCORE);
                        if (*tmp != U_d && *tmp != U_D &&
                            *tmp != U_f && *tmp != U_F)
                        {
                            tmp--;
                        }
                        lex -> ReportMessage(StreamError::INVALID_FLOATING_HEX_MANTISSA,
                                             current_token -> Location(),
                                             tmp - lex -> InputBuffer());
                    }
                }
                else if (ptr == cursor + 2) // Found a runt "0x".
                {
                    if (*ptr == U_p || *ptr == U_P)
                    {
                        // Missing hex digits before exponent, without '.'.
                        tmp = ptr;
                        if (Code::IsSign(*++tmp)) // Skip the exponent letter.
                            tmp++; // Skip the '+' or '-'.
                        if (Code::IsHexDigit(*tmp))
                            while (Code::IsHexDigit(*++tmp) || *tmp == U_UNDERSCORE);
                        if (*tmp != U_d && *tmp != U_D &&
                            *tmp != U_f && *tmp != U_F)
                        {
                            tmp--;
                        }
                        lex -> ReportMessage(StreamError::INVALID_FLOATING_HEX_MANTISSA,
                                             current_token -> Location(),
                                             tmp - lex -> InputBuffer());
                    }
                    else
                    {
                        tmp = (*ptr == U_l || *ptr == U_L) ? ptr : ptr - 1;
                        lex -> ReportMessage(StreamError::INVALID_HEX_CONSTANT,
                                             current_token -> Location(),
                                             tmp - lex -> InputBuffer());
                    }
                }
            }
            // Octal prefix. See if it will become floating point later.
            else if (*ptr != U_e && *ptr != U_E &&
                     *ptr != U_d && *ptr != U_D &&
                     *ptr != U_f && *ptr != U_F)
            {
                tmp = cursor;
                while (Code::IsOctalDigit(*++tmp) || *tmp == U_UNDERSCORE); // Skip leading '0'.
                if (tmp != ptr)
                {
                    tmp = (*ptr == U_l || *ptr == U_L) ? ptr : ptr - 1;
                    lex -> ReportMessage(StreamError::INVALID_OCTAL_CONSTANT,
                                         current_token -> Location(),
                                         tmp - lex -> InputBuffer());
                }
            }
        }
    }

    //
    // If the initial numeric token is followed by an exponent, then it is a
    // floating-point constant. If that's the case, the literal is
    // reclassified and the exponent is scanned. Note that as 'E' and 'e' are
    // legitimate hexadecimal digits, we don't have to worry about a
    // hexadecimal constant being used as the prefix of a floating-point
    // constant. A hex floating point requires a hex prefix. An exponent
    // overrides an octal literal, as do the float and double suffixes. We
    // stop parsing before any type suffix.
    //
    // For example, 0x123e12 is tokenized as a single hexadecimal digit, while
    // the string 0x123e+12 gets broken down as the hex number 0x123e, the
    // operator '+', and the decimal constant 12. Meanwhile, 019e+0 and 019d
    // are both tokenized as a single floating-point constant 19.0. Note that
    // 1e should strictly be parsed as the int 1 followed by identifier e;
    // 1e+ should be the int 1, identifier e, and operator +; and 1p0d should
    // be the int 1 and identifier p0d; however all these cases are guaranteed
    // to be syntax errors later on, so we nicely consume them as a single
    // invalid floating point token now.
    //
    if (*ptr == U_e || *ptr == U_E || *ptr == U_p || *ptr == U_P)
    {
        current_token -> SetKind(TK_DoubleLiteral);
        if ((*ptr == U_p || *ptr == U_P) &&
            ! (cursor[1] == U_x || cursor[1] == U_X))
        {
            tmp = ptr;
            if (Code::IsSign(*++tmp)) // Skip the exponent letter.
                tmp++; // Skip the '+' or '-'.
            if (Code::IsDecimalDigit(*tmp))
                while (Code::IsDecimalDigit(*++tmp) || *tmp == U_UNDERSCORE);
            if (*tmp != U_d && *tmp != U_D && *tmp != U_f && *tmp != U_F)
                tmp--;
            lex -> ReportMessage(StreamError::INVALID_FLOATING_HEX_PREFIX,
                                 current_token -> Location(),
                                 tmp - lex -> InputBuffer());
        }
        if (Code::IsSign(*++ptr)) // Skip the exponent letter.
            ptr++; // Skip the '+' or '-'.
        if (Code::IsDecimalDigit(*ptr))
            while (Code::IsDecimalDigit(*++ptr) || *ptr == U_UNDERSCORE);
        else
        {
            tmp = (*ptr == U_d || *ptr == U_D || *ptr == U_f || *ptr == U_F)
                ? ptr : ptr - 1;
            lex -> ReportMessage(StreamError::INVALID_FLOATING_EXPONENT,
                                 current_token -> Location(),
                                 tmp - lex -> InputBuffer());
        }
    }

    //
    // A numeric constant may be suffixed by a letter that further qualifies
    // what kind of a constant it is. We check for these suffixes here.
    //
    int len;
    if (*ptr == U_f || *ptr == U_F)
    {
        len = ++ptr - cursor;
        current_token ->
            SetSymbol(control.float_table.FindOrInsertLiteral(cursor, len));
        current_token -> SetKind(TK_FloatLiteral);
    }
    else if (*ptr == U_d || *ptr == U_D)
    {
        len = ++ptr - cursor;
        current_token ->
            SetSymbol(control.double_table.FindOrInsertLiteral(cursor, len));
        current_token -> SetKind(TK_DoubleLiteral);
    }
    else if (current_token -> Kind() == TK_IntegerLiteral)
    {
        if (*ptr == U_l || *ptr == U_L)
        {
            if (*ptr == U_l && control.option.pedantic)
            {
                lex -> ReportMessage(StreamError::FAVOR_CAPITAL_L_SUFFIX,
                                     current_token -> Location(),
                                     ptr - lex -> InputBuffer());
            }
            
            len = ++ptr - cursor;
            current_token ->
                SetSymbol(control.long_table.FindOrInsertLiteral(cursor, len));
            current_token -> SetKind(TK_LongLiteral);
        }
        else
        {
            len = ptr - cursor;
            current_token ->
                SetSymbol(control.int_table.FindOrInsertLiteral(cursor, len));
        }
    }
    else
    {
        assert(current_token -> Kind() == TK_DoubleLiteral);
        len = ptr - cursor;
        current_token ->
            SetSymbol(control.double_table.FindOrInsertLiteral(cursor, len));
    }
    cursor = ptr;
}


void Scanner::ClassifyColon()
{
    cursor++;
    if (*cursor == U_COLON &&
        control.option.source >= JopaOption::SDK1_8)
    {
        // Java 8: Method reference ::
        cursor++;
        current_token -> SetKind(TK_COLON_COLON);
    }
    else
    {
        current_token -> SetKind(TK_COLON);
    }
}


void Scanner::ClassifyPlus()
{
    cursor++;
    if (*cursor == U_PLUS)
    {
        cursor++;
        current_token -> SetKind(TK_PLUS_PLUS);
    }
    else if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_PLUS_EQUAL);
    }
    else current_token -> SetKind(TK_PLUS);
}


void Scanner::ClassifyMinus()
{
    cursor++;
    if (*cursor == U_MINUS)
    {
        cursor++;
        current_token -> SetKind(TK_MINUS_MINUS);
    }
    else if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_MINUS_EQUAL);
    }
    else if (*cursor == U_GREATER &&
             control.option.source >= JopaOption::SDK1_8)
    {
        // Java 8: Lambda arrow ->
        cursor++;
        current_token -> SetKind(TK_ARROW);
    }
    else current_token -> SetKind(TK_MINUS);
}


void Scanner::ClassifyStar()
{
    cursor++;
    if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_MULTIPLY_EQUAL);
    }
    else current_token -> SetKind(TK_MULTIPLY);
}


void Scanner::ClassifySlash()
{
    cursor++;
    if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_DIVIDE_EQUAL);
    }
    else if (*cursor == U_SLASH)
        ScanSlashComment();
    else if (*cursor == U_STAR)
        ScanStarComment();
    else current_token -> SetKind(TK_DIVIDE);
}


void Scanner::ClassifyLess()
{
    cursor++;
    if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_LESS_EQUAL);
    }
    else if (*cursor == U_LESS)
    {
        cursor++;
        if (*cursor == U_EQUAL)
        {
            cursor++;
            current_token -> SetKind(TK_LEFT_SHIFT_EQUAL);
        }
        else current_token -> SetKind(TK_LEFT_SHIFT);
    }
    else if (*cursor == U_GREATER &&
             control.option.source >= JopaOption::SDK1_7)
    {
        // Diamond operator <> for Java 7+
        // We emit < ? > so the parser accepts it as type arguments.
        // The synthetic '?' is marked as a diamond token for semantic analysis.

        // Set current token to TK_LESS
        current_token -> SetKind(TK_LESS);

        // Insert a synthetic TK_QUESTION token
        // Use the same location as the '<' for the synthetic '?'
        TokenIndex synth_token_index =
            lex -> GetNextToken(cursor - 1 - lex -> InputBuffer());
        LexStream::Token& synth_token = lex -> token_stream[synth_token_index];
        synth_token.SetKind(TK_QUESTION);

        // Mark this token as a diamond (synthetic) token
        lex -> diamond_tokens.Next() = synth_token_index;

        // cursor stays at U_GREATER, next iteration will emit TK_GREATER
    }
    else current_token -> SetKind(TK_LESS);
}


void Scanner::ClassifyGreater()
{
    cursor++;
    current_token -> SetKind(TK_GREATER);
    if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_GREATER_EQUAL);
    }
    else if (*cursor == U_GREATER)
    {
        cursor++;
        if (*cursor == U_EQUAL)
        {
            cursor++;
            current_token -> SetKind(TK_RIGHT_SHIFT_EQUAL);
        }
        else if (*cursor == U_GREATER)
        {
            cursor++;
            if (*cursor == U_EQUAL)
            {
                cursor++;
                current_token -> SetKind(TK_UNSIGNED_RIGHT_SHIFT_EQUAL);
            }
            else current_token -> SetKind(TK_UNSIGNED_RIGHT_SHIFT);
        }
        else current_token -> SetKind(TK_RIGHT_SHIFT);
    }
}


void Scanner::ClassifyAnd()
{
    cursor++;
    if (*cursor == U_AMPERSAND)
    {
        cursor++;
        current_token -> SetKind(TK_AND_AND);
    }
    else if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_AND_EQUAL);
    }
    else current_token -> SetKind(TK_AND);
}


void Scanner::ClassifyOr()
{
    cursor++;
    if (*cursor == U_BAR)
    {
        cursor++;
        current_token -> SetKind(TK_OR_OR);
    }
    else if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_OR_EQUAL);
    }
    else current_token -> SetKind(TK_OR);
}


void Scanner::ClassifyXor()
{
    cursor++;
    if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_XOR_EQUAL);
    }
    else current_token -> SetKind(TK_XOR);
}


void Scanner::ClassifyNot()
{
    cursor++;
    if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_NOT_EQUAL);
    }
    else current_token -> SetKind(TK_NOT);
}


void Scanner::ClassifyEqual()
{
    cursor++;
    if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_EQUAL_EQUAL);
    }
    else current_token -> SetKind(TK_EQUAL);
}


void Scanner::ClassifyMod()
{
    cursor++;
    if (*cursor == U_EQUAL)
    {
        cursor++;
        current_token -> SetKind(TK_REMAINDER_EQUAL);
    }
    else current_token -> SetKind(TK_REMAINDER);
}


void Scanner::ClassifyPeriod()
{
    if (Code::IsDecimalDigit(cursor[1])) // Is '.' followed by digit?
        ClassifyNumericLiteral();
    else if (cursor[1] == U_DOT && cursor[2] == U_DOT)
    {
        // Added for Java 1.5, varargs, by JSR 201.
        current_token -> SetKind(TK_ELLIPSIS);
        cursor += 3;
    }
    else
    {
        current_token -> SetKind(TK_DOT);
        cursor++;
    }
}


void Scanner::ClassifySemicolon()
{
    current_token -> SetKind(TK_SEMICOLON);
    cursor++;
}


void Scanner::ClassifyComma()
{
    current_token -> SetKind(TK_COMMA);
    cursor++;
}


void Scanner::ClassifyLbrace()
{
    //
    // Instead of setting the symbol for a left brace, we keep track of it.
    // When we encounter its matching right brace, we use the symbol field
    // to identify its counterpart.
    //
    brace_stack.Push(current_token_index);
    current_token -> SetKind(TK_LBRACE);
    cursor++;
}


void Scanner::ClassifyRbrace()
{
    //
    // When a left brace in encountered, it is pushed into the brace_stack.
    // When its matching right brace in encountered, we pop the left brace
    // and make it point to its matching right brace.
    //
    TokenIndex left_brace = brace_stack.Top();
    if (left_brace) // This right brace is matched by a left one
    {
        lex -> token_stream[left_brace].SetRightBrace(current_token_index);
        brace_stack.Pop();
    }
    current_token -> SetKind(TK_RBRACE);
    cursor++;
}


void Scanner::ClassifyLparen()
{
    current_token -> SetKind(TK_LPAREN);
    cursor++;
}


void Scanner::ClassifyRparen()
{
    current_token -> SetKind(TK_RPAREN);
    cursor++;
}


void Scanner::ClassifyLbracket()
{
    current_token -> SetKind(TK_LBRACKET);
    cursor++;
}


void Scanner::ClassifyRbracket()
{
    current_token -> SetKind(TK_RBRACKET);
    cursor++;
}


void Scanner::ClassifyComplement()
{
    current_token -> SetKind(TK_TWIDDLE);
    cursor++;
}


void Scanner::ClassifyAt()
{
    // Added for Java 1.5, attributes, by JSR 175.
    current_token -> SetKind(TK_AT);
    cursor++;
}


void Scanner::ClassifyQuestion()
{
    current_token -> SetKind(TK_QUESTION);
    cursor++;
}


void Scanner::ClassifyNonAsciiUnicode()
{
    if (Code::IsAlpha(cursor)) // Some kind of non-ascii unicode letter
        ClassifyId();
    else ClassifyBadToken();
}


//
// Anything that doesn't fit above. Note that the lex stream already stripped
// any concluding ctrl-z, so we don't need to worry about seeing that as a
// bad token. For fewer error messages, we scan until the next valid
// character, issue the error message, then treat this token as whitespace.
//
void Scanner::ClassifyBadToken()
{
    while (++cursor < input_buffer_tail)
    {
        if ((*cursor < 128 &&
             classify_token[*cursor] != &Scanner::ClassifyBadToken) ||
            Code::IsAlpha(cursor))
        {
            break;
        }
    }
    current_token -> SetKind(0);
    lex -> ReportMessage(StreamError::BAD_TOKEN, current_token -> Location(),
                         cursor - lex -> InputBuffer() - 1);
}


} // Close namespace Jopa block
