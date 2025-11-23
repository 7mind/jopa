#ifndef stream_INCLUDED
#define stream_INCLUDED

#include "platform.h"
#include "tuple.h"
#include "jikesapi.h"


namespace Jikes { // Open namespace Jikes block
class Control;
class Input_info;
class Scanner;
class Symbol;
class FileSymbol;
class ZipFile;
class LexStream;
class ErrorString;

class StreamError : public JikesError
{
    friend class LexStream;

public:

    StreamError();

    virtual const wchar_t* getErrorMessage();
    virtual const wchar_t* getErrorReport();

    virtual JikesErrorSeverity getSeverity();
    virtual const char* getFileName();

    virtual int getLeftLineNo() { return left_line_no; }
    virtual int getLeftColumnNo() { return left_column_no; }
    virtual int getRightLineNo() { return right_line_no; }
    virtual int getRightColumnNo() { return right_column_no; }

    enum StreamErrorKind
    {
        BAD_TOKEN,
        EMPTY_CHARACTER_CONSTANT,
        UNTERMINATED_CHARACTER_CONSTANT,
        MULTI_CHARACTER_CONSTANT,
        ESCAPE_EXPECTED,
        UNTERMINATED_COMMENT,
        UNTERMINATED_STRING_CONSTANT,
        INVALID_HEX_CONSTANT,
        INVALID_FLOATING_HEX_EXPONENT,
        INVALID_FLOATING_HEX_MANTISSA,
        INVALID_FLOATING_HEX_PREFIX,
        INVALID_OCTAL_CONSTANT,
        INVALID_FLOATING_EXPONENT,
        INVALID_UNICODE_ESCAPE,
        INVALID_ESCAPE_SEQUENCE,
        LAST_CHARACTER_NOT_NEWLINE, // pedantic only
        DEPRECATED_IDENTIFIER_ASSERT, // from here, these are warnings only
        DEPRECATED_IDENTIFIER_ENUM,
        DOLLAR_IN_IDENTIFIER,
        FAVOR_CAPITAL_L_SUFFIX
    };

private:

    unsigned start_location;
    unsigned end_location;
    StreamErrorKind kind;

    static bool emacs_style_report;
    LexStream* lex_stream;

    int left_line_no;
    int left_column_no;
    int right_line_no;
    int right_column_no;

    const wchar_t* regularErrorString();
    const wchar_t* emacsErrorString();

    bool initialized;

    void Initialize(StreamErrorKind, unsigned, unsigned, LexStream*);
};


//
// The stream class encapsulates details related to reading
// a stream of possibly encoded data from the file system.
//
class Stream
{
public:

    Stream();
    ~Stream();

    void DestroyInput()
    {
        delete [] input_buffer;
        input_buffer = NULL;
    }

    inline const wchar_t* InputBuffer() { return input_buffer; }
    inline unsigned InputBufferLength() { return input_buffer_length; }

    inline wchar_t* AllocateInputBuffer(unsigned size)
    {
        // +3 for leading \n, trailing \r\0
        return input_buffer = new wchar_t[size + 3];
    }

#if defined(HAVE_ENCODING)
    static bool IsSupportedEncoding(char* encoding);
    bool SetEncoding(char* encoding);
#endif

protected:

    wchar_t* input_buffer;
    unsigned input_buffer_length;

    const char* source_ptr;    // Start of data buffer to decoded
    const char* source_tail;   // End of data buffer to be decoded
    const char* data_buffer;   // The data to be decoded

    bool error_decode_next_character;

//private: // FIXME : Make vars private once extracted from LexStream!

#ifdef HAVE_ENCODING

#if defined(HAVE_LIBICU_UC)
    UConverter* _decoder;
#elif defined(JIKES_ICONV_ENCODING)
    iconv_t _decoder;
#endif

    void DestroyEncoding();

    // Read the next wchar_t from the stream.
    // If an error occurs the ErrorDecodeNextCharacter
    // method will return true on the next call.

    wchar_t DecodeNextCharacter();

    inline bool ErrorDecodeNextCharacter()
    {
        bool result = error_decode_next_character;
        if (result)
            error_decode_next_character = false;
        return result;
    }

    // Returns true if an encoding has been set

    inline bool HaveDecoder()
    {
#if defined(HAVE_LIBICU_UC)
        return _decoder != NULL;
#elif defined(JIKES_ICONV_ENCODING)
        return _decoder != (iconv_t) -1;
#endif
    }

#endif // HAVE_ENCODING

    inline void InitializeDataBuffer(const char* buffer, long size)
    {
        data_buffer = buffer;
        source_ptr = data_buffer;
        source_tail = data_buffer + size - 1;
    }

    inline bool HasMoreData()
    {
        return source_ptr <= source_tail;
    }
};


//
// LexStream holds a stream of tokens generated from an input and
// provides methods to retrieve information from the stream.
//
class LexStream : public Stream
{
    friend class StreamError;

public:
    typedef unsigned CommentIndex;
    enum { LEX_INFINITY = INT_MAX }; // the largest value for TokenIndex

