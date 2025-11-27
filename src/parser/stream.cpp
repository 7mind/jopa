#include "stream.h"
#include "code.h"
#include "zipfile.h"
#include "symbol.h"
#include "control.h"
#include "semantic.h"
#include "grammar/javasym.h"
#include "option.h"
#include "tab.h"


namespace Jopa { // Open namespace Jopa block
// Class StreamError

JopaError::JopaErrorSeverity StreamError::getSeverity()
{
    // Most Lexical errors are ERRORs.
    return kind >= StreamError::LAST_CHARACTER_NOT_NEWLINE
        ? JopaError::JOPA_WARNING : JopaError::JOPA_ERROR;
}

const char* StreamError::getFileName()
{
    assert(lex_stream);
    return lex_stream -> FileName();
}

const wchar_t* StreamError::getErrorMessage()
{
    switch (kind)
    {
    case BAD_TOKEN:
        return L"Illegal token ignored.";
    case EMPTY_CHARACTER_CONSTANT:
        return L"Empty character constant.";
    case UNTERMINATED_CHARACTER_CONSTANT:
        return L"Character constant not properly terminated.";
    case MULTI_CHARACTER_CONSTANT:
        return L"Character constant must be only one character.";
    case ESCAPE_EXPECTED:
        return L"Escape sequence required for this character constant.";
    case UNTERMINATED_COMMENT:
        return L"Comment not properly terminated.";
    case UNTERMINATED_STRING_CONSTANT:
        return L"String constant not properly terminated.";
    case INVALID_HEX_CONSTANT:
        return L"The hexadecimal prefix '0x' must be followed by at least one "
            L"hex digit.";
    case INVALID_FLOATING_HEX_EXPONENT:
        return L"A hexadecimal floating point literal must have an exponent "
            L"'p' designator.";
    case INVALID_FLOATING_HEX_MANTISSA:
        return L"A hexadecimal floating point literal must have at least one "
            L"hex digit between the prefix '0x' and exponent 'p'.";
    case INVALID_FLOATING_HEX_PREFIX:
        return L"A hexadecimal floating point literal must start with the "
            L"prefix '0x'.";
    case INVALID_OCTAL_CONSTANT:
        return L"The octal prefix '0' must not be followed by '8' or '9'.";
    case INVALID_FLOATING_EXPONENT:
        return L"A floating point exponent must have at least one digit.";
    case INVALID_UNICODE_ESCAPE:
        return L"Invalid unicode escape character.";
    case INVALID_ESCAPE_SEQUENCE:
        return L"Invalid escape sequence.";
    case LAST_CHARACTER_NOT_NEWLINE:
        return L"While not necessary, it is a good idea to end a file with a "
            L"line terminator.";
    case DEPRECATED_IDENTIFIER_ASSERT:
        return L"The use of \"assert\" as an identifier is deprecated, "
            L"as it is now a keyword. Use -source 1.4 if you intended "
            L"to make use of assertions.";
    case DEPRECATED_IDENTIFIER_ENUM:
        return L"The use of \"enum\" as an identifier is deprecated, "
            L"as it is a keyword in -source 1.5 and later.";
    case DOLLAR_IN_IDENTIFIER:
        return L"The use of \"$\" in an identifier, while legal, is strongly "
            L"discouraged, since it can conflict with compiler-generated "
            L"names. If you are trying to access a nested type, use \".\" "
            L"instead of \"$\".";
    case FAVOR_CAPITAL_L_SUFFIX:
        return L"The L suffix is preferred over the l suffix because l "
            L"(lowercase L) is easily confused with 1 (the digit 1).";
    default:
        assert(false);
    }

    return L"Unknown Error";
}

bool StreamError::emacs_style_report = false;

const wchar_t* StreamError::getErrorReport()
{
    //
    // We need to use this lazy initialization, because we can't to it in
    // Initialize() method. Reason is that Find* methods are unusable until
    // LexStream::CompressSpace is called, which does not happen until later
    // after scanning is done and all errors are reported.
    //
    if (! initialized)
    {
        left_line_no = lex_stream -> FindLine(start_location);
        left_column_no = lex_stream -> FindColumn(start_location - 1) + 1;
        right_line_no = lex_stream -> FindLine(end_location);
        right_column_no = lex_stream -> FindColumn(end_location);
        initialized = true;
    }

    return emacs_style_report ? emacsErrorString() : regularErrorString();
}

const wchar_t* StreamError::emacsErrorString()
{
    ErrorString s;

    s << getFileName()
      << ':' << left_line_no  << ':' << left_column_no
      << ':' << right_line_no << ':' << right_column_no
      << ": Lexical " << getSeverityString() << ": " << getErrorMessage();

    return s.Array();
}


const wchar_t* StreamError::regularErrorString()
{
    ErrorString s;

    assert(lex_stream);
    lex_stream -> OutputSource(this, s);

    s << endl << "*** Lexical " << getSeverityString() << ": "
      << getErrorMessage();

    return s.Array();
}


void StreamError::Initialize(StreamErrorKind kind_, unsigned start,
                             unsigned end, LexStream* l)
{
    kind = kind_;
    start_location = start;
    end_location = end;
    lex_stream = l;
}

StreamError::StreamError() : initialized(false)
{
}


// Class Stream

Stream::Stream()
    : input_buffer(NULL),
      input_buffer_length(0)
#if defined(HAVE_LIBICU_UC)
    , _decoder(NULL)
#elif defined(JOPA_ICONV_ENCODING)
    , _decoder((iconv_t) - 1)
#endif
{
}

Stream::~Stream()
{
    DestroyInput();
#ifdef HAVE_ENCODING
    DestroyEncoding();
#endif // HAVE_ENCODING
}

#ifdef HAVE_ENCODING

// This method will return true is the given encoding
// can be supported, it is static because we need to
// be able to query encodings without an instance.

bool Stream::IsSupportedEncoding(char* encoding)
{
    // Create a tmp object instead of duplicating
    // the code in SetEncoding and DestroyEncoding
    Stream* tmp = new Stream();
    bool supported = tmp -> SetEncoding(encoding);
    delete tmp;
    return supported;
}

bool Stream::SetEncoding(char* encoding)
{
    assert(encoding);
    DestroyEncoding();

# if defined(HAVE_LIBICU_UC)
    UErrorCode err = U_ZERO_ERROR;
    _decoder = ucnv_open(encoding, &err);
# elif defined(JOPA_ICONV_ENCODING)
    _decoder = iconv_open(JOPA_ICONV_ENCODING, encoding);
# endif

    return HaveDecoder();
}

void Stream::DestroyEncoding()
{
    if (HaveDecoder())
    {
# if defined(HAVE_LIBICU_UC)
        ucnv_close(_decoder);
        _decoder = NULL;
# elif defined(JOPA_ICONV_ENCODING)
        iconv_close(_decoder);
        _decoder = (iconv_t)-1;
# endif
    }
}


// FIXME: We may want to inline this next method

// nah... I wanna get rid of this method instead.

wchar_t Stream::DecodeNextCharacter()
{
    const char* before = source_ptr;
    wchar_t next;
    error_decode_next_character = false;

# if defined(HAVE_LIBICU_UC)

    if (!HaveDecoder())
        return (wchar_t) *source_ptr++;

    UErrorCode err = U_ZERO_ERROR;
    next = ucnv_getNextUChar(_decoder, &source_ptr, source_tail + 1, &err);

    if (U_FAILURE(err))
    {
        fprintf(stderr,"Conversion error: %s at byte %d\n",
            u_errorName(err),
            int(before - data_buffer)
        );
        error_decode_next_character = true;
        return 0;
    }

# elif defined(JOPA_ICONV_ENCODING)

    if (!HaveDecoder()) {
        // you can't just cast a char to a wchar_t, since that would
        // sign extend the results, which if wchar_t is 4 bytes will
        // lead the parser to segfault because it calculates a table
        // offset based on the char.
        return (wchar_t) ((*source_ptr++) & 0x00FF);
    }

    wchar_t* chp = &next;
    size_t chl = sizeof(wchar_t);
    size_t srcl = 1;

 try_it_again:
    size_t n = iconv(_decoder,
#  ifdef HAVE_ERROR_CALL_ICONV_CONST
                     (char**)
#  endif // HAVE_ERROR_CALL_ICONV_CONST
                     &source_ptr, &srcl,
                     (char**) &chp, &chl);

    if (n == (size_t) -1)
    {
        if (errno == EINVAL && before + srcl + 1 <= source_tail) {
            srcl++; //we're on a multibyte input and it didn't fit in srcl
            goto try_it_again; //so we increase the window if there is space
            // and try again. This is the ultimate hack. I hate it.
        }
        else
        {
            fprintf(stderr,"Charset conversion error at offset %d: ",
                    (int) (before - data_buffer));
            perror("");
            error_decode_next_character = true;
            return 0;
        }
    }

#  if JOPA_ICONV_NEEDS_BYTE_SWAP
    char tmp;
    char* targ = (char*) &next;
#   if SIZEOF_WCHAR_T == 2
    tmp = targ[0];
    targ[0] = targ[1];
    targ[1] = tmp;
#   elif SIZEOF_WCHAR_T == 4
    tmp = targ[0];
    targ[0] = targ[3];
    targ[3] = tmp;
    tmp = targ[1];
    targ[1] = targ[2];
    targ[2] = tmp;
#   else
#    error sizeof(wchar_t) unworkable, this should not have passed configure
#   endif //sizeof(wchar_t)

#  endif // JOPA_ICONV_NEEDS_BYTE_SWAP

# endif // JOPA_ICONV_ENCODING

    if (before == source_ptr)
    {
        //End of conversion
        error_decode_next_character = true;
        return 0;
    }

    return next;
}

#endif // HAVE_ENCODING


// Class LexStream

LexStream::LexStream(Control& control_, FileSymbol* file_symbol_)
    : file_symbol(file_symbol_),
#ifdef JOPA_DEBUG
      file_read(false),
#endif
      index(0),
      tokens(NULL),
      token_stream(12, 16),
      comments(NULL),
      comment_stream(10, 8),
      locations(NULL),
      line_location(12, 8),
      package(0),
      initial_reading_of_input(true),
      comment_buffer(NULL),
      control(control_)
{
    StreamError::emacs_style_report = ! control_.option.errors;
}

LexStream::~LexStream()
{
#ifdef JOPA_DEBUG
    if (file_read)
        control.line_count += (line_location.Length() - 3);
#endif

    DestroyInput();
}


const wchar_t* LexStream::KeywordName(int kind)
{
    switch (kind)
    {
    case TK_abstract: return StringConstant::US_abstract;
    case TK_assert: return StringConstant::US_assert;
    case TK_boolean: return StringConstant::US_boolean;
    case TK_break: return StringConstant::US_break;
    case TK_byte: return StringConstant::US_byte;
    case TK_case: return StringConstant::US_case;
    case TK_catch: return StringConstant::US_catch;
    case TK_char: return StringConstant::US_char;
    case TK_class: return StringConstant::US_class;
    case TK_const: return StringConstant::US_const;
    case TK_continue: return StringConstant::US_continue;
    case TK_default: return StringConstant::US_default;
    case TK_do: return StringConstant::US_do;
    case TK_double: return StringConstant::US_double;
    case TK_else: return StringConstant::US_else;
    case TK_enum: return StringConstant::US_enum;
    case TK_extends: return StringConstant::US_extends;
    case TK_false: return StringConstant::US_false;
    case TK_final: return StringConstant::US_final;
    case TK_finally: return StringConstant::US_finally;
    case TK_float: return StringConstant::US_float;
    case TK_for: return StringConstant::US_for;
    case TK_goto: return StringConstant::US_goto;
    case TK_if: return StringConstant::US_if;
    case TK_implements: return StringConstant::US_implements;
    case TK_import: return StringConstant::US_import;
    case TK_instanceof: return StringConstant::US_instanceof;
    case TK_int: return StringConstant::US_int;
    case TK_interface: return StringConstant::US_interface;
    case TK_long: return StringConstant::US_long;
    case TK_native: return StringConstant::US_native;
    case TK_new: return StringConstant::US_new;
    case TK_null: return StringConstant::US_null;
    case TK_package: return StringConstant::US_package;
    case TK_private: return StringConstant::US_private;
    case TK_protected: return StringConstant::US_protected;
    case TK_public: return StringConstant::US_public;
    case TK_return: return StringConstant::US_return;
    case TK_short: return StringConstant::US_short;
    case TK_static: return StringConstant::US_static;
    case TK_strictfp: return StringConstant::US_strictfp;
    case TK_super: return StringConstant::US_super;
    case TK_switch: return StringConstant::US_switch;
    case TK_synchronized: return StringConstant::US_synchronized;
    case TK_this: return StringConstant::US_this;
    case TK_throw: return StringConstant::US_throw;
    case TK_throws: return StringConstant::US_throws;
    case TK_transient: return StringConstant::US_transient;
    case TK_true: return StringConstant::US_true;
    case TK_try: return StringConstant::US_try;
    case TK_void: return StringConstant::US_void;
    case TK_volatile: return StringConstant::US_volatile;
    case TK_while: return StringConstant::US_while;

    case TK_PLUS_PLUS: return StringConstant::US_PLUS_PLUS;
    case TK_MINUS_MINUS: return StringConstant::US_MINUS_MINUS;
    case TK_EQUAL_EQUAL: return StringConstant::US_EQUAL_EQUAL;
    case TK_LESS_EQUAL: return StringConstant::US_LESS_EQUAL;
    case TK_GREATER_EQUAL: return StringConstant::US_GREATER_EQUAL;
    case TK_NOT_EQUAL: return StringConstant::US_NOT_EQUAL;
    case TK_LEFT_SHIFT: return StringConstant::US_LEFT_SHIFT;
    case TK_RIGHT_SHIFT: return StringConstant::US_RIGHT_SHIFT;
    case TK_UNSIGNED_RIGHT_SHIFT:
        return StringConstant::US_UNSIGNED_RIGHT_SHIFT;
    case TK_PLUS_EQUAL: return StringConstant::US_PLUS_EQUAL;
    case TK_MINUS_EQUAL: return StringConstant::US_MINUS_EQUAL;
    case TK_MULTIPLY_EQUAL: return StringConstant::US_MULTIPLY_EQUAL;
    case TK_DIVIDE_EQUAL: return StringConstant::US_DIVIDE_EQUAL;
    case TK_AND_EQUAL: return StringConstant::US_AND_EQUAL;
    case TK_OR_EQUAL: return StringConstant::US_OR_EQUAL;
    case TK_XOR_EQUAL: return StringConstant::US_XOR_EQUAL;
    case TK_REMAINDER_EQUAL: return StringConstant::US_REMAINDER_EQUAL;
    case TK_LEFT_SHIFT_EQUAL: return StringConstant::US_LEFT_SHIFT_EQUAL;
    case TK_RIGHT_SHIFT_EQUAL:
        return StringConstant::US_RIGHT_SHIFT_EQUAL;
    case TK_UNSIGNED_RIGHT_SHIFT_EQUAL:
        return StringConstant::US_UNSIGNED_RIGHT_SHIFT_EQUAL;
    case TK_OR_OR: return StringConstant::US_OR_OR;
    case TK_AND_AND: return StringConstant::US_AND_AND;

    case TK_PLUS: return StringConstant::US_PLUS;
    case TK_MINUS: return StringConstant::US_MINUS;
    case TK_NOT: return StringConstant::US_NOT;
    case TK_REMAINDER: return StringConstant::US_REMAINDER;
    case TK_XOR: return StringConstant::US_XOR;
    case TK_AND: return StringConstant::US_AND;
    case TK_MULTIPLY: return StringConstant::US_MULTIPLY;
    case TK_OR: return StringConstant::US_OR;
    case TK_TWIDDLE: return StringConstant::US_TWIDDLE;
    case TK_DIVIDE: return StringConstant::US_DIVIDE;
    case TK_GREATER: return StringConstant::US_GREATER;
    case TK_LESS: return StringConstant::US_LESS;
    case TK_LPAREN: return StringConstant::US_LPAREN;
    case TK_RPAREN: return StringConstant::US_RPAREN;
    case TK_LBRACE: return StringConstant::US_LBRACE;
    case TK_RBRACE: return StringConstant::US_RBRACE;
    case TK_LBRACKET: return StringConstant::US_LBRACKET;
    case TK_RBRACKET: return StringConstant::US_RBRACKET;
    case TK_SEMICOLON: return StringConstant::US_SEMICOLON;
    case TK_QUESTION: return StringConstant::US_QUESTION;
    case TK_COLON: return StringConstant::US_COLON;
    case TK_COMMA: return StringConstant::US_COMMA;
    case TK_DOT: return StringConstant::US_DOT;
    case TK_ELLIPSIS: return StringConstant::US_DOT_DOT_DOT;
    case TK_AT: return StringConstant::US_AT;
    case TK_EQUAL: return StringConstant::US_EQUAL;
    case TK_EOF: return StringConstant::US_EOF;
    default: break;
    }
    return StringConstant::US_EMPTY;
}


unsigned LexStream::RightColumn(TokenIndex i)
{
    if (! input_buffer)
        return 0;
    unsigned location = tokens[i].Location() - 1 +
        (NameSymbol(i) || LiteralSymbol(i)
         ? tokens[i].additional_info.symbol -> NameLength()
         : wcslen(KeywordName(tokens[i].Kind())));
    return FindColumn(location);
}

const wchar_t* LexStream::NameString(TokenIndex i)
{
    return NameSymbol(i) || LiteralSymbol(i)
        ? tokens[i].additional_info.symbol -> Name()
        : KeywordName(tokens[i].Kind());
}

unsigned LexStream::NameStringLength(TokenIndex i)
{
    return NameSymbol(i) || LiteralSymbol(i)
        ? tokens[i].additional_info.symbol -> NameLength()
        : wcslen(KeywordName(tokens[i].Kind()));
}

unsigned LexStream::LineLength(unsigned line_no)
{
    assert(input_buffer && locations);
    return Tab::Wcslen(input_buffer, locations[line_no],
                       locations[line_no + 1] - 2); // ignore the \n
}

unsigned LexStream::LineSegmentLength(TokenIndex i)
{
    return Tab::Wcslen(input_buffer, tokens[i].Location(),
                       LineEnd(Line(i)));
}

//
// If the token represents a literal, this returns the literal symbol
// associated with it.
//
class LiteralSymbol* LexStream::LiteralSymbol(TokenIndex i)
{
    assert(i < (unsigned) token_stream.Length());
    Symbol* symbol = tokens[i].additional_info.symbol;
    return (symbol && Kind(i) != TK_LBRACE)
        ? symbol -> LiteralCast() : (class LiteralSymbol*) NULL;
}


//
// If the token represents a literal, this returns the name symbol
// associated with it.
//
class NameSymbol* LexStream::NameSymbol(TokenIndex i)
{
    assert(i < (unsigned) token_stream.Length());
    Symbol* symbol = tokens[i].additional_info.symbol;
    return (symbol && Kind(i) != TK_LBRACE)
        ? symbol -> NameCast() : (class NameSymbol*) NULL;
}


//
// Name of input file where the token appeared.
//
char* LexStream::FileName() { return file_symbol -> FileName(); }
unsigned LexStream::FileNameLength()
{
    return file_symbol -> FileNameLength();
}


//
//
//
void LexStream::CompressSpace()
{
    tokens = token_stream.Array();
    comments = comment_stream.Array();
    locations = line_location.Array();
    types = type_index.Array();
}


//
// Outputs a line of source code, flattening literal TABs into spaces for
// uniform output spacing.
//
void LexStream::OutputLine(unsigned line_no, ErrorString& s)
{
    assert(line_no);
    unsigned line_end = LineEnd(line_no);
    bool expand = Coutput.ExpandWchar();
    for (unsigned i = LineStart(line_no), offset = 0; i <= line_end;
         i++, offset++)
    {
        wchar_t ch = input_buffer[i];
        if (ch == U_CARRIAGE_RETURN || ch == U_LINE_FEED)
            s << (wchar_t) U_LINE_FEED;
        else if (ch == U_HORIZONTAL_TAB)
        {
            s.width(Tab::TabSize() - offset % Tab::TabSize());
            s << (wchar_t) U_SPACE;
            offset = Tab::TabSize() - 1;
        }
        else if (ch == U_NULL)
        {
            s << (expand ? "\\u0000" : "?");
        }
        else
        {
            if (expand && (ch < U_SPACE || ch >= 0x0ff))
                offset += 5;
            s << ch;
        }
    }
}


//
// Outputs the section of source code which is in error.
//
void LexStream::OutputSource(JopaError* err, ErrorString& s)
{
    int left_line_no = err -> getLeftLineNo();
    int left_column_no = err -> getLeftColumnNo();
    int right_line_no = err -> getRightLineNo();
    int right_column_no = err -> getRightColumnNo();
    if (left_line_no == 0)
        s << endl;
    else if (left_line_no >= right_line_no)
    {
        s << endl << endl;
        s.width(6);
        s << left_line_no << ". ";
        OutputLine(left_line_no, s);

        s.width(left_column_no + 8);
        s << '^';
        if (left_column_no < right_column_no)
        {
            s.width(right_column_no - left_column_no);
            s.fill('-');
            s << "^";
            s.fill(' ');
        }
    }
    else // multi-line
    {
        s << endl << endl;
        s.width(left_column_no + 8);
        s << "<";
        s.width(LineLength(left_line_no) - left_column_no);
        s.fill('-');
        s << "" << endl;
        s.fill(' ');

        s.width(6);
        s << left_line_no << ". ";
        OutputLine(left_line_no, s);
        if (right_line_no > left_line_no + 1)
            s << "   . . ." << endl;
        s.width(6);
        s << right_line_no << ". ";
        OutputLine(right_line_no, s);

        s.width(8);
        s << "";
        s.width(right_column_no);
        s.fill('-');
        s << ">";
        s.fill(' ');
    }
}


//
// Find and return the index of the first comment that immediately follows
// tok. Return 0 if there is not a comment that immediately follows tok.
//
LexStream::CommentIndex LexStream::FirstComment(TokenIndex tok)
{
    unsigned location = Location(tok);
    int lo = 0;
    int hi = comment_stream.Length() - 1;
    unsigned i = 0;
    if (lo < hi)
    {
        do
        {
            int mid = (lo + hi) / 2;
            if (comment_stream[mid].location < location)
                lo = mid + 1;
            else hi = mid - 1;
        } while (lo < hi);
        i = comment_stream[lo].location > location ? lo : lo + 1;
    }
    return i < (unsigned) comment_stream.Length() &&
        comment_stream[i].previous_token == tok ? i : 0;
}


unsigned LexStream::FindLine(unsigned location)
{
    int lo = 0;
    int hi = line_location.Length() - 1;

    assert(locations);

    //
    // we can place the exit test at the bottom of the loop
    // since the line_location array will always contain at least
    // one element.
    //
    do
    {
        int mid = (lo + hi) / 2;
        if (locations[mid] == location)
            return mid;
        if (locations[mid] < location)
             lo = mid + 1;
        else hi = mid - 1;
    } while (lo < hi);
    return locations[lo] > location ? lo - 1 : lo;
}

unsigned LexStream::FindColumn(unsigned loc)
{
    assert(locations);
    return input_buffer[loc] == U_LINE_FEED ? 0
        : Tab::Wcslen(input_buffer, locations[FindLine(loc)], loc);
}

void LexStream::ReadInput()
{
    if (file_symbol -> IsZip())
    {
        ZipFile* zipfile = new ZipFile(file_symbol);

        if (zipfile -> Buffer() == NULL)
        {
            fprintf(stderr, "chaos: Don\'t know how to process compressed "
                    "(\".java\") source in a zip file\n");
            assert(false);
        }
        else if (! file_symbol -> lex_stream)
        {
            // Once the zip file is loaded, it never changes. So, we only read
            // it the first time
            file_symbol -> lex_stream = this;
            ProcessInput(zipfile -> Buffer(),
                         file_symbol -> uncompressed_size);
        }
        delete zipfile;
    }
    else
    {
        struct stat status;
        JopaAPI::getInstance() -> stat(FileName(), &status);

        file_symbol -> mtime = status.st_mtime; // actual time stamp of file read
        file_symbol -> lex_stream = this;


        JopaAPI::FileReader* file =
            JopaAPI::getInstance() -> read(FileName());
        if (file)
        {
            ProcessInput(file -> getBuffer(), file -> getBufferSize());
            delete file;
        }
    }

    initial_reading_of_input = false;
}

void LexStream::RereadInput()
{
    if (input_buffer) // if input already available, do nothing
        ;
    else if (file_symbol -> IsZip())
    {
        ZipFile* zipfile = new ZipFile(file_symbol);

        if (zipfile -> Buffer() == NULL)
        {
            fprintf(stderr, "chaos: Don\'t know how to process compressed "
                    "(\".java\") source in a zip file\n");
            assert(false);
        }
        else ProcessInput(zipfile -> Buffer(),
                          file_symbol -> uncompressed_size);
        delete zipfile;
    }
    else
    {
        struct stat status;
        JopaAPI::getInstance() -> stat(FileName(), &status);

        if (status.st_mtime == file_symbol -> mtime)
        {
           JopaAPI::FileReader* file =
               JopaAPI::getInstance() -> read(FileName());
           if (file)
           {
               ProcessInput(file -> getBuffer(), file -> getBufferSize());
               delete file;
           }
        }
        else
        {
            // TODO: File has changed !!!
        }
    }
}


int LexStream::hexvalue(wchar_t ch)
{
    switch (ch)
    {
    case U_a: case U_A:
        return 10;
    case U_b: case U_B:
        return 11;
    case U_c: case U_C:
        return 12;
    case U_d: case U_D:
        return 13;
    case U_e: case U_E:
        return 14;
    case U_f: case U_F:
        return 15;
    default:
        return ch - U_0;
    }
}

//
// Store/convert filesize bytes from a file in the input_buffer.
//

#if defined(HAVE_ENCODING)

void LexStream::ProcessInput(const char* buffer, long filesize)
{
    LexStream::ProcessInputUnicode(buffer, filesize);
}

#else // defined(HAVE_ENCODING)

void LexStream::ProcessInput(const char* buffer, long filesize)
{
    LexStream::ProcessInputAscii(buffer, filesize);
}

void LexStream::ProcessInputAscii(const char* buffer, long filesize)
{
#ifdef JOPA_DEBUG
    file_read = true;
#endif

    wchar_t* input_ptr = AllocateInputBuffer(filesize);
    *input_ptr = U_LINE_FEED; // Add an initial '\n' for correct line numbers.

    if (buffer)
    {
        InitializeDataBuffer(buffer, filesize);

        while (source_ptr <= source_tail)
        {
            // The (& 0x00ff) guarantees that quantity is unsigned value.
            *(++input_ptr) = (*source_ptr++) & 0x00ff;

            //
            // During this pass, only flatten \u constructs. Even numbers of
            // \\ are ignored; odd is a unicode escape, which may have
            // unlimited u's (lowercase), then exactly 4 hex digits (no case).
            //
            if (*input_ptr == U_BACKSLASH)
            {
                if (source_ptr > source_tail)
                {
                    // Oops, file ended on single \. This will cause an
                    // error later in the scanner, so do nothing.
                }
                else if (*source_ptr == U_u)
                {
                    // Parse the unicode escape.
                    const char* u_ptr = source_ptr;
                    while (++source_ptr <= source_tail && *source_ptr == U_u);

                    *input_ptr = 0;
                    int i = 0;
                    bool bad_char = false;
                    for ( ; source_ptr <= source_tail && i < 4; i++)
                    {
                        const char ch = *source_ptr++;
                        switch (ch)
                        {
                        case U_a: case U_b: case U_c: case U_d:
                        case U_e: case U_f:
                            *input_ptr = (*input_ptr << 4) + (ch - (U_a - 10));
                            break;
                        case U_A: case U_B: case U_C: case U_D:
                        case U_E: case U_F:
                            *input_ptr = (*input_ptr << 4) + (ch - (U_A - 10));
                            break;
                        case U_0: case U_1: case U_2: case U_3:
                        case U_4: case U_5: case U_6: case U_7:
                        case U_8: case U_9:
                            *input_ptr = (*input_ptr << 4) + (ch - U_0);
                            break;
                        default:
                            bad_char = true;
                            *input_ptr <<= 4;
                        }
                    }
                    if (bad_char || i != 4)
                    {
                        if (initial_reading_of_input)
                            ReportMessage(StreamError::INVALID_UNICODE_ESCAPE,
                                          (unsigned) (input_ptr - input_buffer),
                                          (unsigned) (input_ptr - input_buffer) + (source_ptr - u_ptr));

                        // Restore the input such that we just pass the bad
                        // escape through to the next scan.
                        source_ptr = u_ptr;
                        *input_ptr = U_BACKSLASH;
                    }
                }
                else
                {
                    // All other escaped characters, including \, are just
                    // passed through to the next scan.
                    *(++input_ptr) = *source_ptr++;
                }
            }
            //
            // Replace \r with \n, \r\n with \n. Then the scanner only has
            // to look for \n, and we can use \r as an early EOF flag.
            //
            if (*input_ptr == U_CARRIAGE_RETURN)
            {
                *input_ptr = U_LINE_FEED;
                if (*source_ptr == U_LINE_FEED)
                    source_ptr++;
                else if (*source_ptr == U_BACKSLASH)
                {
                    //
                    // Remember, \u000a is U_LINE_FEED. Here, if we error out,
                    // do nothing, as the next pass through the outermost loop
                    // will catch it.
                    //
                    int i = 0;
                    while (source_ptr + i < source_tail &&
                           source_ptr[++i] == U_u);
                    if (i > 1 && (source_ptr + i + 3) <= source_tail &&
                        source_ptr[i] == U_0 && source_ptr[i + 1] == U_0 &&
                        source_ptr[i + 2] == U_0 &&
                        (source_ptr[i + 3] == U_a || source_ptr[i + 3] == U_A))
                    {
                        source_ptr += i + 4;
                    }
                }
            }
        }
    }

    //
    // To aid the scanner, we artificially remove any U_CTL_Z ending the file,
    // and insert U_CARRIAGE_RETURN, U_NULL. This is because U_CTL_Z is legal
    // inside comments, but // comments must end on a newline; and it is safe
    // since the above pass converted all CR's to LF's.
    //
    if (*input_ptr == U_CTL_Z)
        input_ptr--;
    if (initial_reading_of_input && control.option.pedantic &&
        *input_ptr != U_LINE_FEED)
    {
        ReportMessage(StreamError::LAST_CHARACTER_NOT_NEWLINE,
                      (unsigned) (input_ptr - input_buffer),
                      (unsigned) (input_ptr - input_buffer));
    }
    *(++input_ptr) = U_CARRIAGE_RETURN;
    *(++input_ptr) = U_NULL;
    input_buffer_length = input_ptr - input_buffer;
}

#endif // ! defined(HAVE_ENCODING)



#if defined(HAVE_ENCODING)

void LexStream::ProcessInputUnicode(const char* buffer, long filesize)
{
    //fprintf(stderr,"LexStream::ProcessInputUnicode called.\n");
#ifdef JOPA_DEBUG
    file_read = true;
#endif

    wchar_t* input_ptr = AllocateInputBuffer(filesize);
    wchar_t* input_tail = input_ptr + filesize;
    *input_ptr = U_LINE_FEED; // add an initial '\n';

    if (buffer)
    {
        int escape_value = 0;
        wchar_t* escape_ptr = NULL;
        UnicodeLexerState saved_state = RAW;
        UnicodeLexerState state = START;
        bool oncemore = false;

        // If oncemore is true, ch holds the current character, otherwise
        // it is updated to the next character
        wchar_t ch = 0;

        if (control.option.encoding)
        {
            // The encoding should have been validated by now
            bool encoding_set = SetEncoding(control.option.encoding);
            assert(encoding_set);
            (void)encoding_set;
        }

        // init data after setting the encoding
        InitializeDataBuffer(buffer, filesize);

        while (HasMoreData() || oncemore)
        {
            // On each iteration we advance input_ptr a maximum of 2 positions.
            // Here we check if we are close to the end of input_buffer.
            if (input_ptr >= input_tail)
            {
                // If this happens, reallocate it with some more space.
                // This is very rare case, which could happen if
                // one code page character is represented by several
                // unicode characters. One of exaples of such
                // situation is unicode "surrogates".
                //
                // If such reallocation will be required, it will indeed
                // slow down compilation a bit.
                size_t cursize = input_ptr - input_buffer;
                size_t newsize = cursize + cursize / 10 + 4; // add 10%
                wchar_t* tmp = new wchar_t[newsize];
                memcpy(tmp, input_buffer, cursize * sizeof(wchar_t));
                delete [] input_buffer;
                input_buffer = tmp;
                input_tail = input_buffer + newsize - 1;
                input_ptr  = input_buffer + cursize;
            }

            if (! oncemore)
            {
                ch = DecodeNextCharacter();
                if (ErrorDecodeNextCharacter())
                    break;
            }
            else oncemore = false;

            switch (state)
            {
            case QUOTE:
                *(++input_ptr) = U_BACKSLASH;
                if (ch == U_BACKSLASH)
                {
                    *(++input_ptr) = U_BACKSLASH;
                    state = RAW;
                }
                else if (ch == U_u)
                {
                    //
                    // We transfer all the characters of the escape sequence,
                    // in case it is invalid; but remember where it started
                    // for error reporting, and to back up on success.
                    //
                    escape_ptr = input_ptr;
                    *(++input_ptr) = U_u;
                    state = UNICODE_ESCAPE;
                }
                else
                {
                    state = RAW;
                    oncemore = true;
                }
                break;
            case UNICODE_ESCAPE:
                *(++input_ptr) = ch;
                if (Code::IsHexDigit(ch))
                {
                    state = UNICODE_ESCAPE_DIGIT_0;
                    escape_value = hexvalue(ch) << 12;
                }
                else if (ch != U_u)
                {
                    if (initial_reading_of_input)
                        ReportMessage(StreamError::INVALID_UNICODE_ESCAPE,
                                      (unsigned) (escape_ptr - input_buffer),
                                      ((unsigned) (input_ptr - input_buffer) -
                                       (Code::IsNewline(ch) ? 1 : 0)));
                    state = RAW;
                }
                break;
            case UNICODE_ESCAPE_DIGIT_0:
                *(++input_ptr) = ch;
                if (Code::IsHexDigit(ch))
                {
                    state = UNICODE_ESCAPE_DIGIT_1;
                    escape_value += hexvalue(ch) << 8;
                }
                else
                {
                    if (initial_reading_of_input)
                        ReportMessage(StreamError::INVALID_UNICODE_ESCAPE,
                                      (unsigned) (escape_ptr - input_buffer),
                                      ((unsigned) (input_ptr - input_buffer) -
                                       (Code::IsNewline(ch) ? 1 : 0)));
                    state = RAW;
                }
                break;
            case UNICODE_ESCAPE_DIGIT_1:
                *(++input_ptr) = ch;
                if (Code::IsHexDigit(ch))
                {
                    state = UNICODE_ESCAPE_DIGIT_2;
                    escape_value += hexvalue(ch) << 4;
                }
                else
                {
                    if (initial_reading_of_input)
                        ReportMessage(StreamError::INVALID_UNICODE_ESCAPE,
                                      (unsigned) (escape_ptr - input_buffer),
                                      ((unsigned) (input_ptr - input_buffer) -
                                       (Code::IsNewline(ch) ? 1 : 0)));
                    state = RAW;
                }
                break;
            case UNICODE_ESCAPE_DIGIT_2:
                if (Code::IsHexDigit(ch))
                {
                    ch = escape_value + hexvalue(ch);
                    state = saved_state;
                    input_ptr = escape_ptr - 1; // Back up - see case QUOTE.
                    oncemore = true;
                }
                else
                {
                    *(++input_ptr) = ch;
                    if (initial_reading_of_input)
                        ReportMessage(StreamError::INVALID_UNICODE_ESCAPE,
                                      (unsigned) (escape_ptr - input_buffer),
                                      ((unsigned) (input_ptr - input_buffer) -
                                       (Code::IsNewline(ch) ? 1 : 0)));
                    state = RAW;
                }
                saved_state = UNICODE_ESCAPE_DIGIT_2;
                break;
            case CR:
                if (ch == U_LINE_FEED)
                {
                    // skip line feed if it comes right after a CR.
                    state = RAW;
                }
                else if (ch == U_CARRIAGE_RETURN)
                {
                    // but if CR follows CR then the second CR starts a
                    // line feed too (and note that state=CR afterwards),
                    // so that CR-CR-LF will be handled correctly.
                    *(++input_ptr) = U_LINE_FEED;
                }
                else if (ch == U_BACKSLASH &&
                         saved_state != UNICODE_ESCAPE_DIGIT_2)
                {
                    state = QUOTE;
                }
                else
                {
                    state = RAW;
                    *(++input_ptr) = ch;
                }
                // clear saved_state == UNICODE_ESCAPE_DIGIT_2 status
                saved_state = CR;
                break;
            case START:
                // if for some reason converter produced or passed
                // byte order mark, it have to be ignored.
                state = RAW;
                if (ch == U_BOM || ch == U_REVERSE_BOM)
                    break; //ignore
                // fallthrough
            case RAW:
                if (ch == U_BACKSLASH && saved_state != UNICODE_ESCAPE_DIGIT_2)
                {
                    state = QUOTE;
                }
                else if (ch == U_CARRIAGE_RETURN)
                {
                    state = CR;
                    *(++input_ptr) = U_LINE_FEED;
                }
                else
                {
                    *(++input_ptr) = ch;
                }
                saved_state = RAW;
                break;
            }
        }
        if (state == QUOTE)
        {
            *(++input_ptr) = U_BACKSLASH;
        }
        else if (state >= UNICODE_ESCAPE)
        {
            if (initial_reading_of_input)
                ReportMessage(StreamError::INVALID_UNICODE_ESCAPE,
                              (unsigned) (escape_ptr - input_buffer),
                              (unsigned) (input_ptr - input_buffer));
        }
    }

    //
    // To aid the scanner, we artificially remove any U_CTL_Z ending the file,
    // and insert U_CARRIAGE_RETURN, U_NULL. This is because U_CTL_Z is legal
    // inside comments, but // comments must end on a newline; and it is safe
    // since the above pass converted all CR's to LF's.
    //
    if (*input_ptr == U_CTL_Z)
        input_ptr--;
    if (initial_reading_of_input && control.option.pedantic &&
        *input_ptr != U_LINE_FEED)
    {
        ReportMessage(StreamError::LAST_CHARACTER_NOT_NEWLINE,
                      (unsigned) (input_ptr - input_buffer),
                      (unsigned) (input_ptr - input_buffer));
    }
    *(++input_ptr) = U_CARRIAGE_RETURN;
    *(++input_ptr) = U_NULL;
    input_buffer_length = input_ptr - input_buffer;
}
#endif // defined(HAVE_ENCODING)

void LexStream::ReportMessage(StreamError::StreamErrorKind kind,
                              unsigned start_location,
                              unsigned end_location)
{
    if (control.option.tolerance != JopaOption::NO_WARNINGS ||
        kind < StreamError::DEPRECATED_IDENTIFIER_ASSERT)
    {
        bad_tokens.Next().Initialize(kind, start_location, end_location, this);
    }
}

//
// This procedure uses a  quick sort algorithm to sort the stream ERRORS
// by their locations.
//
void LexStream::SortMessages()
{
     int lower,
         upper,
         lostack[32],
         histack[32];

     int top,
         i,
         j;
     StreamError pivot,
                 temp;

     top = 0;
     lostack[top] = 0;
     histack[top] = bad_tokens.Length() - 1;

     while (top >= 0)
     {
         lower = lostack[top];
         upper = histack[top];
         top--;

         while (upper > lower)
         {
             //
             // The array is most-likely almost sorted. Therefore,
             // we use the middle element as the pivot element.
             //
             i = (lower + upper) / 2;
             pivot = bad_tokens[i];
             bad_tokens[i] = bad_tokens[lower];

             //
             // Split the array section indicated by LOWER and UPPER
             // using ARRAY(LOWER) as the pivot.
             //
             i = lower;
             for (j = lower + 1; j <= upper; j++)
             {
                 if (bad_tokens[j].start_location < pivot.start_location)
                 {
                     temp = bad_tokens[++i];
                     bad_tokens[i] = bad_tokens[j];
                     bad_tokens[j] = temp;
                 }
             }
             bad_tokens[lower] = bad_tokens[i];
             bad_tokens[i] = pivot;

             top++;
             if ((i - lower) < (upper - i))
             {
                 lostack[top] = i + 1;
                 histack[top] = upper;
                 upper = i - 1;
             }
             else
             {
                 histack[top] = i - 1;
                 lostack[top] = lower;
                 lower = i + 1;
             }
         }
     }
}


//
//
//
void LexStream::PrintMessages()
{
    //
    // If control.option.dump_errors then the error messages have already
    // been printed
    //
    if (! control.option.dump_errors)
    {
        RereadInput();

        if (control.option.errors)
        {
            char* file_name = FileName();

            int error_count = NumBadTokens(),
                warning_count = NumWarnTokens();
            if (error_count)
            {
                Coutput << endl << "Found " << error_count << " lexical error"
                        << (error_count == 1 ? "" : "s");
            }
            if (warning_count)
            {
                if (error_count)
                    Coutput << "and issued ";
                else
                    Coutput << endl << "Issued ";
                Coutput << warning_count << " lexical warning"
                        << (warning_count == 1 ? "" : "s");
            }
            if (error_count || warning_count)
                Coutput << " in \"" << file_name << "\":";

            if (! input_buffer)
            {
                int length = FileNameLength();
                wchar_t* name = new wchar_t[length + 1];
                for (int i = 0; i < length; i++)
                    name[i] = file_name[i];
                name[length] = U_NULL;
                control.system_semantic ->
                    ReportSemError(SemanticError::CANNOT_REOPEN_FILE,
                                   BAD_TOKEN, name);
                delete [] name;
            }
            else
            {
                for (unsigned i = 0; i < bad_tokens.Length(); i++)
                    JopaAPI::getInstance() -> reportError(&bad_tokens[i]);
            }
        }
        else
        {
            for (unsigned i = 0; i < bad_tokens.Length(); i++)
                JopaAPI::getInstance() -> reportError(&bad_tokens[i]);
        }

        DestroyInput();
        Coutput.flush();
    }
}


} // Close namespace Jopa block

