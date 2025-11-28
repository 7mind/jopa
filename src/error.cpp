#include "error.h"
#include "control.h"
#include "semantic.h"
#include "ast.h"
#include "diagnose.h"
#include "option.h"
#include "jikesapi.h"
#include "stream.h"


namespace Jopa { // Open namespace Jopa block
void Semantic::PrintMessages()
{
    if (this != control.system_semantic)
    {
        if (lex_stream -> NumBadTokens() > 0)
        {
            lex_stream -> PrintMessages();
            return_code = 1;
        }
        else if (lex_stream -> NumWarnTokens() > 0)
            lex_stream -> PrintMessages();

        if (! compilation_unit ||
            compilation_unit -> BadCompilationUnitCast())
        {
            DiagnoseParser *diagnose_parser =
                new DiagnoseParser(control, lex_stream);
            return_code = 1;
            delete diagnose_parser;
        }

        if (! control.option.nocleanup && compilation_unit)
            CleanUp();
    }

    if (error && error -> error.Length() > 0 &&
        error -> PrintMessages() > return_code)
    {
        return_code = 1;
    }

    //
    // Once we have processed the errors, reset the error object
    //
    delete error;
    error = NULL;
}


ErrorString::ErrorString()
    : ConvertibleArray<wchar_t>(1024),
      fill_char(' '),
      field_width(0)
{
}

void ErrorString::DoFill(int n)
{
    while (n < field_width)
    {
        Next() = (wchar_t) fill_char;
        n++;
    }
    field_width = 0;
}

ErrorString& ErrorString::operator<<(const wchar_t c)
{
    DoFill(1);
    Next() = c;
    return *this;
}

ErrorString& ErrorString::operator<<(const char c)
{
    DoFill(1);
    Next() = (wchar_t) c;
    return *this;
}

ErrorString& ErrorString::operator<<(const wchar_t* s)
{
    if (s)
    {
        DoFill(
                wcslen(
#ifdef HAVE_ERROR_CALL_WCSLEN_CONST
                       (wchar_t*)
#endif
                       s)
                );
        while (*s)
            Next() = *(s++);
    }

    return *this;
}

ErrorString& ErrorString::operator<<(const char* s)
{
    if (s)
    {
        DoFill(strlen(s));
        while (*s)
            Next() = (wchar_t) *(s++);
    }

    return *this;
}

ErrorString& ErrorString::operator<<(int n)
{
    char buf[64];
    sprintf(buf, "%d", n);

    return (*this << buf);
}

const wchar_t* ErrorString::Array()
{
    // Not thread-safe, but jikes isn't threaded.
    static wchar_t* result = NULL;

    Next() = U_NULL; // zero terminate string
    delete [] result;
    result = new wchar_t[top];
    memcpy(result, ConvertibleArray<wchar_t>::Array(), top * sizeof(wchar_t));
    return result;
}

const wchar_t* ErrorString::SafeArray()
{
    // safe, but the caller will be resposible for freeing the memory
    Next() = U_NULL; // zero terminate string
    wchar_t* result = new wchar_t[top];
    memcpy(result, ConvertibleArray<wchar_t>::Array(), top * sizeof(wchar_t));
    return result;
}

void ErrorString::width(int w)
{
    field_width = w;
}

void ErrorString::fill(const char c)
{
    fill_char = c;
}


SemanticError::WarningLevel SemanticError::warning[_num_kinds] = {
    MANDATORY_ERROR
};
const char* SemanticError::messages[_num_kinds] = { 0 };

void ErrorInfo::Initialize(LexStream* l)
{
    lex_stream = l;
    left_line_no = lex_stream -> Line(left_token);
    left_column_no = lex_stream -> Column(left_token);
    right_line_no = lex_stream -> Line(right_token);
    right_column_no = lex_stream -> RightColumn(right_token);
}

ErrorInfo::ErrorInfo()
    : msg(NULL),
      severity(JopaError::JOPA_ERROR)
{
}

ErrorInfo::~ErrorInfo()
{
}


JopaError::JopaErrorSeverity ErrorInfo::getSeverity() { return severity; }
int ErrorInfo::getLeftLineNo() { return left_line_no; }
int ErrorInfo::getLeftColumnNo() { return left_column_no; }
int ErrorInfo::getRightLineNo() { return right_line_no; }
int ErrorInfo::getRightColumnNo() { return right_column_no; }

const char* ErrorInfo::getFileName()
{
    assert(lex_stream);
    return lex_stream -> FileName();
}

const wchar_t* ErrorInfo::getErrorMessage()
{
    assert(msg);
    return msg;
}

bool ErrorInfo::emacs_style_report = false;

const wchar_t* ErrorInfo::getErrorReport()
{
    return emacs_style_report ? emacsErrorString() : regularErrorString();
}

const wchar_t* ErrorInfo::regularErrorString()
{
    ErrorString s;

    lex_stream -> OutputSource(this, s);

    s << endl << "*** Semantic " << getSeverityString() << ": "
      << getErrorMessage();

    return s.Array();
}


const wchar_t* ErrorInfo::emacsErrorString()
{
    ErrorString s;

    s << getFileName()
      << ':' << left_line_no  << ':' << left_column_no
      << ':' << right_line_no << ':' << right_column_no
      << ": Semantic " << getSeverityString() << ": "
      << getErrorMessage();

    return s.Array();
}


SemanticError::SemanticError(Control& control_,
                             FileSymbol* file_symbol)
    : num_errors(0),
      num_warnings(0),
      control(control_),
      lex_stream(file_symbol -> lex_stream),
      clone_count(0),
      buffer(1024),
      error(512)
{
    ErrorInfo::emacs_style_report = ! control.option.errors;
}


//
// This procedure is invoked by a JIKES PARSER or a semantic
// routine to process an error message.  The JIKES parser always
// passes the value 0 to msg_level to indicate an error.
// This routine simply stores all necessary information about
// the message into an array: error.
//
void SemanticError::Report(SemanticErrorKind msg_code,
                           TokenIndex left_token, TokenIndex right_token,
                           const wchar_t* insert1, const wchar_t* insert2,
                           const wchar_t* insert3, const wchar_t* insert4,
                           const wchar_t* insert5, const wchar_t* insert6,
                           const wchar_t* insert7, const wchar_t* insert8,
                           const wchar_t* insert9)
{
    //
    // Do not report errors detected while processing a clone !!!
    //
    assert(msg_code < _num_kinds);
    if (clone_count)
        return;

    //
    // Some warning severities are dependent on command-line options, and
    // may need rewriting from what we have in warning[].
    //
    switch (warning[msg_code])
    {
    case NAMED_WEAK_OFF:
    case NAMED_STRONG_OFF:
    case DISABLED:
        warning[msg_code] = DISABLED;
        return;
    case NAMED_WEAK_ON:
    case WEAK_WARNING:
        warning[msg_code] =
            control.option.tolerance & JopaOption::WARNINGS_ARE_ERRORS
                ? MANDATORY_ERROR : WEAK_WARNING;
        break;
    case NAMED_STRONG_ON:
    case STRONG_WARNING:
        warning[msg_code] =
            control.option.tolerance & JopaOption::CAUTIONS_ARE_ERRORS
                ? MANDATORY_ERROR : STRONG_WARNING;
        break;
    case MANDATORY_ERROR:
        break;
    }

    //
    // Don't report non-mandatory errors if we're in -nowarn mode.
    //
    if (control.option.tolerance == JopaOption::NO_WARNINGS &&
        warning[msg_code] != MANDATORY_ERROR)
    {
        return;
    }

    int i = error.NextIndex();

    if (warning[msg_code] != MANDATORY_ERROR)
        num_warnings++;
    else num_errors++;

    error[i].msg_code = msg_code;
    error[i].severity = (JopaError::JopaErrorSeverity) warning[msg_code];

    int total_length = 0,
        length1 = 0,
        length2 = 0,
        length3 = 0,
        length4 = 0,
        length5 = 0,
        length6 = 0,
        length7 = 0,
        length8 = 0,
        length9 = 0;
    // Change ErrorInfo::MAX_INSERTS if you need more inserts.

    if (insert1)
    {
        length1 = wcslen(insert1);
        total_length += (length1 + 1);
    }
    else error[i].insert[0] = NULL;
    if (insert2)
    {
        length2 = wcslen(insert2);
        total_length += (length2 + 1);
    }
    else error[i].insert[1] = NULL;
    if (insert3)
    {
        length3 = wcslen(insert3);
        total_length += (length3 + 1);
    }
    else error[i].insert[2] = NULL;
    if (insert4)
    {
        length4 = wcslen(insert4);
        total_length += (length4 + 1);
    }
    else error[i].insert[3] = NULL;
    if (insert5)
    {
        length5 = wcslen(insert5);
        total_length += (length5 + 1);
    }
    else error[i].insert[4] = NULL;
    if (insert6)
    {
        length6 = wcslen(insert6);
        total_length += (length6 + 1);
    }
    else error[i].insert[5] = NULL;
    if (insert7)
    {
        length7 = wcslen(insert7);
        total_length += (length7 + 1);
    }
    else error[i].insert[6] = NULL;
    if (insert8)
    {
        length8 = wcslen(insert8);
        total_length += (length8 + 1);
    }
    else error[i].insert[7] = NULL;
    if (insert9)
    {
        length9 = wcslen(insert9);
        total_length += (length9 + 1);
    }
    else error[i].insert[8] = NULL;

    if (total_length > 0)
    {
        wchar_t* ptr = new wchar_t[total_length];
        buffer.Next() = ptr;
        if (insert1)
        {
            memcpy(ptr, insert1, length1 * sizeof(wchar_t));
            error[i].insert[0] = ptr;
            ptr += length1;
            *ptr++ = U_NULL;
        }
        if (insert2)
        {
            memcpy(ptr, insert2, length2 * sizeof(wchar_t));
            error[i].insert[1] = ptr;
            ptr += length2;
            *ptr++ = U_NULL;
        }
        if (insert3)
        {
            memcpy(ptr, insert3, length3 * sizeof(wchar_t));
            error[i].insert[2] = ptr;
            ptr += length3;
            *ptr++ = U_NULL;
        }
        if (insert4)
        {
            memcpy(ptr, insert4, length4 * sizeof(wchar_t));
            error[i].insert[3] = ptr;
            ptr += length4;
            *ptr++ = U_NULL;
        }
        if (insert5)
        {
            memcpy(ptr, insert5, length5 * sizeof(wchar_t));
            error[i].insert[4] = ptr;
            ptr += length5;
            *ptr++ = U_NULL;
        }
        if (insert6)
        {
            memcpy(ptr, insert6, length6 * sizeof(wchar_t));
            error[i].insert[5] = ptr;
            ptr += length6;
            *ptr++ = U_NULL;
        }
        if (insert7)
        {
            memcpy(ptr, insert7, length7 * sizeof(wchar_t));
            error[i].insert[6] = ptr;
            ptr += length7;
            *ptr++ = U_NULL;
        }
        if (insert8)
        {
            memcpy(ptr, insert8, length8 * sizeof(wchar_t));
            error[i].insert[7] = ptr;
            ptr += length8;
            *ptr++ = U_NULL;
        }

        if (insert9)
        {
            memcpy(ptr, insert9, length9 * sizeof(wchar_t));
            error[i].insert[8] = ptr;
            ptr += length9;
            *ptr++ = U_NULL;
        }
    }

    error[i].num = i;
    error[i].left_token = (left_token > right_token
                           ? right_token : left_token);
    error[i].right_token = right_token;

    //
    // Dump the error immediately ?
    //
    if (control.option.dump_errors)
    {
        lex_stream -> RereadInput();
        reportError(i);

        if (buffer.Length() > 0)
        {
            delete [] buffer[0];
            buffer.Reset();
        }
        // we need at least 1 error in order for the return code to be
        // set properly. See PrintMessages().
        error.Reset(1);
    }
}

void SemanticError::StaticInitializer()
{
    static bool initialized = false;

    if (initialized)
        return;
    initialized = true;

    //
    // If not explicitly told otherwise, we assume that a SemanticErrorKind
    // is an error (as opposed to a warning or a caution). An individual
    // warning or caution can be marked as such below; a group of related
    // warnings is better defined in InitializeMessageGroups where the
    // group can be named for command-line use.
    //
    memset(warning, MANDATORY_ERROR, _num_kinds * sizeof(unsigned char));

    //
    // Weak warnings.
    //
    warning[CANNOT_OPEN_ZIP_FILE] = WEAK_WARNING;
    warning[CANNOT_OPEN_PATH_DIRECTORY] = WEAK_WARNING;
    warning[IO_WARNING] = WEAK_WARNING;

    warning[EMPTY_DECLARATION] = WEAK_WARNING;
    warning[DUPLICATE_THROWS_CLAUSE_CLASS] = WEAK_WARNING;
    warning[REDUNDANT_THROWS_CLAUSE_CLASS] = WEAK_WARNING;
    warning[UNCHECKED_THROWS_CLAUSE_CLASS] = WEAK_WARNING;
    warning[NO_TYPES] = WEAK_WARNING;

    warning[DEPRECATED_TYPE] = WEAK_WARNING;
    warning[DEPRECATED_FIELD] = WEAK_WARNING;
    warning[DEPRECATED_METHOD] = WEAK_WARNING;
    warning[DEPRECATED_CONSTRUCTOR] = WEAK_WARNING;

    warning[UNNECESSARY_TYPE_IMPORT] = WEAK_WARNING;
    warning[MULTIPLE_PUBLIC_TYPES] = WEAK_WARNING;
    warning[TYPE_IN_MULTIPLE_FILES] = WEAK_WARNING;
    warning[MISMATCHED_TYPE_AND_FILE_NAMES] = WEAK_WARNING;
    warning[REFERENCE_TO_TYPE_IN_MISMATCHED_FILE] = WEAK_WARNING;
    warning[RECOMPILATION] = WEAK_WARNING;
    warning[METHOD_WITH_CONSTRUCTOR_NAME] = WEAK_WARNING;
    warning[DEFAULT_METHOD_NOT_OVERRIDDEN] = WEAK_WARNING;
    warning[INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_LOCAL] =
        WEAK_WARNING;
    warning[INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_MEMBER] =
        WEAK_WARNING;
    warning[CLASS_METHOD_INVOKED_VIA_INSTANCE] = WEAK_WARNING;
    warning[CLASS_FIELD_ACCESSED_VIA_INSTANCE] = WEAK_WARNING;
    warning[ASSIGNMENT_USED_AS_TRUTH_VALUE] = WEAK_WARNING;
    warning[NON_STATIC_FINAL_CONSTANT_FIELD] = WEAK_WARNING;
    warning[AMBIGUOUS_NULL_VARARG] = WEAK_WARNING;
    warning[UNCHECKED_TYPE_CONVERSION] = WEAK_WARNING;

    //
    // Somewhat stronger warnings, but code will be generated anyway.
    //
    warning[OBSOLESCENT_BRACKETS] = STRONG_WARNING;
    warning[NEGATIVE_ARRAY_SIZE] = STRONG_WARNING;
    warning[NEGATIVE_SHIFT_COUNT] = STRONG_WARNING;
    warning[SHIFT_COUNT_TOO_LARGE] = STRONG_WARNING;
    warning[UNNECESSARY_PARENTHESIS] = STRONG_WARNING;
    warning[ZERO_DIVIDE_CAUTION] = STRONG_WARNING;
    warning[UNIMPLEMENTABLE_INTERFACE] = STRONG_WARNING;
    warning[UNIMPLEMENTABLE_CLASS] = STRONG_WARNING;
    warning[INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_TYPE] =
        STRONG_WARNING;
    warning[CONSTANT_OVERFLOW] = STRONG_WARNING;

    InitializeMessages();
    InitializeMessageGroups();
}

void SemanticError::SetWarningLevel(SemanticErrorKind code, WarningLevel level)
{
    warning[code] = level;
}

//
// Describes an error code (or group of error codes) for the purpose
// of turning them on or off by name.
//
// 'name' is used on the command-line, and with 'reason' in
// Jikes' -help output.
//
struct MessageGroup
{
    MessageGroup(const char* name, const char* reason,
                 const SemanticError::WarningLevel level)
        : name(name), reason(reason), level(level)
    {}