    FileSymbol* file_symbol;

    inline TokenIndex Next(TokenIndex i)
    {
        return ++i < token_stream.Length() ? i : token_stream.Length() - 1;
    }
    inline TokenIndex Previous(TokenIndex i) { return i <= 0 ? 0 : i - 1; }
    inline TokenIndex Peek() { return Next(index); }
    inline void Reset(TokenIndex i = 1) { index = Previous(i); }
    inline TokenIndex Gettoken() { return index = Next(index); }
    inline TokenIndex Gettoken(TokenIndex end_token)
    {
        return index = (index < end_token ? Next(index)
                        : token_stream.Length() - 1);
    }

    inline unsigned Kind(TokenIndex i)
    {
        return tokens[i >= NumTokens() ? NumTokens() - 1 : i].Kind();
    }

    inline unsigned Location(TokenIndex i)
    {
        assert(i < NumTokens());
        return tokens[i].Location();
    }

    inline unsigned Line(TokenIndex i)
    {
        return FindLine(tokens[i].Location());
    }

    inline unsigned Column(TokenIndex i)
    {
        // FindColumn grabs the right edge of an expanded character.
        return input_buffer ? FindColumn(tokens[i].Location() - 1) + 1 : 0;
    }
    unsigned RightColumn(TokenIndex i);

    inline bool AfterEol(TokenIndex i)
    {
        return i < 1 ? true : Line(i - 1) < Line(i);
    }

    inline bool IsDeprecated(TokenIndex i) { return tokens[i].Deprecated(); }

    inline TokenIndex MatchingBrace(TokenIndex i)
    {
        return tokens[i].additional_info.right_brace;
    }

    const wchar_t* NameString(TokenIndex i);
    unsigned NameStringLength(TokenIndex i);

    // TODO: Rename these methods to differ from the class name?
    class LiteralSymbol* LiteralSymbol(TokenIndex);
    class NameSymbol* NameSymbol(TokenIndex);

    char* FileName();
    unsigned FileNameLength();

    unsigned LineLength(unsigned line_no);
    inline unsigned LineStart(unsigned line_no)
    {
        return locations[line_no];
    }
    inline unsigned LineEnd(unsigned line_no)
    {
        return locations[line_no + 1] - 1;
    }

    unsigned LineSegmentLength(TokenIndex i);

    //
    // For a sequence of tokens in a given range find out how many large
    // characters they contain and compute the appropriate offset.
    //
    inline unsigned WcharOffset(TokenIndex start, TokenIndex end)
    {
        unsigned offset = 0;
        for (TokenIndex i = start; i <= end; i++)
        {
            for (const wchar_t* str = NameString(i); *str; str++)
            {
                if (*str > 0xff)
                    offset += 5;
            }
        }

        return offset;
    }

    //
    // When only an end token is supplied, the start token is assume to be the
    // first one on the same line.
    //
    inline unsigned WcharOffset(TokenIndex end)
    {
        TokenIndex start = end;
        unsigned the_line = Line(end);
        while (Line(--start) == the_line);
        return WcharOffset(start + 1, end);
    }

    //
    // Used for outputting sections of source code in error messages.
    //
    void OutputLine(unsigned, ErrorString&);
    void OutputSource(JikesError*, ErrorString&);

    CommentIndex FirstComment(TokenIndex);

    inline unsigned NumTypes() { return type_index.Length(); }
    inline TokenIndex Type(unsigned i) { return types[i]; }

    inline unsigned NumTokens() { return token_stream.Length(); }
    inline unsigned NumComments() { return comment_stream.Length(); }
    inline TokenIndex PrecedingToken(CommentIndex i)
    {
        return comments[i].previous_token;
    }
    inline unsigned CommentLocation(CommentIndex i)
    {
        return comments[i].location;
    }

    inline const wchar_t* CommentString(CommentIndex i)
    {
        return comments[i].string;
    }

    inline unsigned CommentStringLength(CommentIndex i)
    {
        return comments[i].length;
    }

    inline TokenIndex PackageToken()
    {
        return package;
    }

    inline unsigned NumBadTokens()
    {
        unsigned count = 0;
        for (unsigned i = 0; i < bad_tokens.Length(); i++)
            if (bad_tokens[i].getSeverity() == JikesError::JIKES_ERROR)
                count++;
        return count;
    }
    inline unsigned NumWarnTokens()
    {
        return bad_tokens.Length() - NumBadTokens();
    }

#ifdef JIKES_DEBUG
    bool file_read;
#endif

    //
    // Constructors and Destructor.
    //
    LexStream(Control&, FileSymbol*);

    void RereadInput();
    ~LexStream();

    void DestroyInput()
    {
        Stream::DestroyInput();

        delete [] comment_buffer;
        comment_buffer = NULL;
    }

