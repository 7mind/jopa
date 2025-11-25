#include "platform.h"
#include "javasym.h"
#include "control.h"
#include "stream.h"

#ifdef JOPA_DEBUG


namespace Jopa { // Open namespace Jopa block
static const char* token_type(unsigned char kind)
{
    switch (kind)
    {
    case TK_Identifier: return "TK_Identifier";
    case TK_abstract: return "TK_abstract";
    case TK_assert: return "TK_assert";
    case TK_boolean: return "TK_boolean";
    case TK_break: return "TK_break";
    case TK_byte: return "TK_byte";
    case TK_case: return "TK_case";
    case TK_catch: return "TK_catch";
    case TK_char: return "TK_char";
    case TK_class: return "TK_class";
    case TK_const: return "TK_const";
    case TK_continue: return "TK_continue";
    case TK_default: return "TK_default";
    case TK_do: return "TK_do";
    case TK_double: return "TK_double";
    case TK_else: return "TK_else";
    case TK_enum: return "TK_enum";
    case TK_extends: return "TK_extends";
    case TK_false: return "TK_false";
    case TK_final: return "TK_final";
    case TK_finally: return "TK_finally";
    case TK_float: return "TK_float";
    case TK_for: return "TK_for";
    case TK_goto: return "TK_goto";
    case TK_if: return "TK_if";
    case TK_implements: return "TK_implements";
    case TK_import: return "TK_import";
    case TK_instanceof: return "TK_instanceof";
    case TK_int: return "TK_int";
    case TK_interface: return "TK_interface";
    case TK_long: return "TK_long";
    case TK_native: return "TK_native";
    case TK_new: return "TK_new";
    case TK_null: return "TK_null";
    case TK_package: return "TK_package";
    case TK_private: return "TK_private";
    case TK_protected: return "TK_protected";
    case TK_public: return "TK_public";
    case TK_return: return "TK_return";
    case TK_short: return "TK_short";
    case TK_static: return "TK_static";
    case TK_strictfp: return "TK_strictfp";
    case TK_super: return "TK_super";
    case TK_switch: return "TK_switch";
    case TK_synchronized: return "TK_synchronized";
    case TK_this: return "TK_this";
    case TK_throw: return "TK_throw";
    case TK_throws: return "TK_throws";
    case TK_transient: return "TK_transient";
    case TK_true: return "TK_true";
    case TK_try: return "TK_try";
    case TK_void: return "TK_void";
    case TK_volatile: return "TK_volatile";
    case TK_while: return "TK_while";
    case TK_IntegerLiteral: return "TK_IntegerLiteral";
    case TK_LongLiteral: return "TK_LongLiteral";
    case TK_FloatLiteral: return "TK_FloatLiteral";
    case TK_DoubleLiteral: return "TK_DoubleLiteral";
    case TK_CharacterLiteral: return "TK_CharacterLiteral";
    case TK_StringLiteral: return "TK_StringLiteral";
    case TK_PLUS_PLUS: return "TK_PLUS_PLUS";
    case TK_MINUS_MINUS: return "TK_MINUS_MINUS";
    case TK_EQUAL_EQUAL: return "TK_EQUAL_EQUAL";
    case TK_LESS_EQUAL: return "TK_LESS_EQUAL";
    case TK_GREATER_EQUAL: return "TK_GREATER_EQUAL";
    case TK_NOT_EQUAL: return "TK_NOT_EQUAL";
    case TK_LEFT_SHIFT: return "TK_LEFT_SHIFT";
    case TK_RIGHT_SHIFT: return "TK_RIGHT_SHIFT";
    case TK_UNSIGNED_RIGHT_SHIFT: return "TK_UNSIGNED_RIGHT_SHIFT";
    case TK_PLUS_EQUAL: return "TK_PLUS_EQUAL";
    case TK_MINUS_EQUAL: return "TK_MINUS_EQUAL";
    case TK_MULTIPLY_EQUAL: return "TK_MULTIPLY_EQUAL";
    case TK_DIVIDE_EQUAL: return "TK_DIVIDE_EQUAL";
    case TK_AND_EQUAL: return "TK_AND_EQUAL";
    case TK_OR_EQUAL: return "TK_OR_EQUAL";
    case TK_XOR_EQUAL: return "TK_XOR_EQUAL";
    case TK_REMAINDER_EQUAL: return "TK_REMAINDER_EQUAL";
    case TK_LEFT_SHIFT_EQUAL: return "TK_LEFT_SHIFT_EQUAL";
    case TK_RIGHT_SHIFT_EQUAL: return "TK_RIGHT_SHIFT_EQUAL";
    case TK_UNSIGNED_RIGHT_SHIFT_EQUAL: return "TK_UNSIGNED_RIGHT_SHIFT_EQUAL";
    case TK_OR_OR: return "TK_OR_OR";
    case TK_AND_AND: return "TK_AND_AND";
    case TK_PLUS: return "TK_PLUS";
    case TK_MINUS: return "TK_MINUS";
    case TK_NOT: return "TK_NOT";
    case TK_REMAINDER: return "TK_REMAINDER";
    case TK_XOR: return "TK_XOR";
    case TK_AND: return "TK_AND";
    case TK_MULTIPLY: return "TK_MULTIPLY";
    case TK_OR: return "TK_OR";
    case TK_TWIDDLE: return "TK_TWIDDLE";
    case TK_DIVIDE: return "TK_DIVIDE";
    case TK_GREATER: return "TK_GREATER";
    case TK_LESS: return "TK_LESS";
    case TK_LPAREN: return "TK_LPAREN";
    case TK_RPAREN: return "TK_RPAREN";
    case TK_LBRACE: return "TK_LBRACE";
    case TK_RBRACE: return "TK_RBRACE";
    case TK_LBRACKET: return "TK_LBRACKET";
    case TK_RBRACKET: return "TK_RBRACKET";
    case TK_SEMICOLON: return "TK_SEMICOLON";
    case TK_QUESTION: return "TK_QUESTION";
    case TK_COLON: return "TK_COLON";
    case TK_COMMA: return "TK_COMMA";
    case TK_DOT: return "TK_DOT";
    case TK_ELLIPSIS: return "TK_ELLIPSIS";
    case TK_AT: return "TK_AT";
    case TK_EQUAL: return "TK_EQUAL";
    case TK_ERROR: return "TK_ERROR";
    case TK_EOF: return "TK_EOF";
    default: return "TK_notoken";
    }
}

void LexStream::Dump()
{
    FILE* tokfile;
    // +1 for '\0' +4 for length(".tok")
    char* tokfile_name = new char[FileNameLength() + 5];
    strcpy(tokfile_name, FileName());
    strcat(tokfile_name, StringConstant::U8S_DO_tok);

    if ((tokfile = SystemFopen(tokfile_name, "w")) == NULL)
    {
        Coutput << "*** Cannot open LexStream dump output file "
                << tokfile_name << endl;
        return;
    }

    RereadInput();
    SetUpComments();

    TokenIndex tok = 0;
    for (CommentIndex com = FirstComment(tok);
         com > 0 && com < NumComments() && PrecedingToken(com) == tok; com++)
    {
        fprintf(tokfile, "*%5d ", com);
        fprintf(tokfile, "%s", FileName());
        fprintf(tokfile, ", line %d.%d: ",
                FindLine(comments[com].location),
                FindColumn(comments[com].location - 1) + 1);
        for (const wchar_t* s = CommentString(com); *s; s++)
            fprintf(tokfile, "%c", *s);
        fprintf(tokfile, "\n");
    }
    do
    {
        tok = Gettoken();
        fprintf(tokfile, "%6d ", tok);
        fprintf(tokfile, " %s", FileName());
        fprintf(tokfile, ", %cline %d.%d: %s %s  ",
                (AfterEol(tok) ? '*' : ' '),
                Line(tok), (Kind(tok) == TK_EOF ? 0 : Column(tok)),
                token_type(Kind(tok)),
                (tokens[tok].Deprecated() ? "(d)" : " "));
        for (const wchar_t* s = NameString(tok); *s; s++)
            fprintf(tokfile, "%c", *s);
        fprintf(tokfile, "\n");

        for (CommentIndex com = FirstComment(tok);
             com > 0 && com < NumComments() && PrecedingToken(com) == tok; com++)
        {
            fprintf(tokfile, "*%5d ", com);
            fprintf(tokfile, " %s", FileName());
            fprintf(tokfile, ", line %d.%d: ",
                    FindLine(comments[com].location),
                    FindColumn(comments[com].location - 1) + 1);
            for (const wchar_t* s = CommentString(com); *s; s++)
                fprintf(tokfile, "%c", *s);
            fprintf(tokfile, "\n");
        }
    } while (Kind(tok) != TK_EOF);

    DestroyInput();
    fprintf(tokfile, "\n");
#ifdef UNIQUE_NAMES
    fprintf(tokfile, "\nThe unique names are:\n\n");
    for (int i = 0; i < control.name_table.symbol_pool.length(); i++)
    {
        fprintf(tokfile, "%4d ", i);
        for (const wchar_t* s = control.name_table.symbol_pool[i].name();
             *s; s++)
        {
            fprintf(tokfile, "%c", *s);
        }
        fprintf(tokfile, "\n");
    }
#endif // UNIQUE_NAMES

    if (tokfile)
        fclose(tokfile);
    delete [] tokfile_name;
}


// We often run into cases where we have a wchar_t that we want to print out
// while debugging. This method provides a way to print out a wchar_t* as
// a char *, if one of the characters is outside the range of a char it
// will be returned as a \u subst. The string must be terminated by
// a U_NULL char.

// There is currently no prototype for this method, that is ok since
// it is to be called from inside the debugger.

const char* Dump(const wchar_t* wstr)
{
    Tuple<char>& str = *(new Tuple<char>(500));
    static char* saved_data = NULL;

    const wchar_t* ptr = wstr;
    while (*ptr)
    {
        const wchar_t c = *ptr;
        if (c >= U_SPACE && c < 128)
            str.Next() = (char) c;
        else
        {
            str.Next() = '\\';
            str.Next() = 'u';
            char buff[5];
            sprintf(buff, "%04x", (int) c);
            str.Next() = buff[0];
            str.Next() = buff[1];
            str.Next() = buff[2];
            str.Next() = buff[3];
        }
        ptr++;
    }

    delete [] saved_data; // Delete data allocated in last call to Dump()
    saved_data = new char[str.Length() + 1];
    char* tmp = saved_data;
    for (int i = 0, len = str.Length(); i < len; i++, tmp++)
        *tmp = str[i];
    *tmp = '\0';

    delete &str;
    return saved_data;
}


} // Close namespace Jopa block

#endif // JOPA_DEBUG