    void AddMessage(const SemanticError::SemanticErrorKind code)
    {
        codes.Push(code);
        SemanticError::SetWarningLevel(code, level);
    }

    const char* name;
    const char* reason;
    SemanticError::WarningLevel level;
    Tuple<SemanticError::SemanticErrorKind> codes;
};


static Tuple<MessageGroup*> message_groups;


//
// HOWTO: Add a +Pno-<something> flag to selectively enable/disable a warning.
//
// 1. Push a MessageGroup instance to message_groups. The WarningLevel will
//    be the default for all the members of the group.
//
// 2. Add the SemanticErrorKind enum values for the warning(s) that should be
//    in the group.
//
// 3. Update the documentation in docs/jikes.1 to reflect the new option.
//
void SemanticError::InitializeMessageGroups()
{
    MessageGroup* group;

    group = new MessageGroup("modifier-order",
                             "modifiers appearing out of order",
                             NAMED_WEAK_OFF);
    group -> AddMessage(RECOMMENDED_MODIFIER_ORDER);
    group -> AddMessage(RECOMMENDED_ANNOTATION_ORDER);
    message_groups.Push(group);

    group = new MessageGroup("redundant-modifiers",
                             "modifiers which are implied",
                             NAMED_WEAK_OFF);
    group -> AddMessage(REDUNDANT_MODIFIER);
    message_groups.Push(group);

    group = new MessageGroup("serial",
                             "serialization warnings",
                             NAMED_WEAK_OFF);
    group -> AddMessage(BAD_SERIAL_VERSION_UID);
    group -> AddMessage(EJ_SERIALIZABLE_INNER_CLASS);
    group -> AddMessage(MISSING_SERIAL_VERSION_UID);
    group -> AddMessage(UNNEEDED_SERIAL_VERSION_UID);
    message_groups.Push(group);

    group = new MessageGroup("shadow", "shadowed and hidden fields",
                             NAMED_WEAK_ON);
    group -> AddMessage(HIDDEN_FIELD);
    group -> AddMessage(LOCAL_SHADOWS_FIELD);
    message_groups.Push(group);

    group = new MessageGroup("switchcheck",
                             "fallthrough between switch statement cases",
                             NAMED_WEAK_ON);
    group -> AddMessage(SWITCH_FALLTHROUGH);
    message_groups.Push(group);

    //
    // Naming convention warnings.
    //
    group = new MessageGroup("naming-convention",
                             "names which differ from standard convention",
                             NAMED_WEAK_OFF);
    group -> AddMessage(UNCONVENTIONAL_CLASS_NAME);
    group -> AddMessage(UNCONVENTIONAL_CONSTANT_FIELD_NAME);
    group -> AddMessage(UNCONVENTIONAL_FIELD_NAME);
    group -> AddMessage(UNCONVENTIONAL_METHOD_NAME);
    group -> AddMessage(UNCONVENTIONAL_VARIABLE_NAME);
    message_groups.Push(group);

    //
    // Warnings from Bloch's "Effective Java".
    //
    group = new MessageGroup("effective-java",
                             "practices warned about in \"Effective Java\"",
                             NAMED_WEAK_OFF);
    group -> AddMessage(EJ_AVOID_OVERLOADING_EQUALS);
    group -> AddMessage(EJ_EMPTY_CATCH_BLOCK);
    group -> AddMessage(EJ_EMPTY_FINALLY_BLOCK);
    group -> AddMessage(EJ_EQUALS_WITHOUT_HASH_CODE);
    group -> AddMessage(EJ_HASH_CODE_WITHOUT_EQUALS);
    group -> AddMessage(EJ_INTERFACE_DOES_NOT_DEFINE_TYPE);
    group -> AddMessage(EJ_MISSING_PRIVATE_CONSTRUCTOR);
    group -> AddMessage(EJ_OVERLY_GENERAL_THROWS_CLAUSE);
    group -> AddMessage(EJ_PUBLIC_STATIC_FINAL_ARRAY_FIELD);
    group -> AddMessage(EJ_RETURN_OF_NULL_ARRAY);
    message_groups.Push(group);

    //
    // Warnings about unused imports
    //
    group = new MessageGroup("unused-type-imports",
                             "unused single-type import statements",
                             NAMED_WEAK_OFF);
    group -> AddMessage(UNUSED_TYPE_IMPORT);
    message_groups.Push(group);

    group = new MessageGroup("unused-package-imports",
                             "unused package import statements",
                             NAMED_WEAK_OFF);
    group -> AddMessage(UNUSED_PACKAGE_IMPORT);
    message_groups.Push(group);
}

//
// Cleans up static message groups to avoid memory leaks.
// Should be called at program shutdown.
//
void SemanticError::CleanupMessageGroups()
{
    for (unsigned i = 0; i < message_groups.Length(); i++)
        delete message_groups[i];
    message_groups.Resize(0);
}

//
// Outputs information about the warnings that can be disabled on the
// command-line. Called by the code that handles the -help option.
//
void SemanticError::PrintNamedWarnings()
{
    static const char* LEADING = "                      ";
    StaticInitializer();
    for (unsigned i = 0; i < message_groups.Length(); ++i)
    {
        MessageGroup* group = message_groups[i];

        static const unsigned SPACE_FOR_NAME = 15;
        printf("+P[no-]%-*s", SPACE_FOR_NAME, group -> name);
        if (strlen(group -> name) >= SPACE_FOR_NAME)
            printf("\n%s", LEADING);
        printf("warn about %s\n", group -> reason);
        if (group -> level == NAMED_WEAK_ON ||
            group -> level == NAMED_STRONG_ON)
        {
            printf("%s(on by default)\n", LEADING);
        }
    }
}

//
// Turns all named warnings not previously initialized to their plain +P
// default level, using NamedError::level (see documentation of named_errors).
//
void SemanticError::EnableDefaultWarnings()
{
    StaticInitializer();
    for (unsigned g = 0; g < message_groups.Length(); ++g)
    {
        MessageGroup* group = message_groups[g];

        assert(group -> level > DISABLED); // Is this a named warning?
        for (unsigned c = 0; c < group -> codes.Length(); ++c)
        {
            SemanticErrorKind kind = group -> codes[c];
            if (warning[kind] > DISABLED)
            {
                warning[kind] = group -> level;
            }
        }
    }
}

//
// Processes a command-line option enabling or disabling a warning.
// Returns true if the switch was recognized, false otherwise.
//
// Command-line options are of the form +P<name> or +Pno-<name> to
// enable or disable the warning <name> respectively.
//
// The 'image' parameter should not include the "+P" prefix (partly because
// synonyms like -Xswitchcheck also use this method).
//
bool SemanticError::ProcessWarningSwitch(const char* image)
{
    StaticInitializer();

    // Is this +P<name> or +Pno-<name>?
    bool enable = true;
    if (strncmp(image, "no-", 3) == 0)
    {
        image += 3;
        enable = false;
    }

    // +P[no-]all turns everything on/off.
    bool override = (strcmp(image, "all") == 0);

    bool switch_recognized = false;
    for (unsigned g = 0; g < message_groups.Length(); ++g)
    {
        MessageGroup* group = message_groups[g];
        if (override || strcmp(group -> name, image) == 0)
        {
            switch_recognized = true;
            for (unsigned c = 0; c < group -> codes.Length(); ++c)
            {
                SemanticErrorKind kind = group -> codes[c];
                switch(group -> level)
                {
                    case NAMED_STRONG_ON:
                    case NAMED_STRONG_OFF:
                        warning[kind] = enable ? STRONG_WARNING : DISABLED;
                        break;
                    case NAMED_WEAK_ON:
                    case NAMED_WEAK_OFF:
                        warning[kind] = enable ? WEAK_WARNING : DISABLED;
                        break;
                    default:
                        assert(false && "Invalid default level for named warning");
                }
            }
        }
    }
    return switch_recognized;
}

//
// This procedure uses a  quick sort algorithm to sort the ERRORS
// by the left_line_no and left_column_no fields.
//
void SemanticError::SortMessages()
{
     int lower,
         upper,
         lostack[32],
         histack[32];

     int top,
         i,
         j;
     ErrorInfo pivot,
               temp;

     top = 0;
     lostack[top] = 0;
     histack[top] = error.Length() - 1;

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
             pivot = error[i];
             error[i] = error[lower];

             //
             // Split the array section indicated by LOWER and UPPER
             // using ARRAY(LOWER) as the pivot.
             //
             i = lower;
             for (j = lower + 1; j <= upper; j++)
                 if ((error[j].left_token < pivot.left_token) ||
                 //
                 // When two error messages start in the same location
                 // and one is nested inside the other, the outer one
                 // is placed first so that it can be printed last.
                 // Recall that its right-span location is reached
                 // after the inner one has been completely processed.
                 //
                     (error[j].left_token == pivot.left_token &&
                      error[j].right_token > pivot.right_token) ||
                 //
                 // When two error messages are at the same location
                 // span, check the NUM field to keep the sort stable.
                 // When the location spans only a single symbol,
                 // the one with the lowest "num" is placed first.
                 //
                     (error[j].left_token  == pivot.left_token  &&
                      error[j].right_token == pivot.right_token &&
                      pivot.left_token == pivot.right_token     &&
                      error[j].num < pivot.num)                       ||
                 //
                 // When two error messages are at the same location
                 // which spans more than one symbol in the source,
                 // the first message is treated as being nested into
                 // the second message and (just like the nested case
                 // above) it is placed last in the sorted sequence.
                 //
                     (error[j].left_token  == pivot.left_token  &&
                      error[j].right_token == pivot.right_token &&
                      pivot.left_token < pivot.right_token      &&
                      error[j].num > pivot.num))
                 {
                     temp = error[++i];
                     error[i] = error[j];
                     error[j] = temp;
                 }
             error[lower] = error[i];
             error[i] = pivot;

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
// This is the local private procedure that prints the semantic error messages.
//
int SemanticError::PrintMessages()
{
    int return_code = (num_errors > 0 ? 1 : 0);

    //
    // If the errors were already dumped, return.
    //
    if (control.option.dump_errors)
        return return_code;
    if (control.option.errors) // regular error messages
    {
        if (num_errors == 0)
        {
            if (control.option.tolerance == JopaOption::NO_WARNINGS)
                // we only had warnings and they should not be reported
                return return_code;

            Coutput << endl << "Issued " << num_warnings
                    << (lex_stream -> file_symbol -> semantic ==
                        control.system_semantic ? " system" : " semantic")
                    << " warning" << (num_warnings <= 1 ? "" : "s");
        }
        else // we had some errors, and possibly warnings as well
        {
            Coutput << endl << "Found " << num_errors
                    << (lex_stream -> file_symbol -> semantic ==
                        control.system_semantic ? " system" : " semantic")
                    << " error" << (num_errors <= 1 ? "" : "s");
            if (num_warnings > 0 &&
                control.option.tolerance != JopaOption::NO_WARNINGS)
            {
                Coutput << " and issued " << num_warnings
                        << " warning" << (num_warnings <= 1 ? "" : "s");
            }
        }

        if (lex_stream -> file_symbol -> semantic !=
            control.system_semantic)
        {
            Coutput << " compiling \"" << lex_stream -> FileName() << '\"';
        }
        Coutput << ':';
    }

    //
    // Reopen the file to report the errors, unless we didn't parse it in the
    // first place.
    //
    if (lex_stream -> file_symbol -> semantic != control.system_semantic)
    {
        lex_stream -> RereadInput();

        if (! lex_stream -> InputBuffer())
        {
            char *file_name = lex_stream -> FileName();
            int length = lex_stream -> FileNameLength();
            wchar_t *name = new wchar_t[length + 1];
            for (int i = 0; i < length; i++)
                name[i] = file_name[i];
            name[length] = U_NULL;
            control.system_semantic ->
                ReportSemError(SemanticError::CANNOT_REOPEN_FILE,
                               BAD_TOKEN, name);
            delete [] name;
        }
    }

    if (lex_stream -> file_symbol -> semantic == control.system_semantic ||
        lex_stream -> InputBuffer())
    {
        SortMessages();
        for (unsigned k = 0; k < error.Length(); k++)
        {
            if (warning[error[k].msg_code] != 1 ||
                control.option.tolerance != JopaOption::NO_WARNINGS)
            {
                reportError(k);
            }
        }
        lex_stream -> DestroyInput();
    }

    Coutput.flush();
    return return_code;
}


//
// Returns the insert for the given index. Used to translate from the
// numeric indices used in format strings to the appropriate member variable.
//
const wchar_t* ErrorInfo::getInsert(unsigned index)
{
    assert(index && index <= MAX_INSERTS);
    return insert[index - 1];
}

//
// Formats a classpath for an error message. Used to implement the "%C" verb.
//
static void FormatClasspath(ErrorString& s, Control& control)
{
    s << endl;
    for (unsigned i = 1; i < control.classpath.Length(); i++)
    {
        PathSymbol* path_symbol = control.classpath[i];
        s << "                " << path_symbol -> Name() << endl;
    }
}

//
// Writes a filename into an ErrorString. Replaces the U_SLASH character used
// internally by jikes to the platform's native separator character. Used by
// various format routines.
//
static void PrintFile(ErrorString& s, const wchar_t* filename,
                      const wchar_t slash_char)
{
    if (filename)
    {
        while (*filename)
        {
            wchar_t ch = *filename++;
            s << (ch != U_SLASH ? ch : slash_char);
        }
    }
}

//
// Tests whether 'str' represents a path worth printing (returns true), or is
// either empty or the current directory (returns false).
//
static bool NotDot(const wchar_t* str)
{
    return str && *str && *str != U_DO && str[1] != U_NU;
}

//
// Formats a filename, using both the path and leaf parts (from successive
// inserts). Used to implement the "%F" verb.
//
static void FormatFile(ErrorInfo& err, ErrorString& s, const char insert_char,
                       const wchar_t slash_char)
{
    assert(isdigit(insert_char));
    int index = insert_char - '0';

    const wchar_t* first_insert = err.getInsert(index);
    if (NotDot(first_insert))
    {
        PrintFile(s, first_insert, slash_char);
        s << slash_char;
    }
    PrintFile(s, err.getInsert(index + 1), slash_char);
}

//
// Formats a package name. If no name is available, the package is described
// as the unnamed package. Otherwise, the package name is used, with dots
// separating the components, as in Java source (java.lang.ref, say).
// Used to implement the "%P" verb.
//
static void FormatPackage(ErrorInfo& err, ErrorString& s,
                          const char insert_char)
{
    assert(isdigit(insert_char));
    int index = insert_char - '0';

    const wchar_t* insert = err.getInsert(index);
    if (! insert || *insert == U_NULL)
        s << L"the unnamed package";
    else
    {
        s << L"package \"";
        PrintFile(s, insert, U_DOT);
        s << L"\"";
    }
}

//
// If the given insert is non-NULL, writes " or this.<insert-text>" to the
// ErrorString. Used to implement the "%Q" verb. Try searching for where it's
// used to see an example.
//
static void FormatOptionalThisQualifiedName(ErrorInfo& err, ErrorString& s,
                                            char insert_char)
{
    assert(isdigit(insert_char));
    int index = insert_char - '0';

    const wchar_t* insert = err.getInsert(index);
    if (insert && *insert)
        s << L" or this." << insert;
}

//
// Formats an error. The format string corresponding to the error code is
// looked up, then interpreted in a style reminiscent of printf(3). For
// information about the verbs available, see the HOWTO later on.
//
void SemanticError::FormatError(ErrorInfo& err)
{
    const char* format_string = messages[err.msg_code];
    const char* p = format_string;
    ErrorString s;
    while (*p)
    {
        char ch = *p++;
        if (ch != '%')
            s << ch;
        else
        {
            char verb = *p++;
            if (isdigit(verb))
            {
                s << err.getInsert(verb - '0');
                continue;
            }
            switch (verb)
            {
            case 'C':
                FormatClasspath(s, control);
                break;
            case 'F': // A .class or .java file name
                FormatFile(err, s, *p++,
                           U_SLASH
                           );
                break;
            case 'L':
                s << lex_stream -> FileName();
                break;
            case 'P':
                FormatPackage(err, s, *p++);
                break;
            case 'Q':
                FormatOptionalThisQualifiedName(err, s, *p++);
                break;
            case 'T': // A (qualified) type name
                FormatFile(err, s, *p++, U_DOT);
                break;
            case 'Z':
                s << err.msg_code;
                break;
            case '%':
                s << '%';
                break;
            default:
                assert(0 && "unknown verb in error format string");
            }
        }
    }
    err.msg = s.Array();
}


void SemanticError::reportError(int k)
{
    FormatError(error[k]);
    error[k].Initialize(lex_stream);
    JopaAPI::getInstance() -> reportError(&error[k]);
}

//
// HOWTO: Add an error message.
//
// There are two steps:
//   1. Add an enum value to the SemanticErrorKind enum in 'error.h'.
//   2. Assign a format string to the message array here.
//
// The format strings are similar to printf(3) in that anything up to a '%'
// is simply copied verbatim. A '%' is interpreted as a command.
//
// These commands use no inserts and take no parameter:
//   %C  Writes the classpath into the ErrorString.
//   %L  Writes the current lex_stream filename. This is useful if the error
//       needs to refer to this filename because you don't have to pass
//       lex_stream -> FileName() as an insert.
//   %Z  Writes the message code for this error into the ErrorString.
//   %%  Writes a single '%' character into the ErrorString.
//
// These commands use a single insert (where <n> is replaced with an insert
// number):
//   %1 %2 %3 %4 %5 %6 %7 %8 %9
//       Writes an insert into the ErrorString. %1 uses insert1, %2 uses
//       insert2, and so on.
//   %Pn Writes a package name. If insert<n> is NULL, the package is described
//       as the unnamed package. Otherwise the insert is used, with Jikes'
//       internal separators replaced with dots so "java/lang/ref" will appear
//       as "java.lang.ref".
//   %Qn Writes an optional this-qualified name. The name is written if
//       insert<n> is non-NULL, otherwise nothing is written. The text written
//       is of the form " or this.<insert-text>".
//
// These commands use two successive inserts (where <n> is replaced with an
// insert number and refers implicitly to two inserts):
//   %Fn Writes the given filename. This uses two inserts, insert<n> and
//       insert<n+1>. The former should contain the pathname, the latter
//       the leafname. The two will be stitched together (if the pathname
//       is worth printing) and Jikes' internal separators will be replaced
//       with the platform's native separator character (which for Unix
//       is the same, '/'). %F is typically used for a .class or .java
//       filename.
//   %Tn Writes a (possibly qualified) type name. This uses two inserts,
//       insert<n> and insert<n+1>. The former should contain the package
//       name, the latter the unqualified type name. The two will be stitched
//       together (if the package name is worth printing) and Jikes' internal
//       separators will be replaced with dots, so "java/lang", "String" will
//       appear as "java.lang.String".
// TODO: %Tn should print nested classes as A.B instead of A$B, anonymous
//   classes as Bar.<anon Foo> instead of Bar$1, and local classes as
//   Bar.m().Local instead of Bar$1Local.
//
// It is possible, of course, to only ever use the numeric verbs, %1 and
// friends.  The disadvantage of this is that you don't make your intent clear
// to Jikes, so it can't help you in any way. As the descriptions above show,
// if you tell Jikes you're outputting a filename, say, it'll make sure it
// looks like a filename.
//
// If you find yourself writing repetitive code to support a group of errors,
// consider instead adding a new verb to SemanticError::FormatError.
//
void SemanticError::InitializeMessages()
{
    for (int i = 0; i < _num_kinds; i++)
        messages[i] = NULL;
    messages[BAD_ERROR] =
        "chaos: Error code %Z is not a valid error message code.";
    messages[DEFAULT_ERROR] = "%1%2%3%4%5%6%7%8%9";

    // File related errors.
    messages[NO_CURRENT_DIRECTORY] = "Could not open current directory.";
    messages[CANNOT_OPEN_ZIP_FILE] =
        "The file \"%1\" does not exist or else is not a valid zip file.";
    messages[CANNOT_OPEN_PATH_DIRECTORY] =
        "The file \"%1\" is not a valid directory.";
    messages[PACKAGE_NOT_FOUND] =
        "You need to modify your classpath, sourcepath, bootclasspath, "
        "and/or extdirs setup. Jikes could not find %P1 in: %C";
    messages[CANNOT_OPEN_DIRECTORY] = "Unable to open directory \"%1\".";
    messages[BAD_INPUT_FILE] =
        "The input file \"%1\" does not have the \".java\" extension.";
    messages[UNREADABLE_INPUT_FILE] = "The input file \"%1\" was not found.";
    messages[NON_STANDARD_LIBRARY_TYPE] =
        "A non-standard version of the type \"%T1\" "
        "was found. Class files that depend on this type may not "
        "have been generated.";
    messages[LIBRARY_METHOD_NOT_FOUND] =
        "A class file was not generated for the type \"%T1\" "
        "because a library method that it depends on was not found. "
        "See system messages for more information.";
    messages[CANNOT_REOPEN_FILE] = "Unable to reopen file \"%1\".";
    messages[CANNOT_WRITE_FILE] = "Unable to write file \"%1\".";
    messages[ASSERT_UNSUPPORTED_IN_TARGET] =
        "The type \"%T1\" requires support for assert statements not present "
        "in your choice of -target. Either use \"-target 1.4\" or greater, or "
        "use \"--noassert\" (or \"+a\") at the command line.";
    messages[CONSTANT_POOL_OVERFLOW] =
        "The type \"%T1\" produced a constant pool that exceeded "
        "the limit of 65535 elements.";
    messages[INTERFACES_OVERFLOW] =
        "The type \"%T1\" directly implemented more than the limit "
        "of 65535 interfaces.";
    messages[METHODS_OVERFLOW] =
        "The type \"%T1\" declared more than the limit of 65535 methods.";
    messages[STRING_OVERFLOW] =
        "The type \"%T1\" generated one or more strings whose length "
        "exceeds the maximum length of 65535 bytes when encoded in Utf8. "
        "Lengthy identifiers, method signatures, and string literals can "
        "all cause this problem.";
    messages[PARAMETER_OVERFLOW] =
        "Method \"%1\" in type \"%T2\" "
        "contained more than the limit of 255 formal parameters. Note "
        "that a parameter of type long or double counts as 2 parameters.";
    messages[ARRAY_OVERFLOW] =
        "The number of dimensions in an array is limited to 255.";
    messages[FIELDS_OVERFLOW] =
        "The type \"%T1\" declared more than the limit of 65535 fields.";
    messages[LOCAL_VARIABLES_OVERFLOW] =
        "Method \"%1\" in type \"%T2\" contained more than the limit "
        "of 65535 local variables.";
    messages[STACK_OVERFLOW] =
        "Method \"%1\" in type \"%T2\" required a stack that exceeds "
        "the limit of 65535 positions.";
    messages[CODE_OVERFLOW] =
        "Method \"%1\" in type \"%T2\" produced a code attribute that "
        "exceeds the code limit of 65535 elements.";
    messages[COMPRESSED_ZIP_FILE] =
        "The file %1(%F2) is in an unsupported compressed format. "
        "(Unzip and) Rezip \"%1\".";
    messages[INVALID_CLASS_FILE] =
        "The class file \"%1.class\" in \"%F2\" has an invalid format (%4).";
    messages[CANNOT_OPEN_CLASS_FILE] =
        "Unable to open file associated with type \"%T1\".";
    messages[IO_ERROR] = "I/O error: %1.";
    messages[IO_WARNING] = "I/O warning: %1.";

    // Warnings and pedantic errors.
    messages[NEGATIVE_ARRAY_SIZE] =
        "Array initialization will fail with a negative dimension.";
    messages[NEGATIVE_SHIFT_COUNT] =
        "The shift count %1 is negative; it will be masked to the "
        "appropriate width and behave as a positive shift count.";
    messages[SHIFT_COUNT_TOO_LARGE] =
        "The shift count of %1 is >= the %2-bit width of the type.";
    messages[UNNECESSARY_PARENTHESIS] =
        "Parenthesis surrounding a variable are syntactically unnecessary. "
        "While legal now, they were illegal in previous versions of Java.";
    messages[EMPTY_DECLARATION] =
        "An EmptyDeclaration is useless. \";\" ignored.";
    messages[REDUNDANT_MODIFIER] =
        "The use of the \"%1\" modifier in this context "
        "is redundant and is discouraged as a matter of style.";
    messages[RECOMMENDED_MODIFIER_ORDER] =
        "The modifier \"%1\" did not appear in the recommended order: "
        "public/protected/private, abstract, static, final, synchronized, "
        "transient, volatile, native, strictfp.";
    messages[SWITCH_FALLTHROUGH] =
        "This switch block can fall through to the next case. Did you forget "
        "a break statement?";
    messages[OBSOLESCENT_BRACKETS] =
        "The use of empty bracket pairs following a MethodDeclarator should "
        "not be used in new Java programs.";
    messages[NO_TYPES] = "This compilation unit contains no type declaration.";
    messages[MULTIPLE_PUBLIC_TYPES] =
        "The type \"%1\" is declared public in compilation unit \"%L\" "
        "which also contains the public type, \"%2\".";
    messages[TYPE_IN_MULTIPLE_FILES] =
        "The file \"%F1.java\" contains type \"%4\" which conflicts with "
        "file \"%F3.java\".";
    messages[PACKAGE_TYPE_CONFLICT] =
        "The type \"%T1\" contained in file \"%3\" conflicts with the "
        "package \"%T1\".";
    messages[FILE_FILE_CONFLICT] =
        "Cannot write class file \"%1.class\" because that name conflicts "
        "with the class file \"%2\" in directory \"%F3\". This is illegal "
        "because file names are case-insensitive in this system.";
    messages[MISMATCHED_TYPE_AND_FILE_NAMES] =
        "The public type \"%1\" does not match the name of its containing "
        "file \"%L\".";
    messages[REFERENCE_TO_TYPE_IN_MISMATCHED_FILE] =
        "The type \"%1\" is defined in the file \"%2.java\" but referenced "
        "in the file \"%L\". It is recommended that it be redefined "
        "in \"%1.java\".";
    messages[ZERO_DIVIDE_CAUTION] =
        "Integer division will fail with division by zero.";
    messages[VOID_TO_STRING] =
        "Attempt to convert a void expression into java.lang.String.";
    messages[CLASS_METHOD_INVOKED_VIA_INSTANCE] =
        "Invoking the class method \"%1\" via an instance is discouraged "
        "because the method invoked will be the one in the variable's "
        "declared type, not the instance's dynamic type.";
    messages[CLASS_FIELD_ACCESSED_VIA_INSTANCE] =
        "Accessing the class field \"%1\" via an instance is discouraged "
        "because the field accessed will be the one in the variable's "
        "declared type, not the instance's dynamic type.";
    messages[CONSTANT_OVERFLOW] =
        "Overflow in %1 expression.";
    messages[LOCAL_SHADOWS_FIELD] =
        "Local \"%1\" shadows a field of the same name in \"%T2\".";
    messages[HIDDEN_FIELD] =
        "Field \"%1\" shadows a field of the same name in \"%T2\".";
    messages[ASSIGNMENT_USED_AS_TRUTH_VALUE] =
        "Suggest parentheses around assignment used as truth value.";
    messages[NON_STATIC_FINAL_CONSTANT_FIELD] =
        "Final field \"%1\" is initialized with a constant expression and "
        "could be made static to save space.";
    messages[AMBIGUOUS_NULL_VARARG] =
        "Cast to an array to demonstrate that you really want a null "
        "array rather than an array of length 1 containing a null value.";

    // "Effective Java" warnings.
    messages[EJ_AVOID_OVERLOADING_EQUALS] =
        "The class \"%1\" has an \"equals\" method with parameter of a type "
        "other than \"java.lang.Object\". This will overload, rather than "
        "override, \"java.lang.Object.equals\". "
        "(See item 17 of \"Effective Java\".)";
    messages[EJ_EMPTY_CATCH_BLOCK] =
        "An empty catch block defeats the purpose of exceptions. "
        "(See item 47 of \"Effective Java\".)";
    messages[EJ_EMPTY_FINALLY_BLOCK] =
        "An empty finally block is unnecessary and misleading.";
    messages[EJ_EQUALS_WITHOUT_HASH_CODE] =
        "The class \"%1\" overrides \"equals\" without overriding "
        "\"hashCode\". "
        "Equal objects must return equal values from \"hashCode\", so always "
        "override \"hashCode\" when you override \"equals\". "
        "(See item 8 of \"Effective Java\".)";
    messages[EJ_HASH_CODE_WITHOUT_EQUALS] =
        "The class \"%1\" overrides \"hashCode\" without overriding "
        "\"equals\". "
        "Equal objects must return equal values from \"hashCode\", and "
        "though this may still be the case here, overriding \"hashCode\" "
        "without overriding \"equals\" is usually a mistake. "
        "(See item 8 of \"Effective Java\".)";
    messages[EJ_INTERFACE_DOES_NOT_DEFINE_TYPE] =
        "An interface should define a type with behavior. "
        "Should \"%1\" have been an enum or a noninstantiable utility class? "
        "(See item 7 of \"Effective Java\".)";
    messages[EJ_MISSING_PRIVATE_CONSTRUCTOR] =
        "A private constructor would enforce the noninstantiability of \"%1\". "
        "(See item 3 of \"Effective Java\".)";
    messages[EJ_OVERLY_GENERAL_THROWS_CLAUSE] =
        "An overly-general throws clause obscures which exceptions may "
        "actually be thrown. "
        "(See item 44 of \"Effective Java\".)";
    messages[EJ_PUBLIC_STATIC_FINAL_ARRAY_FIELD] =
        "The field \"%1\" can be modified by clients. It is nearly always "
        "wrong to have a public static final array field. The public array "
        "should be replaced by a private array and a public immutable "
        "\"java.util.List\". "
        "(See item 12 of \"Effective Java\".)";
    messages[EJ_RETURN_OF_NULL_ARRAY] =
        "Return a zero-length array instead of null. This avoids the need "
        "for special-case code in the caller. "
        "(See item 27 of \"Effective Java\".)";
    messages[EJ_SERIALIZABLE_INNER_CLASS] =
        "The default serialized form of an inner class is ill-defined; "
        "inner classes should rarely, if ever, implement Serializable. "
        "(See item 54 of \"Effective Java\".)";

    // serialVersionUID warnings.
    messages[UNNEEDED_SERIAL_VERSION_UID] =
        "serialVersionUID is only needed in classes that implement "
        "\"java.io.Serializable\".";
    messages[BAD_SERIAL_VERSION_UID] =
        "serialVersionUID should be a private static final long field.";
    messages[MISSING_SERIAL_VERSION_UID] =
        "It is strongly recommended that all serializable classes "
        "explicitly declare serialVersionUID, since the default computation "
        "can result in unexpected InvalidClassExceptions during "
        "deserialization.";

    // Naming convention warnings.
    messages[UNCONVENTIONAL_CLASS_NAME] =
        "Use names ThatLookLikeThis for classes such as \"%1\". "
        "(See item 38 of \"Effective Java\".)";
    messages[UNCONVENTIONAL_CONSTANT_FIELD_NAME] =
        "Use names THAT_LOOK_LIKE_THIS for final fields such as \"%1\". "
        "(See item 38 of \"Effective Java\".)";
    messages[UNCONVENTIONAL_FIELD_NAME] =
        "Use names thatLookLikeThis for fields such as \"%1\". "
        "(See item 38 of \"Effective Java\".)";
    messages[UNCONVENTIONAL_METHOD_NAME] =
        "Use names thatLookLikeThis for methods such as \"%1\". "
        "(See item 38 of \"Effective Java\".)";
    messages[UNCONVENTIONAL_VARIABLE_NAME] =
        "Use names thatLookLikeThis for variables such as \"%1\". "
        "(See item 38 of \"Effective Java\".)";

    // Type and package related errors.
    messages[DUPLICATE_INNER_TYPE_NAME] =
        "The nested type name \"%1\" is illegal, as it is enclosed in "
        "another class with the same simple name at location %2.";
    messages[DUPLICATE_TYPE_DECLARATION] =
        "Duplicate declaration of type \"%1\". The other occurrence is "
        "at location %2.";
    messages[DUPLICATE_IMPORT_NAME] =
        "The imported simple name \"%1\" names a different type than the "
        "other use of the name at location %2.";
    messages[UNNECESSARY_TYPE_IMPORT] =
        "Unnecessary import of type \"%1\". The type is declared at "
        "location %2.";
    messages[UNUSED_TYPE_IMPORT] =
        "Unnecessary import of type \"%T1\". The type is never referenced.";
    messages[UNUSED_PACKAGE_IMPORT] =
        "Unnecessary import of %P1. No types of this package are "
        "referenced.";
    messages[DUPLICATE_ACCESS_MODIFIER] =
        "Duplicate specification of an access modifier. "
        "Only one instance of \"public\", \"private\", or \"protected\" "
        "may appear in a declaration.";
    messages[DUPLICATE_MODIFIER] =
        "Duplicate specification of the modifier \"%1\".";
    messages[FINAL_ABSTRACT_ENTITY] =
        "It is not possible for %1 to be both \"final\" and \"abstract\".";
    messages[VOLATILE_FINAL_FIELD] =
        "A field may not be both \"volatile\" and \"final\".";
    messages[INVALID_MODIFIER] =
        "\"%1\" is not a valid modifier for %2.";
    messages[RECOMPILATION] =
        "The type associated with this construct depends on file "
        "%F1.class which, in turn, depends on file %F3.java. "
        "All files that depend on this source file, in particular, "
        "%F1.java should be recompiled.";
    messages[PACKAGE_NOT_TYPE] =
        "Found %P1 when a type was expected.";
    messages[TYPE_NOT_FOUND] =
        "Type \"%T1\" was not found.";
    messages[INVALID_TYPE_FOUND] =
        "A candidate for type \"%1\" was found, but it is invalid and needs "
        "to be fixed before this type will successfully compile.";
    messages[IMPORT_FROM_UNNAMED_PACKAGE] =
        "Type \"%1\" exists in the unnamed package, and "
        "cannot be imported. Consider putting it into a named package.";
    messages[DUPLICATE_ON_DEMAND_IMPORT] =
        "Type \"%1\" is imported on demand from %P2 and %P3.";
    messages[UNKNOWN_ON_DEMAND_IMPORT] =
        "The import \"%1\" is not valid, since it does not name a type "
        "in a package.";
    messages[IMPORT_NOT_CANONICAL] =
        "The import for nested type \"%1\" is not valid, since it does "
        "not use the canonical name \"%T2\".";
    messages[NOT_A_TYPE] = "A type is expected here.";
    messages[NOT_A_CLASS] =
        "Interface \"%T1\" cannot be used where a class is expected.";
    messages[NOT_AN_INTERFACE] =
        "Class \"%T1\" cannot be used where an interface is expected.";
    messages[SUPER_IS_FINAL] =
        "The super class \"%T1\" is final, and cannot have subclasses.";
    messages[OBJECT_WITH_SUPER_TYPE] =
        "The type \"java.lang.Object\" must not have an extends or implements "
        "clause, as it has no supertype.";
    messages[OBJECT_HAS_NO_SUPER_TYPE] =
        "The type \"java.lang.Object\" does not have a supertype.";
    messages[DUPLICATE_FIELD] =
        "Duplicate declaration of field \"%1\" in type \"%2\". The other "
        "occurrence is at location \"%3\".";
    messages[DUPLICATE_METHOD] =
        "Duplicate declaration of method \"%1\" in type \"%2\". The other "
        "occurrence is at location \"%3\".";
    messages[DUPLICATE_CONSTRUCTOR] =
        "Duplicate declaration of this constructor signature in type \"%1\". "
        "The other occurrence is at location \"%2\".";
    messages[MISMATCHED_INHERITED_METHOD] =
        "The return type of method \"%1\" does not match the return type of "
        "the accessible method \"%2\" declared in type \"%T3\".";
    messages[MISMATCHED_IMPLICIT_METHOD] =
        "The return type of method \"%1\" does not match the return type of "
        "method \"%2\" declared implicitly for interfaces.";
    messages[UNIMPLEMENTABLE_INTERFACE] =
        "Interface \"%T1\" is legal, but cannot be implemented: "
        "method \"%3\" has a different return type than \"%4\" declared "
        "in java.lang.Object.";
    messages[UNIMPLEMENTABLE_CLASS] =
        "Class \"%T1\" cannot be implemented: method \"%3\" declared "
        "in \"%T4\" has a different return type than the non-inherited "
        "default access abstract method \"%6\" declared in the superclass "
        "\"%T7\".";
    messages[MISMATCHED_INHERITED_METHOD_EXTERNALLY] =
        "In type \"%1\", the method \"%2\", inherited from type \"%T3\", "
        "does not have the same return type as the method \"%5\", "
        "inherited from type \"%T6.";
    messages[DUPLICATE_FORMAL_PARAMETER] =
        "Duplicate declaration of formal parameter \"%1\".";
    messages[MISSPELLED_CONSTRUCTOR_NAME] =
        "The name of the constructor \"%1\" does not match the name of "
        "the class \"%2\". Assuming it is misspelled.";
    messages[MISMATCHED_CONSTRUCTOR_NAME] =
        "The name of the constructor \"%1\" does not match the name of "
        "the class \"%2\". Assuming it is a method with missing return type.";
    messages[METHOD_WITH_CONSTRUCTOR_NAME] =
        "The name of this method \"%1\" matches the name of the containing "
        "class. However, the method is not a constructor since its declarator "
        "is qualified with a type.";

    // Statement and expression related errors.
    messages[DUPLICATE_LOCAL_VARIABLE_DECLARATION] =
        "Duplicate declaration of local variable \"%1\". The other occurrence "
        "is at location \"%2\".";
    messages[MULTIPLE_DEFAULT_LABEL] =
        "Multiple specification of default label in switch statement.";
    messages[UNDECLARED_LABEL] =
        "\"%1\" is an undeclared label.";
    messages[DUPLICATE_LABEL] =
        "Duplicate declaration of label \"%1\".";
    messages[AMBIGUOUS_FIELD] =
        "Ambiguous access of field \"%1\". At least two fields are "
        "accessible from here: one declared in type \"%T2\" and "
        "one declared in type \"%T4\".";
    messages[AMBIGUOUS_TYPE] =
        "Ambiguous use of type name \"%1\". At least two member types "
        "are accessible from here: one declared in type \"%T2\" "
        "and one declared in type \"%T4\".";
    messages[FIELD_NOT_FOUND] =
        "No accessible field named \"%1\" was found in type \"%T2\".";
    messages[FIELD_NAME_MISSPELLED] =
        "No field named \"%1\" was found in type \"%T2\". However, there is "
        "an accessible field \"%4\" whose name closely matches the name "
        "\"%1\".";
    messages[METHOD_NOT_FIELD] =
        "The name \"%1\" is not a field name but the name of a method "
        "declared in the type \"%T2\".";
    messages[NAME_NOT_YET_AVAILABLE] =
        "Illegal use of name \"%1\" which has not yet been fully declared "
        "at this point.";
    messages[NAME_NOT_CLASS_VARIABLE] =
        "The field \"%1\" is not static, and cannot be accessed in this "
        "static context.";
    messages[NOT_A_VARIABLE] =
        "The left-hand side of an assignment must be a variable.";
    messages[NOT_A_NUMERIC_VARIABLE] =
        "Only a variable of numeric type can appear in this context.";
    messages[METHOD_OVERLOAD_NOT_FOUND] =
        "No applicable overload for a method with signature \"%1\" was found "
        "in type \"%T2\". Perhaps you wanted the overloaded version \"%4\" "
        "instead?";
    messages[METHOD_NOT_FOUND] =
        "No accessible method with signature \"%1\" was found in type "
        "\"%T2\".";
    messages[METHOD_NAME_MISSPELLED] =
        "No method named \"%1\" was found in type \"%T2\". However, there is "
        "an accessible method \"%4\" whose name closely matches the name "
        "\"%1\".";
    messages[HIDDEN_METHOD_IN_ENCLOSING_CLASS] =
        "The method \"%1\" contained in the enclosing type \"%T2\" "
        "is a perfect match for this method call. "
        "However, it is not visible in this nested class because a "
        "method with the same name in an intervening class is hiding it.";
    messages[FIELD_NOT_METHOD] =
        "The name \"%1\" is not a method name but the name of a field "
        "member of the type \"%T2\".";
    messages[TYPE_NOT_METHOD] =
        "The keyword \"new\" is expected before this name, \"%1\", as it "
        "is not the name of a method but the name of a type.";
    messages[TYPE_NOT_FIELD] =
        "A type \"%1\" was found where a field name "
        "or method call was expected. Did you mean to write \"%1.xxx\", "
        "or \"new %1()\", or ... ?";
    messages[METHOD_NOT_CLASS_METHOD] =
        "The method \"%1\" is not static, and cannot be accessed in "
        "this static context.";
    messages[AMBIGUOUS_CONSTRUCTOR_INVOCATION] =
        "Ambiguous invocation of constructor \"%1\". At least two "
        "constructors are accessible from here: \"%2\" and \"%3\".";
    messages[AMBIGUOUS_METHOD_INVOCATION] =
        "Ambiguous invocation of method \"%1\". At least two methods "
        "are accessible from here: \"%2\" declared in type \"%T3\" "
        "and \"%5\" declared in type \"%T6\".";
    messages[CONSTRUCTOR_NOT_FOUND] =
        "No constructor with signature \"%1\" was found in type \"%T2\".";
    messages[METHOD_FOUND_FOR_CONSTRUCTOR] =
        "No match was found for constructor \"%1\". However, a method "
        "with the same name was found at location %2.";
    messages[CONSTRUCTOR_OVERLOAD_NOT_FOUND] =
        "No applicable overload was found for a constructor with signature "
        "\"%1\" in type \"%T2\". Perhaps you wanted the overloaded version "
        "\"%4\" instead?";
    messages[ABSTRACT_TYPE_CREATION] =
        "Attempt to instantiate an abstract class \"%1\".";
    messages[INVALID_INSTANCEOF_CONVERSION] =
        "The type of the left sub-expression, \"%T1\", cannot possibly "
        "be an instance of type \"%T3\".";
    messages[INVALID_CAST_CONVERSION] =
        "An expression of type \"%T1\" cannot be cast into type \"%T3\".";
    messages[INCOMPATIBLE_TYPE_FOR_INITIALIZATION] =
        "The type of the initializer, \"%T3\", is not "
        "assignable to the variable, of type \"%T1\".";
    messages[INCOMPATIBLE_TYPE_FOR_ASSIGNMENT] =
        "The type of the right sub-expression, \"%T3\", is not "
        "assignable to the variable, of type \"%T1\".";
    messages[INCOMPATIBLE_TYPE_FOR_BINARY_EXPRESSION] =
        "The type of the left sub-expression, \"%T1\", is not "
        "compatible with the type of the right sub-expression, \"%T3\".";
    messages[INCOMPATIBLE_TYPE_FOR_CONDITIONAL_EXPRESSION] =
        "In the conditional, the type of the true sub-expression, "
        "\"%T1\", is not compatible with the type of the false "
        "sub-expression, \"%T3\".";
    messages[INCOMPATIBLE_TYPE_FOR_FOREACH] =
        "The component type of the expression, \"%T1\", is not compatible "
        "with the index parameter type, \"%T3\".";
    messages[VOID_ARRAY] = "Arrays of type \"void\" are not legal.";
    messages[DUPLICATE_THROWS_CLAUSE_CLASS] =
        "The duplicate listing of type \"%T1\" in the throws "
        "clause is not necessary.";
    messages[REDUNDANT_THROWS_CLAUSE_CLASS] =
        "The listing of type \"%T1\" in the throws clause is not "
        "necessary, since its superclass, \"%T3\", is also listed.";
    messages[UNCHECKED_THROWS_CLAUSE_CLASS] =
        "Since type \"%T1\" is an unchecked exception, it does not "
        "need to be listed in the throws clause.";
    messages[TYPE_NOT_THROWABLE] =
        "The type \"%T1\" is not a subclass of \"java.lang.Throwable\".";
    messages[TYPE_NOT_INTEGRAL] =
        "The type of this expression, \"%T1\", is not an integral type.";
    messages[TYPE_NOT_NUMERIC] =
        "The type of this expression, \"%T1\", is not numeric.";
    messages[TYPE_NOT_INTEGER] =
        "The type of this expression, \"%T1\", is not assignable to \"int\".";
    messages[TYPE_NOT_STRING] =
        "The type of this expression, \"%T1\", is not \"java.lang.String\".";
    messages[TYPE_NOT_BOOLEAN] =
        "The type of this expression, \"%T1\", is not \"boolean\".";
    messages[TYPE_NOT_ARRAY] =
        "The type of this expression, \"%T1\", is not an array type.";
    messages[TYPE_NOT_REFERENCE] =
        "The type of this expression, \"%1\", is not a reference type.";
    messages[TYPE_NOT_ITERABLE] =
        "The type of this expression, \"%1\", is not an array type nor an "
        "instance of \"java.lang.Iterable\".";
    messages[INVALID_ENUM_SWITCH_CASE] =
        "Case label must be an unqualified constant name of enum type \"%T1\".";
    messages[TYPE_IS_VOID] =
        "An expression of type \"void\" is not valid in this context where "
        "a value is expected.";
    messages[VALUE_NOT_REPRESENTABLE_IN_SWITCH_TYPE] =
        "The value of this expression, %1, cannot be represented in the "
        "type of the switch statement expression, \"%2\".";
    messages[DUPLICATE_CASE_VALUE] =
        "The value of this expression, %1, has already been used in this "
        "switch statement.";
    messages[MISPLACED_THIS_EXPRESSION] =
        "A \"this\" expression may only be used in the body of an instance "
        "method, constructor (after the explicit constructor invocation, if "
        "any), initializer block, or in the initializer expression of an "
        "instance variable.";
    messages[MISPLACED_SUPER_EXPRESSION] =
        "A \"super\" expression may only be used in the body of an instance "
        "method, constructor (after any explicit constructor invocation), "
        "initializer block, or in an instance variable initializer.";

    // Definite assignment related errors.
    messages[VARIABLE_NOT_DEFINITELY_UNASSIGNED] =
        "Possible attempt to reassign a value to the blank final "
        "variable \"%1\".";
    messages[VARIABLE_NOT_DEFINITELY_UNASSIGNED_IN_LOOP] =
        "The blank final variable \"%1\" cannot be assigned within the body "
        "of a loop that may execute more than once.";
    messages[FINAL_VARIABLE_NOT_BLANK] =
        "The final variable \"%1\" is not a blank final in this context, "
        "so it may not be assigned.";
    messages[FINAL_FIELD_ASSIGNMENT_NOT_SIMPLE] =
        "The final field \"%1\" may not be assigned in a qualified "
        "expression. Use a simple name%Q2 instead.";
    messages[UNINITIALIZED_FINAL_VARIABLE] =
        "The blank final field \"%1\" must be initialized in an instance "
        "initializer block or instance field initializer, since this class "
        "has no explicit constructor.";
    messages[UNINITIALIZED_STATIC_FINAL_VARIABLE] =
        "The blank static final field \"%1\" must be initialized in a "
        "static initializer block or static field initializer.";
    messages[UNINITIALIZED_FINAL_VARIABLE_IN_CONSTRUCTOR] =
        "The blank final field \"%1\" must be initialized in this and "
        "every constructor which does not call a form of this(); or else "
        "once in an instance initializer block or instance field initializer.";
    messages[INIT_SCALAR_WITH_ARRAY] =
        "An array initializer cannot be used to initialize a variable of "
        "type \"%1\".";
    messages[INIT_ARRAY_WITH_SCALAR] =
        "A single expression cannot be used to initialize an array variable "
        "of type \"%1\".";
    messages[INVALID_BYTE_VALUE] =
        "A byte value must be an integer value in the range -128..127.";
    messages[INVALID_SHORT_VALUE] =
        "A short value must be an integer value in the range -32768..32767.";
    messages[INVALID_CHARACTER_VALUE] =
        "A character must be in the range 0..65535 ('\\u0000'..'\\uffff').";
    messages[INVALID_INT_VALUE] =
        "The value of an int literal must be a decimal value in the "
        "range -2147483648..2147483647 or a hexadecimal or octal literal "
        "that fits in 32 bits.";
    messages[INVALID_LONG_VALUE] =
        "The value of a long literal must be a decimal value in the range "
        "-9223372036854775808L..9223372036854775807L or a hexadecimal or "
        "octal literal that fits in 64 bits.";
    messages[INVALID_FLOAT_VALUE] =
        "The value of a float literal must not round to infinity or zero.";
    messages[INVALID_DOUBLE_VALUE] =
        "The value of a double literal must not round to infinity or zero.";
    messages[RETURN_STATEMENT_IN_INITIALIZER] =
        "A return statement may not appear in an initializer block.";
    messages[ABRUPT_INITIALIZER] =
        "An initializer block must be able to complete normally.";
    messages[MISPLACED_RETURN_WITH_EXPRESSION] =
        "A return statement with expression must be contained in a method "
        "declaration that is declared to return a value.";
    messages[MISPLACED_RETURN_WITH_NO_EXPRESSION] =
        "A return statement with no expression may only appear in void "
        "method or a constructor.";
    messages[MISMATCHED_RETURN_AND_METHOD_TYPE] =
        "The type of this return expression, \"%T1\", does not match "
        "the return type of the method, \"%T3\".";
    messages[EXPRESSION_NOT_THROWABLE] =
        "The expression in a throw statement must be assignable to the "
        "type \"java.lang.Throwable.\"";
    messages[MISPLACED_BREAK_STATEMENT] =
        "A \"break\" statement must be enclosed in a \"switch\", \"while\", "
        "\"do\" or \"for\" statement.";
    messages[MISPLACED_CONTINUE_STATEMENT] =
        "A \"continue\" statement must be enclosed in a \"while\", \"do\" "
        "or \"for\" statement.";
    messages[MISPLACED_EXPLICIT_CONSTRUCTOR] =
        "Misplaced explicit constructor invocation. It may only be the "
        "first statement in constructors.";
    messages[INVALID_CONTINUE_TARGET] =
        "The statement labeled \"%1\" cannot be continued since it is "
        "not a \"while\", \"do\" or \"for\" statement.";

    // JDK 1.5 (JLS3) feature-related errors.
    messages[HEX_FLOATING_POINT_UNSUPPORTED] =
        "Hexadecimal floating point values are only supported for `-source "
        "1.5' or greater.";
    messages[FOREACH_UNSUPPORTED] =
        "Enhanced for loops (also known as foreach loops) are only supported "
        "for `-source 1.5' or greater.";
    messages[VARARGS_UNSUPPORTED] =
        "Variable-arity methods (also known as varargs) are only supported "
        "for `-source 1.5' or greater.";
    messages[STATIC_IMPORT_UNSUPPORTED] =
        "Static imports are only supported for `-source 1.5' or greater.";
    messages[INVALID_STATIC_IMPORT] =
        "Static import \"%1\" is invalid. Static imports must be qualified "
        "(e.g., \"import static pkg.Type.member;\").";
    messages[STATIC_MEMBER_NOT_FOUND] =
        "Static member \"%1\" not found in type \"%2.%3\".";
    messages[ANNOTATION_MODIFIER_UNSUPPORTED] =
        "Annotation modifiers are only supported for `-source 1.5' or "
        "greater.";
    messages[RECOMMENDED_ANNOTATION_ORDER] =
        "It is recommended that the annotation modifier \"%1\" appear before "
        "all modifier keywords.";
    messages[DUPLICATE_ANNOTATION] =
        "Duplicate specification of the annotation modifier \"%1\".";
    messages[ANNOTATION_TYPE_UNSUPPORTED] =
        "Annotation types are only supported for `-source 1.5' or greater.";
    messages[ENUM_TYPE_UNSUPPORTED] =
        "Enumeration types are only supported for `-source 1.5' or greater.";
    messages[SUPER_IS_ENUM] =
        "Enumeration type \"%T1\" cannot be used as a superclass. Instead, "
        "use the \"enum\" keyword added in `-source 1.5'.";
    messages[CANNOT_CONSTRUCT_ENUM] =
        "An instance of the enumeration type \"%T1\" cannot be created in a "
        "\"new\" expression.";
    messages[TYPE_ARGUMENTS_UNSUPPORTED] =
        "Using type arguments to access generic types requires the use of "
        "\"-source 1.5\" or greater.";
    messages[TYPE_PARAMETERS_UNSUPPORTED] =
        "Declaring type parameters for types or methods requires the use of "
        "\"-source 1.5\" or greater.";
    messages[COVARIANCE_UNSUPPORTED] =
        "Covariant return types require the use of \"-source 1.5\" or "
        "greater. Otherwise, the return type of method \"%1\" must match the "
        "return type of \"%2\" that it overrides.";
    messages[WILDCARD_UNSUPPORTED] =
        "Wildcard type parameters are only supported for \"-source 1.5\" or greater.";
    messages[EXPLICIT_TYPE_ARGUMENTS_UNSUPPORTED] =
        "Explicit type arguments are only supported for \"-source 1.5\" or greater.";
    messages[UNCHECKED_TYPE_CONVERSION] =
        "Conversion of the expression from type \"%T1\" to type \"%T3\" "
        "bypasses type parameterization, and may cause runtime exceptions.";

    // Type parameterization related errors.
    messages[DUPLICATE_TYPE_PARAMETER] =
        "Duplicate declaration of a type parameter \"%1\" in the declaration "
        "of \"%2\".";
    messages[TYPE_PARAMETER_FORWARD_REFERENCE] =
        "Illegal forward reference to parameter \"%1\" in bounds of parameter "
        "\"%2\".";
    messages[TYPE_PARAMETER_IN_MULTIPLE_BOUNDS] =
        "The type parameter \"%1\" cannot be combined with other bounds in "
        "the definition of parameter \"%2\".";
    messages[TYPE_NOT_PARAMETERIZED] =
        "The type \"%T1\" is not parameterized.";
    messages[MISMATCHED_TYPE_PARAMETER_COUNT] =
        "Wrong number of type arguments for type \"%T1\".";
    messages[TYPE_ARGUMENT_FAILS_BOUNDS] =
        "The type \"%T1\" does not satisfy all the bounds required by type "
        "parameter \"%3\".";
    messages[TYPE_PARAMETER_NOT_TYPE] =
        "The type parameter \"%1\" cannot be used where an actual class or "
        "interface is expected.";
    messages[TYPE_MAY_NOT_HAVE_PARAMETERS] =
        "The type \"%T1\" may not be parameterized.";

    // More type-related errors.
    messages[NON_ABSTRACT_TYPE_CONTAINS_ABSTRACT_METHOD] =
        "The abstract method \"%1\" is enclosed in class \"%2\" which "
        "is not abstract.";
    messages[NON_ABSTRACT_TYPE_INHERITS_ABSTRACT_METHOD] =
        "The abstract method \"%1\", inherited from type \"%T2\", is "
        "not implemented in the non-abstract class \"%T4\".";
    messages[NON_ABSTRACT_TYPE_CANNOT_OVERRIDE_DEFAULT_ABSTRACT_METHOD] =
        "The abstract method \"%1\", belonging to the superclass \"%T2\", "
        "has default access, so it is not inherited and cannot be implemented "
        "in this package. Therefore, class \"%T4\" must be abstract.";
    messages[ANONYMOUS_TYPE_CANNOT_OVERRIDE_DEFAULT_ABSTRACT_METHOD] =
        "The abstract method \"%1\", belonging to the class \"%T2\", "
        "has default access, so it is not inherited and "
        "cannot be implemented in this package. Therefore, an anonymous "
        "subclass cannot be created here.";
    messages[DUPLICATE_INTERFACE] =
        "Duplicate specification of interface \"%T1\" "
        "in definition of type \"%3\".";
    messages[UNKNOWN_AMBIGUOUS_NAME] =
        "\"%1\" is either a misplaced package name or a non-existent entity. "
        "An expression name is expected in this context.";
    messages[CIRCULAR_INTERFACE] =
        "The interface \"%T1\" may not have a superinterface which extends "
        "itself, or which is enclosed by itself or any subtype.";
    messages[CIRCULAR_CLASS] =
        "The class \"%T1\" may not have a superclass or superinterface "
        "which extends itself, or which is enclosed by itself or a subclass.";
    messages[TYPE_NOT_ACCESSIBLE] =
        "The type \"%T1\" has %3 access and is not accessible here.";
    messages[FIELD_NOT_ACCESSIBLE] =
        "The field \"%1\" in type \"%T2\" has %4 access and is not "
        "accessible here.";
    messages[PROTECTED_INSTANCE_FIELD_NOT_ACCESSIBLE] =
        "The instance field \"%1\" in class \"%T2\" has protected "
        "access, but the qualifying expression is not of type \"%T4\" "
        "or any of its enclosing types.";
    messages[METHOD_NOT_ACCESSIBLE] =
        "The method \"%1\" in type \"%T2\" has %4 access and is not "
        "accessible here.";
    messages[PROTECTED_INSTANCE_METHOD_NOT_ACCESSIBLE] =
        "The instance method \"%1\" in class \"%T2\" has protected "
        "access, but the qualifying expression is not of type \"%T4\" "
        "or any of its enclosing types.";
    messages[PROTECTED_INTERFACE_METHOD_NOT_ACCESSIBLE] =
        "The method \"%1\" only has protected access in \"java.lang.Object\", "
        "so it is not accessible from an interface.";
    messages[CONSTRUCTOR_NOT_ACCESSIBLE] =
        "The constructor \"%1\" in type \"%T2\" has %4 access and is "
        "not accessible here.";
    messages[BAD_ABSTRACT_METHOD_MODIFIER] =
        "A method declaration that contains the keyword \"abstract\" may "
        "not contain any of the keywords: \"private\", \"static\", "
        "\"final\", \"native\", \"strictfp\" or \"synchronized\".";
    messages[STRICTFP_NATIVE_METHOD] =
        "A \"native\" method method may not also be \"strictfp\".";
    messages[ABSTRACT_METHOD_INVOCATION] =
        "An abstract method, \"%1\", cannot be invoked.";
    messages[FINAL_METHOD_OVERRIDE] =
        "The method \"%1\" cannot replace the accessible final method \"%2\" "
        "declared in type \"%T3\".";
    messages[FINAL_IMPLICIT_METHOD_OVERRIDE] =
        "The explicit method \"%1\" is not allowed in an interface, "
        "because it conflicts with the final method \"%2\" declared "
        "implicitly for interfaces.";
    messages[INSTANCE_METHOD_OVERRIDE] =
        "The static method \"%1\" cannot hide the accessible instance "
        "method \"%2\" declared in type \"%T3\".";
    messages[INSTANCE_METHOD_OVERRIDE_EXTERNALLY] =
        "In class \"%1\", the static method \"%2\", inherited from the "
        "superclass \"%T3\", conflicts with the abstract instance "
        "method \"%5\", inherited from the interface \"%T6\".";
    messages[CLASS_METHOD_OVERRIDE] =
        "The instance method \"%1\" cannot override the accessible static "
        "method \"%2\" declared in type \"%T3\".";
    messages[MISMATCHED_OVERRIDDEN_EXCEPTION] =
        "The checked exception \"%1\" is not assignable to any exception "
        "in the throws clause of the accessible method \"%2\" declared in "
        "type \"%T3\".";
    messages[MISMATCHED_IMPLICIT_OVERRIDDEN_EXCEPTION] =
        "The checked exception \"%1\" is not compatible with the "
        "throws clause in the method \"%2\" declared implicitly for "
        "interfaces.";
    messages[MISMATCHED_OVERRIDDEN_EXCEPTION_EXTERNALLY] =
        "In type \"%1\", the checked exception \"%2\" specified by "
        "method \"%3\", inherited from type \"%T4\", is not "
        "assignable to any exception in the throws clause of the "
        "overridden method \"%6\" declared in type \"%T7\".";
    messages[ABSTRACT_METHOD_WITH_BODY] =
        "The declaration of the abstract or native method, \"%1\", "
        "must not contain a method body.";
    messages[NON_ABSTRACT_METHOD_WITHOUT_BODY] =
        "The declaration of the non-abstract and non-native method, \"%1\", "
        "must contain a method body.";
    messages[BAD_ACCESS_METHOD_OVERRIDE] =
        "The method \"%1\" with %2 access cannot replace the accessible "
        "method \"%3\" with %4 access declared in type \"%T5\".";
    messages[BAD_ACCESS_METHOD_OVERRIDE_EXTERNALLY] =
        "In class \"%1\", the method \"%2\" with %3 access, inherited "
        "from type \"%T4\", cannot override the method \"%6\" with "
        "%7 access, inherited from type \"%T8\".";
    messages[CIRCULAR_THIS_CALL] =
        "The constructor \"%1\" may not directly or indirectly invoke itself.";
    messages[INSTANCE_VARIABLE_IN_EXPLICIT_CONSTRUCTOR] =
        "The instance variable \"%1\" declared in class \"%2\" is not "
        "accessible in an explicit constructor invocation.";
    messages[INSTANCE_METHOD_IN_EXPLICIT_CONSTRUCTOR] =
        "The instance method \"%1\" declared in class \"%2\" is not "
        "accessible in an explicit constructor invocation.";
    messages[SYNTHETIC_VARIABLE_ACCESS] =
        "Illegal attempt to access the synthetic field \"%1\" contained "
        "in class \"%T2\".";
    messages[SYNTHETIC_METHOD_INVOCATION] =
        "Illegal attempt to invoke the synthetic method \"%1\" contained "
        "in class \"%T2\".";
    messages[SYNTHETIC_CONSTRUCTOR_INVOCATION] =
        "Illegal attempt to invoke the synthetic constructor \"%1\" from "
        "class \"%T2\".";
    messages[SYNTHETIC_TYPE_ACCESS] =
        "Illegal attempt to use the synthetic type \"%T1\".";
    messages[UNNAMED_TYPE_ACCESS] =
        "Illegal attempt to use the type \"%T1\" which does not have a fully "
        "qualified name since it is local or anonymous.";
    messages[SELF_IN_EXPLICIT_CONSTRUCTOR] =
        "The expression \"%1\" is not yet initialized here.";
    messages[EXPRESSION_NOT_CONSTANT] =
        "A constant expression is expected in this context. A constant "
        "expression is built from literals, operators, and constant "
        "variables referenced by 'id' or 'Classname.id'.";
    messages[UNCAUGHT_METHOD_EXCEPTION] =
        "The method \"%1\" can throw the checked exception \"%T2\", "
        "so its invocation%4";
    messages[UNCAUGHT_CONSTRUCTOR_EXCEPTION] =
        "The constructor \"%1\" can throw the checked exception \"%T2\", "
        "so the class creation%4";
    messages[UNCAUGHT_ANONYMOUS_CONSTRUCTOR_EXCEPTION] =
        "The constructor in the anonymous subclass of \"%1\" can throw the "
        "checked exception \"%T2\", so the class creation%4";
    messages[UNCAUGHT_THROWN_EXCEPTION] =
        "This throw statement throws the checked exception \"%T1\", so it%3";
    messages[UNCAUGHT_EXPLICIT_THIS_EXCEPTION] =
        "This constructor must declare the checked exception \"%T1\" "
        "thrown by the explicit this() call.";
    messages[UNCAUGHT_EXPLICIT_SUPER_EXCEPTION] =
        "This constructor must declare the checked exception \"%T1\" "
        "thrown by the explicit super() call to type \"%T3\".";
    messages[UNREACHABLE_CATCH_CLAUSE] =
        "This catch block is unreachable because there is no non-null "
        "exception whose type is assignable to \"%T1\" that can be thrown "
        "during execution of the body of the try block.";
    messages[UNREACHABLE_STATEMENT] = "This statement is unreachable.";
    messages[UNREACHABLE_STATEMENTS] = "These statements are unreachable.";
    messages[BLOCKED_CATCH_CLAUSE] =
        "This catch block is unreachable: the exception \"%T1\" "
        "is a subclass of the type \"%T3\", caught previously at location %5.";
    messages[VARIABLE_NOT_DEFINITELY_ASSIGNED] =
        "The variable \"%1\" may be accessed here before having been "
        "definitely assigned a value.";
    messages[TYPED_METHOD_WITH_NO_RETURN] =
        "The method \"%1\" must contain a return statement with an "
        "expression compatible with type \"%2\".";
    messages[DEFAULT_METHOD_NOT_OVERRIDDEN] =
        "Method \"%1\" in class \"%T2\" does not override or hide "
        "the corresponding method with default access in class \"%T4\".";
    messages[DEPRECATED_METHOD_OVERRIDE] =
        "The overridden method \"%1\" is deprecated in type \"%T2\".";

    // Package related errors.
    messages[WRONG_TYPE_IN_CLASSFILE] =
        "The file \"%1.class\" was found in directory \"%F2\" specified in "
        "the CLASSPATH. However, that class file specifies the type \"%4\".";
    messages[TYPE_NAME_MISMATCH] =
        "The name of the type specified, \"%T1\", does not match "
        "the name found in the class file: \"%3\".";

    // Deprecation errors.
    messages[DEPRECATED_TYPE] =
        "The type \"%T1\" has been deprecated.";
    messages[DEPRECATED_FIELD] =
        "The variable \"%1\" declared in type \"%T2\" has been deprecated.";
    messages[DEPRECATED_METHOD] =
        "The method \"%1\" declared in type \"%T2\" has been deprecated.";
    messages[DEPRECATED_CONSTRUCTOR] =
        "The constructor \"%1\" declared in type \"%T2\" has been deprecated.";

    // Inner type related errors.
    messages[INTERFACE_NOT_INNER_CLASS] =
        "The interface \"%T1\" is not an inner class.";
    messages[STATIC_NOT_INNER_CLASS] =
        "The static class \"%T1\" is not an inner class.";
    messages[SUPER_TYPE_NOT_INNER_CLASS] =
        "The super type \"%T1\" of this type, \"%T3\", is not "
        "an inner class that is immediately enclosed in type \"%T5\".";
    messages[STATIC_FIELD_IN_INNER_CLASS_NOT_FINAL] =
        "This static variable declaration is invalid, because it is not "
        "final, but is enclosed in an inner class, \"%1\", located at %2.";
    messages[STATIC_FIELD_IN_INNER_CLASS_NOT_CONSTANT] =
        "The static final field \"%1\" is invalid, because it does not "
        "represent a compile-time constant, but is enclosed in an inner "
        "class, \"%2\", located at %3.";
    messages[STATIC_METHOD_IN_INNER_CLASS] =
        "The static method \"%1\" is invalid, because it is enclosed in "
        "an inner class, \"%2\", located at %3.";
    messages[STATIC_TYPE_IN_INNER_CLASS] =
        "The static type \"%1\" is invalid, because it is enclosed in "
        "an inner class, \"%2\", located at %3.";
    messages[STATIC_INITIALIZER_IN_INNER_CLASS] =
        "This static initializer is invalid, because it is enclosed in "
        "an inner class, \"%1\", located at %2.";
    messages[INNER_CLASS_REFERENCE_TO_NON_FINAL_LOCAL_VARIABLE] =
        "Invalid reference in inner class \"%T1\" to a non-final "
        "local variable, \"%3\", declared in method \"%4\".";
    messages[INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_LOCAL] =
        "The unqualified usage of \"%1\" refers to the member inherited "
        "from type \"%T2\", and not the local version in the enclosing "
        "method \"%4\". Renaming the local version is suggested.";
    messages[INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_MEMBER] =
        "The unqualified usage of \"%1\" refers to the member inherited "
        "from type \"%T2\", and not the version declared in the enclosing "
        "type \"%T4\". Explicit qualification is suggested.";
    messages[INHERITANCE_AND_LEXICAL_SCOPING_CONFLICT_WITH_TYPE] =
        "The unqualified usage of \"%1\" refers to the inherited member "
        "type \"%T2\", and not the enclosing type \"%T4\". "
        "Explicit qualification is suggested.";
    messages[ILLEGAL_THIS_FIELD_ACCESS] =
        "The type \"%T1\" is either not an outer type of type \"%T3\" "
        "or it is not accessible because this expression appears in a "
        "static region.";
    messages[CONSTRUCTOR_FOUND_IN_ANONYMOUS_CLASS] =
        "An anonymous class cannot have a constructor. Assuming "
        "that \"%1\" is a method with missing return type.";
    messages[ENCLOSING_INSTANCE_ACCESS_FROM_CONSTRUCTOR_INVOCATION] =
        "The innermost enclosing instance of type \"%T1\" "
        "is \"this\", which is not yet initialized here.";
    messages[ENCLOSING_INSTANCE_ACCESS_ACROSS_STATIC_REGION] =
        "An instance of \"%T1.this\" exists, but is not accessible at this "
        "location because an intermediate anonymous type occurs in an "
        "explicit constructor call.";
    messages[ENCLOSING_INSTANCE_NOT_ACCESSIBLE] =
        "An instance of \"%T1.this\" is not accessible here. In general, an "
        "enclosing instance is accessible only in the body of an instance "
        "method, constructor (after the explicit constructor invocation, if "
        "any), initializer block, or in the initializer expression of an "
        "instance variable.";
    messages[INVALID_ENCLOSING_INSTANCE] =
        "The super type of this type, \"%T1\", is immediately enclosed "
        "in type \"%T3\" which does not match the type of this "
        "primary expression, \"%T5\".";
    messages[STATIC_TYPE_ACCESSING_MEMBER_TYPE] =
        "The static type \"%T1\" must use a qualified name to access the "
        "non-static member type \"%T3\" of the enclosing type \"%T5\".";

    // Java 7 errors
    messages[TYPE_NOT_AUTOCLOSEABLE] =
        "The type \"%T1.%T3\" must implement java.lang.AutoCloseable to be "
        "used in a try-with-resources statement.";
    messages[DUPLICATE_EXCEPTION_IN_MULTICATCH] =
        "The exception type \"%T1.%T3\" appears more than once in this "
        "multi-catch clause.";
    messages[REDUNDANT_EXCEPTION_IN_MULTICATCH] =
        "The exception type \"%T1.%T3\" is a subtype of \"%T5.%T7\" in this "
        "multi-catch clause.";

    //
    // Make sure that there is a message associated with each code
    //
    for (int j = 0; j < _num_kinds; j++)
        assert(messages[j]);
}


} // Close namespace Jopa block