    void ReportMessage(StreamError::StreamErrorKind,
                       unsigned start, unsigned end);
    void SortMessages();
    void PrintMessages();

    void SetUpComments()
    {
        if (comment_buffer)
            return;
        RereadInput();
        //
        // Calculate the length of the string required to save the comments.
        // Allocate the buffer, save the comments in the buffer and update
        // their respective "string" pointer.
        //
        unsigned length = 0;
        unsigned i;

        for (i = 1; i < comment_stream.Length(); i++)
            length += (comments[i].length + 1);
        comment_buffer = new wchar_t[length];
        wchar_t* ptr = comment_buffer;
        for (i = 1; i < comment_stream.Length(); i++)
        {
            memcpy(ptr, &(input_buffer[comments[i].location]),
                   comments[i].length * sizeof(wchar_t));
            comments[i].string = ptr;
            ptr += comments[i].length;
            *ptr++ = U_NULL;
        }
    }

#ifdef JIKES_DEBUG
    void Dump(); // temporary function used to dump token stream.
#endif

    //
    // Return the total size of space allocated for the tokens.
    //
    size_t TokenSpaceAllocated(void)
    {
        return token_stream.Length() * sizeof(Token);
    }

    //
    // Return the total size of space allocated for the comments.
    //
    size_t CommentSpaceAllocated(void)
    {
        return comment_stream.Length() * sizeof(Comment);
    }

private:

    int hexvalue(wchar_t ch);

#if defined(HAVE_ENCODING)
    enum UnicodeLexerState
    {
        START,
        RAW,
        CR,
        QUOTE,
        UNICODE_ESCAPE,
        UNICODE_ESCAPE_DIGIT_0,
        UNICODE_ESCAPE_DIGIT_1,
        UNICODE_ESCAPE_DIGIT_2
    };
#endif // HAVE_ENCODING

    friend class Scanner;

    struct Comment
    {
        TokenIndex previous_token;
        unsigned location;
        unsigned length;
        wchar_t* string;
    };

    class Token
    {
        //
        // It is expected that a location will be set for every token.
        // Therefore, as we are setting the location, we also reset the
        // deprecated bit to 0. If it is subsequently discovered that the
        // token is followed by one or more deprecated tags then the bit is
        // set to 1 by an invocation of the function SetDeprecated. Note that
        // a better way to resetting all the bits in "info" is to use the
        // function ResetInfoAndSetLocation defined below, instead of using
        // SetLocation.
        //
        inline void SetLocation(unsigned location)
        {
            assert(location <= 0x00FFFFFF);
            info = (info & 0x0000007F) | (location << 8);
        }

    public:
        unsigned info;
        union
        {
            Symbol* symbol;
            TokenIndex right_brace;
        } additional_info;

        //
        // To just reset the info, this function should be invoked with a
        // location value of 0.
        //
        inline void ResetInfoAndSetLocation(unsigned location)
        {
            assert(location <= 0x00FFFFFF);
            info = (location << 8);
            additional_info.symbol = NULL;
        }

        inline unsigned Location() { return info >> 8; }
        inline void SetKind(unsigned kind)
        {
            assert(kind <= 0x0000007F);
            info = (info & 0xFFFFFF80) | kind;
        }
        inline unsigned Kind() { return info & 0x0000007F; }
        inline void ResetDeprecated() { info &= ~0x00000080; }
        inline void SetDeprecated() { info |= 0x00000080; }
        inline bool Deprecated() { return (info & 0x00000080) != 0; }

        inline void SetSymbol(Symbol* symbol)
        {
            additional_info.symbol = symbol;
        }
        inline void SetRightBrace(TokenIndex rbrace)
        {
            additional_info.right_brace = rbrace;
        }
    };

    TokenIndex GetNextToken(unsigned location = 0)
    {
        TokenIndex index = token_stream.NextIndex();
        token_stream[index].ResetInfoAndSetLocation(location);

        return index;
    }

    Tuple<StreamError> bad_tokens;

    TokenIndex index;
    Token* tokens;
    ConvertibleArray<Token> token_stream;
    Comment* comments;
    ConvertibleArray<Comment> comment_stream;
    unsigned* locations;
    ConvertibleArray<unsigned> line_location;
    TokenIndex* types;
    ConvertibleArray<TokenIndex> type_index;
    TokenIndex package;

    void CompressSpace();

    bool initial_reading_of_input;

    wchar_t* comment_buffer;

    Control& control;

    void ReadInput();
    void ProcessInput(const char*, long);
#if defined(HAVE_ENCODING)
    void ProcessInputUnicode(const char*, long);
#else
    void ProcessInputAscii(const char*, long);
#endif // defined(HAVE_ENCODING)

    const wchar_t* KeywordName(int);

    unsigned FindLine(unsigned location);

    //
    // Finds the column of the right edge of a character.
    //
    unsigned FindColumn(unsigned loc);
};


} // Close namespace Jikes block

#endif // stream_INCLUDED

