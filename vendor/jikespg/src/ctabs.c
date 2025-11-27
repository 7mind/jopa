static char hostfile[] = __FILE__;

#include <string.h>
#include "common.h"
#include "space.h"
#include "header.h"

static char dcl_tag[SYMBOL_SIZE],
            sym_tag[SYMBOL_SIZE],
            def_tag[SYMBOL_SIZE],
            prs_tag[SYMBOL_SIZE];

static int  byte_check_bit = 1;

/*********************************************************************/
/*                         NON_TERMINAL_TIME_ACTION:                 */
/*********************************************************************/
static void non_terminal_time_action(void)
{
    if (c_bit)
        fprintf(sysprs,
                "#define nt_action(state, sym) \\\n"
                "           ((check[state+sym] == sym) ? \\\n"
                "                   action[state + sym] : "
                                    "default_goto[sym])\n\n");
    else if (cpp_bit)
        fprintf(sysprs,
                "    static int nt_action(int state, int sym)\n"
                "    {\n"
                "        return (check[state + sym] == sym)\n"
                "                                    ? action[state + sym]\n"
                "                                    : default_goto[sym];\n"
                "    }\n\n");
    else if (java_bit)
        fprintf(sysprs,
                "    public final static int nt_action(int state, int sym)\n"
                "    {\n"
                "        return (check(state + sym) == sym)\n"
                "                                    ? action[state + sym]\n"
                "                                    : default_goto[sym];\n"
                "    }\n\n");
    return;
}


/*********************************************************************/
/*            NON_TERMINAL_NO_GOTO_DEFAULT_TIME_ACTION:              */
/*********************************************************************/
static void non_terminal_no_goto_default_time_action(void)
{
    if (c_bit)
        fprintf(sysprs,
                "#define nt_action(state, sym) action[state + sym]\n\n");
    else if (cpp_bit)
        fprintf(sysprs,
                "    static int nt_action(int state, int sym)\n"
                "    {\n        return action[state + sym];\n    }\n\n");
    else if (java_bit)
        fprintf(sysprs,
                "    public final static int nt_action(int state, int sym)\n"
                "    {\n        return action[state + sym];\n    }\n\n");
    return;
}


/*********************************************************************/
/*                        NON_TERMINAL_SPACE_ACTION:                 */
/*********************************************************************/
static void non_terminal_space_action(void)
{
    if (c_bit)
        fprintf(sysprs,
                "#define nt_action(state, sym) \\\n"
                "           ((base_check[state + sym] == sym) ? \\\n"
                "               base_action[state + sym] : "
                                    "default_goto[sym])\n\n");
    else if (cpp_bit)
        fprintf(sysprs,
                "    static int nt_action(int state, int sym)\n"
                "    {\n"
                "        return (base_check[state + sym] == sym)\n"
                "                             ? base_action[state + sym]\n"
                "                             : default_goto[sym];\n"
                "    }\n\n");
    else if (java_bit)
        fprintf(sysprs,
                "    public final static int nt_action(int state, int sym)\n"
                "    {\n"
                "        return (base_check(state + sym) == sym)\n"
                "                             ? base_action[state + sym]\n"
                "                             : default_goto[sym];\n"
                "    }\n\n");
    return;
}


/*********************************************************************/
/*              NON_TERMINAL_NO_GOTO_DEFAULT_SPACE_ACTION:           */
/*********************************************************************/
static void non_terminal_no_goto_default_space_action(void)
{
    if (c_bit)
        fprintf(sysprs,
                "#define nt_action(state, sym) "
                   "base_action[state + sym]\n\n");
    else if (cpp_bit)
        fprintf(sysprs,
                "    static int nt_action(int state, int sym)\n"
                "    {\n        return base_action[state + sym];\n    }\n\n");
    else if (java_bit)
        fprintf(sysprs,
                "    public final static int nt_action(int state, int sym)\n"
                "    {\n        return base_action[state + sym];\n    }\n\n");
    return;
}


/*********************************************************************/
/*                          TERMINAL_TIME_ACTION:                    */
/*********************************************************************/
static void terminal_time_action(void)
{
    if (c_bit)
        fprintf(sysprs,
            "#define t_action(state, sym, next_tok) \\\n"
            "   action[check[state + sym] == sym ? state + sym : state]\n\n");
    else if (cpp_bit)
        fprintf(sysprs,
            "    static int t_action(int state, int sym, LexStream *stream)\n"
            "    {\n"
            "        return action[check[state + sym] == sym"
                                 " ? state + sym : state];\n"
            "    }\n");

    else if (java_bit)
        fprintf(sysprs,
            "    public final static int t_action(int state, int sym, LexStream stream)\n"
            "    {\n"
            "        return action[check(state + sym) == sym"
                                 " ? state + sym : state];\n"
            "    }\n");

    return;
}


/*********************************************************************/
/*                          TERMINAL_SPACE_ACTION:                   */
/*********************************************************************/
static void terminal_space_action(void)
{
    if (c_bit)
        fprintf(sysprs,
            "#define t_action(state, sym, next_tok) \\\n"
            "  term_action[term_check[base_action[state]+sym] == sym ? \\\n"
            "          base_action[state] + sym : base_action[state]]\n\n");
    else if (cpp_bit)
        fprintf(sysprs,
            "    static int t_action(int state, int sym, LexStream *stream)\n"
            "    {\n"
            "        return term_action[term_check[base_action[state]"
                                                      "+sym] == sym\n"
            "                               ? base_action[state] + sym\n"
            "                               : base_action[state]];\n"
            "    }\n");
    else if (java_bit)
        fprintf(sysprs,
            "    public final static int t_action(int state, int sym, LexStream stream)\n"
            "    {\n"
            "        return term_action[term_check[base_action[state]"
                                                      "+sym] == sym\n"
            "                               ? base_action[state] + sym\n"
            "                               : base_action[state]];\n"
            "    }\n");

    return;
}


/*********************************************************************/
/*               TERMINAL_SHIFT_DEFAULT_SPACE_ACTION:                */
/*********************************************************************/
static void terminal_shift_default_space_action(void)
{
    if (c_bit)
        fprintf(sysprs,
            "static int t_action(int state, int sym, TokenObject next_tok)\n"
            "{\n"
            "    int i;\n\n"
            "    if (sym == 0)\n"
            "        return ERROR_ACTION;\n"
            "    i = base_action[state];\n"
            "    if (term_check[i + sym] == sym)\n"
            "        return term_action[i + sym];\n"
            "    i = term_action[i];\n"
            "    return ((shift_check[shift_state[i] + sym] == sym) ?\n"
            "                 default_shift[sym] : default_reduce[i]);\n"
            "}\n\n");
    else if (cpp_bit)
        fprintf(sysprs,
            "    static int t_action(int state, int sym, LexStream *stream)\n"
            "    {\n"
            "        if (sym == 0)\n"
            "            return ERROR_ACTION;\n"
            "        int i = base_action[state];\n"
            "        if (term_check[i + sym] == sym)\n"
            "            return term_action[i + sym];\n"
            "        i = term_action[i];\n"
            "        return ((shift_check[shift_state[i] + sym] == sym) ?\n"
            "                      default_shift[sym] : default_reduce[i]);\n"
            "    }\n");
    else if (java_bit)
        fprintf(sysprs,
            "    public final static int t_action(int state, int sym, LexStream stream)\n"
            "    {\n"
            "        if (sym == 0)\n"
            "            return ERROR_ACTION;\n"
            "        int i = base_action[state];\n"
            "        if (term_check[i + sym] == sym)\n"
            "            return term_action[i + sym];\n"
            "        i = term_action[i];\n"
            "        return ((shift_check[shift_state[i] + sym] == sym) ?\n"
            "                      default_shift[sym] : default_reduce[i]);\n"
            "    }\n");

    return;
}


/*********************************************************************/
/*                          TERMINAL_TIME_LALR_K:                    */
/*********************************************************************/
static void terminal_time_lalr_k(void)
{
    if (c_bit)
        fprintf(sysprs,
            "static int t_action(int act, int sym, TokenObject next_tok)\n"
            "{\n"
            "    int i = act + sym;\n\n"
            "    act = action[check[i] == sym ? i : act];\n\n"
            "    if (act > LA_STATE_OFFSET)\n"
            "    {\n"
            "        for (;;)\n"
            "        {\n"
            "            act -= ERROR_ACTION;\n"
            "            sym = Class(next_tok);\n"
            "            i = act + sym;\n"
            "            act = action[check[i] == sym ? i : act];\n"
            "            if (act <= LA_STATE_OFFSET)\n"
            "                break;\n"
            "            next_tok = Next(next_tok);\n"
            "        }\n"
            "    }\n\n"
            "    return act;\n"
            "}\n\n");
    else if (cpp_bit)
        fprintf(sysprs,
            "    static int t_action(int act, int sym, LexStream *stream)\n"
            "    {\n"
            "        int i = act + sym;\n\n"
            "        act = action[check[i] == sym ? i : act];\n\n"
            "        if (act > LA_STATE_OFFSET)\n"
            "        {\n"
            "            for (TokenObject tok = stream -> Peek();\n"
            "                 ;\n"
            "                 tok = stream -> Next(tok))\n"
            "            {\n"
            "                act -= ERROR_ACTION;\n"
            "                sym = stream -> Kind(tok);\n"
            "                i = act + sym;\n"
            "                act = action[check[i] == sym ? i : act];\n"
            "                if (act <= LA_STATE_OFFSET)\n"
            "                    break;\n"
            "            }\n"
            "        }\n\n"
            "        return act;\n"
            "    }\n\n");
    else if (java_bit)
        fprintf(sysprs,
            "    public final static int t_action(int act, int sym, LexStream stream)\n"
            "    {\n"
            "        int i = act + sym;\n\n"
            "        act = action[check(i) == sym ? i : act];\n\n"
            "        if (act > LA_STATE_OFFSET)\n"
            "        {\n"
            "            for (int tok = stream.Peek();\n"
            "                 ;\n"
            "                 tok = stream.Next(tok))\n"
            "            {\n"
            "                act -= ERROR_ACTION;\n"
            "                sym = stream.Kind(tok);\n"
            "                i = act + sym;\n"
            "                act = action[check(i) == sym ? i : act];\n"
            "                if (act <= LA_STATE_OFFSET)\n"
            "                    break;\n"
            "            }\n"
            "        }\n\n"
            "        return act;\n"
            "    }\n\n");

    return;
}


/*********************************************************************/
/*                       TERMINAL_SPACE_LALR_K:                      */
/*********************************************************************/
static void terminal_space_lalr_k(void)
{
    if (c_bit)
        fprintf(sysprs,
            "static int t_action(int state, int sym, TokenObject next_tok)\n"
            "{\n"
            "    int act = base_action[state],\n"
            "          i = act + sym;\n\n"
            "    act = term_action[term_check[i] == sym ? i : act];\n\n"
            "    if (act > LA_STATE_OFFSET)\n"
            "    {\n"
            "        for (;;)\n"
            "        {\n"
            "           act -= LA_STATE_OFFSET;\n"
            "           sym = Class(next_tok);\n"
            "           i = act + sym;\n"
            "           act = term_action[term_check[i] == sym ? i : act];\n"
            "           if (act <= LA_STATE_OFFSET)\n"
            "               break;\n"
            "           next_tok = Next(next_tok);\n"
            "        }\n"
            "    }\n\n"
            "    return act;\n"
            "}\n\n");
    else if (cpp_bit)
        fprintf(sysprs,
        "    static int t_action(int act, int sym, LexStream *stream)\n"
        "    {\n"
        "        act = base_action[act];\n"
        "        int i = act + sym;\n\n"
        "        act = term_action[term_check[i] == sym ? i : act];\n\n"
        "        if (act > LA_STATE_OFFSET)\n"
        "        {\n"
        "            for (TokenObject tok = stream -> Peek();\n"
        "                 ;\n"
        "                 tok = stream -> Next(tok))\n"
        "            {\n"
        "               act -= LA_STATE_OFFSET;\n"
        "               sym = stream -> Kind(tok);\n"
        "               i = act + sym;\n"
        "               act = term_action[term_check[i] == sym ? i : act];\n"
        "               if (act <= LA_STATE_OFFSET)\n"
        "                   break;\n"
        "            } \n"
        "        }\n\n"
        "        return act;\n"
        "    }\n");
    else if (java_bit)
        fprintf(sysprs,
        "    public final static int t_action(int act, int sym, LexStream stream)\n"
        "    {\n"
        "        act = base_action[act];\n"
        "        int i = act + sym;\n\n"
        "        act = term_action[term_check[i] == sym ? i : act];\n\n"
        "        if (act > LA_STATE_OFFSET)\n"
        "        {\n"
        "            for (int tok = stream.Peek();\n"
        "                 ;\n"
        "                 tok = stream.Next(tok))\n"
        "            {\n"
        "               act -= LA_STATE_OFFSET;\n"
        "               sym = stream.Kind(tok);\n"
        "               i = act + sym;\n"
        "               act = term_action[term_check[i] == sym ? i : act];\n"
        "               if (act <= LA_STATE_OFFSET)\n"
        "                   break;\n"
        "            } \n"
        "        }\n\n"
        "        return act;\n"
        "    }\n");

    return;
}


/*********************************************************************/
/*                TERMINAL_SHIFT_DEFAULT_SPACE_LALR_K:               */
/*********************************************************************/
static void terminal_shift_default_space_lalr_k(void)
{
    if (c_bit)
        fprintf(sysprs,
            "static int t_action(int state, int sym, TokenObject next_tok)\n"
            "{\n"
            "    int act = base_action[state],\n"
            "          i = act + sym;\n\n"
            "    if (sym == 0)\n"
            "        act = ERROR_ACTION;\n"
            "    else if (term_check[i] == sym)\n"
            "        act = term_action[i];\n"
            "    else\n"
            "    {\n"
            "        act = term_action[act];\n"
            "        i = shift_state[act] + sym;\n"
            "        act = (shift_check[i] == sym ? default_shift[sym]\n"
            "                                     : default_reduce[act]);\n"
            "    }\n\n"
            "    while (act > LA_STATE_OFFSET)\n"
            "    {\n"
            "         act -= LA_STATE_OFFSET;\n"
            "         sym = Class(next_tok);\n"
            "         i = act + sym;\n"
            "         if (term_check[i] == sym)\n"
            "             act = term_action[i];\n"
            "         else\n"
            "         {\n"
            "             act = term_action[act];\n"
            "             i = shift_state[act] + sym;\n"
            "             act = (shift_check[i] == sym\n"
            "                                    ? default_shift[sym]\n"
            "                                    : default_reduce[act]);\n"
            "         }\n"
            "         if (act <= LA_STATE_OFFSET)\n"
            "             break;\n"
            "         next_tok = Next(next_tok);\n"
            "    }\n\n"
            "    return act;\n"
            "}\n\n");
    else if (cpp_bit)
        fprintf(sysprs,
        "    static int t_action(int act, int sym, LexStream *stream)\n"
        "    {\n"
        "        act = base_action[act];\n"
        "        int i = act + sym;\n\n"
        "        if (sym == 0)\n"
        "            act = ERROR_ACTION;\n"
        "        else if (term_check[i] == sym)\n"
        "            act = term_action[i];\n"
        "        else\n"
        "        {\n"
        "            act = term_action[act];\n"
        "            i = shift_state[act] + sym;\n"
        "            act = (shift_check[i] == sym ? default_shift[sym]\n"
        "                                         : default_reduce[act]);\n"
        "        }\n\n"
        "        if (act > LA_STATE_OFFSET)\n"
        "        {\n"
        "            for (TokenObject tok = stream -> Peek();\n"
        "                 ;\n"
        "                 tok = stream -> Next(tok))\n"
        "            {\n"
        "                 act -= LA_STATE_OFFSET;\n"
        "                 sym = stream -> Kind(tok);\n"
        "                 i = act + sym;\n"
        "                 if (term_check[i] == sym)\n"
        "                     act = term_action[i];\n"
        "                 else\n"
        "                 {\n"
        "                     act = term_action[act];\n"
        "                     i = shift_state[act] + sym;\n"
        "                     act = (shift_check[i] == sym\n"
        "                                            ? default_shift[sym]\n"
        "                                            : default_reduce[act]);\n"
        "                 }\n"
        "                 if (act <= LA_STATE_OFFSET)\n"
        "                     break;\n"
        "            }\n"
        "        }\n\n"
        "        return act;\n"
        "    }\n");
    else if (java_bit)
        fprintf(sysprs,
        "    public final static int t_action(int act, int sym, LexStream stream)\n"
        "    {\n"
        "        act = base_action[act];\n"
        "        int i = act + sym;\n\n"
        "        if (sym == 0)\n"
        "            act = ERROR_ACTION;\n"
        "        else if (term_check[i] == sym)\n"
        "            act = term_action[i];\n"
        "        else\n"
        "        {\n"
        "            act = term_action[act];\n"
        "            i = shift_state[act] + sym;\n"
        "            act = (shift_check[i] == sym ? default_shift[sym]\n"
        "                                         : default_reduce[act]);\n"
        "        }\n\n"
        "        if (act > LA_STATE_OFFSET)\n"
        "        {\n"
        "            for (int tok = stream.Peek();\n"
        "                 ;\n"
        "                 tok = stream.Next(tok))\n"
        "            {\n"
        "                 act -= LA_STATE_OFFSET;\n"
        "                 sym = stream.Kind(tok);\n"
        "                 i = act + sym;\n"
        "                 if (term_check[i] == sym)\n"
        "                     act = term_action[i];\n"
        "                 else\n"
        "                 {\n"
        "                     act = term_action[act];\n"
        "                     i = shift_state[act] + sym;\n"
        "                     act = (shift_check[i] == sym\n"
        "                                            ? default_shift[sym]\n"
        "                                            : default_reduce[act]);\n"
        "                 }\n"
        "                 if (act <= LA_STATE_OFFSET)\n"
        "                     break;\n"
        "            }\n"
        "        }\n\n"
        "        return act;\n"
        "    }\n");

    return;
}


/*********************************************************************/
/*                            INIT_FILE:                             */
/*********************************************************************/
static void init_file(FILE **file, char *file_name, char *file_tag)
{
    char *p;

#if defined(C370) && !defined(CW)
    int size;

    size = PARSER_LINE_SIZE;
    if (record_format != 'F')
        size += 4;
    sprintf(msg_line, "w, recfm=%cB, lrecl=%d",
                      record_format, size);

    p = strchr(file_name, '.');
    if ((*file = fopen(file_name, msg_line)) == NULL)
#else
    p = strrchr(file_name, '.');
    if ((*file = fopen(file_name, "w")) == NULL)
#endif
    {
        fprintf(stderr,
                "***ERROR: Symbol file \"%s\" cannot be opened\n",
                file_name);
        exit(12);
    }

    memcpy(file_tag, file_name, p - file_name);
    file_tag[p - file_name] = '\0';

    if (jikes_bit)
	fprintf(*file, "// $I"/*CVS hack*/"d$\n"
		"// DO NOT MODIFY THIS FILE - it is generated using jikespg on java.g.\n"
		"//\n"
		"// This software is subject to the terms of the IBM Jikes Compiler Open\n"
		"// Source License Agreement available at the following URL:\n"
		"// http://ibm.com/developerworks/opensource/jikes.\n"
		"// Copyright (C) 1996, 1997, 1998, 1999, 2001, International\n"
		"// Business Machines Corporation and others.  All Rights Reserved.\n"
		"// You must accept the terms of that agreement to use this software.\n"
		"//\n\n");

    if (c_bit || cpp_bit)
    {
        fprintf(*file, "#ifndef %s_INCLUDED\n", file_tag);
        fprintf(*file, "#define %s_INCLUDED\n\n", file_tag);
    }

    if (jikes_bit)
	fprintf(*file, "#ifdef HAVE_JIKES_NAMESPACE\n"
		"namespace Jikes { // Open namespace Jikes block\n"
		"#endif\n\n");

    return;
}


/*********************************************************************/
/*                        INIT_PARSER_FILES:                         */
/*********************************************************************/
static void init_parser_files(void)
{
    init_file(&sysdcl, dcl_file, dcl_tag);
    init_file(&syssym, sym_file, sym_tag);
    init_file(&sysdef, def_file, def_tag);
    init_file(&sysprs, prs_file, prs_tag);

    return;
}


/*********************************************************************/
/*                            EXIT_FILE:                             */
/*********************************************************************/
static void exit_file(FILE **file, char *file_tag)
{
    if (jikes_bit)
	fprintf(*file, "\n#ifdef HAVE_JIKES_NAMESPACE\n"
		"} // Close namespace Jikes block\n"
		"#endif\n");

    if (c_bit || cpp_bit)
        fprintf(*file, "\n#endif /* %s_INCLUDED */\n", file_tag);

    fclose(*file);

    return;
}


/*********************************************************************/
/*                            EXIT_PRS_FILE:                         */
/*********************************************************************/
static void exit_parser_files(void)
{
    exit_file(&sysdcl, dcl_tag);
    exit_file(&syssym, sym_tag);
    exit_file(&sysdef, def_tag);
    exit_file(&sysprs, prs_tag);

    return;
}


/**************************************************************************/
/*                              PRINT_C_NAMES:                            */
/**************************************************************************/
static void print_c_names(void)
{
    char tok[SYMBOL_SIZE + 1];
    short *name_len = Allocate_short_array(num_names + 1);
    long num_bytes = 0;
    int i, j, k, n;

    max_name_length = 0;
    mystrcpy("\nconst char  CLASS_HEADER string_buffer[] = {0,\n");

    n = 0;
    j = 0;
    padline();
    for (i = 1; i <= num_names; i++)
    {
        strcpy(tok, RETRIEVE_NAME(i));
        name_len[i] = strlen(tok);
        num_bytes += name_len[i];
        if (max_name_length < name_len[i])
            max_name_length = name_len[i];
        k = 0;
        for (j = 0; j < name_len[i]; j++)
        {
            *output_ptr++ = '\'';
            if (tok[k] == '\'' || tok[k] == '\\')
                *output_ptr++ = '\\';
            if (tok[k] == '\n')
                *output_ptr++ = escape;
            else
                *output_ptr++ = tok[k];
            k++;
            *output_ptr++ = '\'';
            *output_ptr++ = ',';
            n++;
            if (n == 10 && ! (i == num_names && j == name_len[i] - 1))
            {
                n = 0;
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
            }
        }
    }
    *(output_ptr - 1) = '\n';  /*overwrite last comma*/
    BUFFER_CHECK(sysdcl);
    if (java_bit)
         mystrcpy("    };\n");
    else mystrcpy("                          };\n");

    /*****************************************************************/
    /* Compute and list space required for STRING_BUFFER map.        */
    /*****************************************************************/
    sprintf(msg_line,
            "    Storage required for STRING_BUFFER map: %d Bytes",
            num_bytes);
    PRNT(msg_line);

    /******************************/
    /* Write out NAME_START array */
    /******************************/
    mystrcpy("\nconst unsigned short CLASS_HEADER name_start[] = {0,\n");

    padline();
    j = 1;
    k = 0;
    for (i = 1; i <= num_names; i++)
    {
        itoc(j);
        *output_ptr++ = COMMA;
        j += name_len[i];
        k++;
        if (k == 10 && i != num_names)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(sysdcl);
            padline();
            k = 0;
        }
    }

    if (k != 0)
    {
        *(output_ptr - 1) = '\n';
        BUFFER_CHECK(sysdcl);
    }
    if (java_bit)
         mystrcpy("    };\n");
    else mystrcpy("                          };\n");

    /*****************************************************************/
    /* Compute and list space required for NAME_START map.           */
    /*****************************************************************/
    sprintf(msg_line,
            "    Storage required for NAME_START map: %d Bytes",
            (2 * num_names));
    PRNT(msg_line);

    /*******************************/
    /* Write out NAME_LENGTH array */
    /*******************************/
    prnt_shorts("\nconst unsigned char  CLASS_HEADER name_length[] = {0,\n",
                1, num_names, 10, name_len);

    /*****************************************************************/
    /* Compute and list space required for NAME_LENGTH map.          */
    /*****************************************************************/
    sprintf(msg_line,
            "    Storage required for NAME_LENGTH map: %d Bytes",
            num_names);
    PRNT(msg_line);

    ffree(name_len);

    return;
}


/**************************************************************************/
/*                             PRINT_JAVA_NAMES:                          */
/**************************************************************************/
static void print_java_names(void)
{
    char tok[SYMBOL_SIZE + 1];
    long num_bytes = 0;
    int i, j, k;

    max_name_length = 0;
    mystrcpy("\n    public final static String name[] = { null,\n");

    for (i = 1; i <= num_names; i++)
    {
        int len;
        strcpy(tok, RETRIEVE_NAME(i));
        len = strlen(tok);
        num_bytes += (len * 2);
        if (max_name_length < len)
            max_name_length = len;
        padline();
        *output_ptr++ = '\"';
        k = 0;
        for (j = 0; j < len; j++)
        {
            if (tok[j] == '\"' || tok[j] == '\\')
                *output_ptr++ = '\\';

            if (tok[j] == '\n')
                *output_ptr++ = escape;
            else
                *output_ptr++ = tok[j];
            k++;
            if (k == 30 && (! (j == len - 1)))
            {
                k = 0;
                *output_ptr++ = '\"';
                *output_ptr++ = ' ';
                *output_ptr++ = '+';
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                *output_ptr++ = '\"';
            }
        }
        *output_ptr++ = '\"';
        if (i < num_names)
            *output_ptr++ = ',';
        *output_ptr++ = '\n';
    }
    BUFFER_CHECK(sysdcl);
    if (java_bit)
         mystrcpy("    };\n");
    else mystrcpy("                          };\n");

    /*****************************************************************/
    /* Compute and list space required for STRING_BUFFER map.        */
    /*****************************************************************/
    sprintf(msg_line,
            "    Storage required for STRING_BUFFER map: %d Bytes",
            num_bytes);
    PRNT(msg_line);

    return;
}


/**************************************************************************/
/*                          PRINT_ERROR_MAPS:                             */
/**************************************************************************/
static void print_error_maps(void)
{
    short *state_start,
          *state_stack,
          *temp,
          *original,
          *as_size,
          *action_symbols_base,
          *action_symbols_range,
          *naction_symbols_base,
          *naction_symbols_range;

    int i,
        j,
        k,
        n,
        offset,
        state_no,
        symbol;

    long num_bytes;

    state_start = Allocate_short_array(num_states + 2);
    state_stack = Allocate_short_array(num_states + 1);

    PRNT("\nError maps storage:");

    /********************************************************************/
    /* We now construct a bit map for the set of terminal symbols that  */
    /* may appear in each state. Then, we invoke PARTSET to apply the   */
    /* Partition Heuristic and print it.                                */
    /********************************************************************/
    as_size = Allocate_short_array(num_states + 1);

    if (table_opt == OPTIMIZE_TIME)
    {
        original = Allocate_short_array(num_symbols + 1);

       /*************************************************************/
       /* In a compressed TIME table, the terminal and non-terminal */
       /* symbols are mixed together when they are remapped.        */
       /* We shall now recover the original number associated with  */
       /* each terminal symbol since it lies very nicely in the     */
       /* range 1..NUM_TERMINALS.  This will save a considerable    */
       /* amount of space in the bit_string representation of sets  */
       /* as well as time when operations are performed on those    */
       /* bit-strings.                                              */
       /*************************************************************/

       for ALL_TERMINALS(symbol)
          original[symbol_map[symbol]] = symbol;
    }

    for ALL_STATES(state_no)
    {
        struct shift_header_type  sh;
        struct reduce_header_type red;

        sh = shift[statset[state_no].shift_number];
        as_size[state_no] = sh.size;
        for (i = 1; i <= sh.size; i++)
        {
            if (table_opt == OPTIMIZE_TIME)
                symbol = original[SHIFT_SYMBOL(sh, i)];
            else
                symbol = SHIFT_SYMBOL(sh, i);
            SET_BIT_IN(action_symbols, state_no, symbol);
        }

        red = reduce[state_no];
        as_size[state_no] += red.size;
        for (i = 1; i <= red.size; i++)
        {
            if (table_opt == OPTIMIZE_TIME)
                symbol = original[REDUCE_SYMBOL(red, i)];
            else
                symbol = REDUCE_SYMBOL(red, i);
            SET_BIT_IN(action_symbols, state_no, symbol);
        }
    }

    partset(action_symbols, as_size, state_list, state_start,
            state_stack, num_terminals);

    ffree(action_symbols);

/*************************************************************/
/* Compute and write out the base of the ACTION_SYMBOLS map. */
/*************************************************************/
    action_symbols_base = Allocate_short_array(num_states + 1);

    for ALL_STATES(i)
        action_symbols_base[state_list[i]] =
                       ABS(state_start[state_list[i]]);
    if (java_bit)
    {
        prnt_shorts("\n    public final static char asb[] = {0,\n",
                    1, num_states, 10, action_symbols_base);
    }
    else
    {
        prnt_shorts("\nconst unsigned short CLASS_HEADER asb[] = {0,\n",
                    1, num_states, 10, action_symbols_base);
    }

    ffree(action_symbols_base);

/**************************************************************/
/* Compute and write out the range of the ACTION_SYMBOLS map. */
/**************************************************************/
    offset = state_start[num_states + 1];
    action_symbols_range = Allocate_short_array(offset);

    compute_action_symbols_range(state_start, state_stack,
                                 state_list, action_symbols_range);

    for (i = 0; i < (offset - 1); i++)
    {
         if (action_symbols_range[i] > (java_bit ? 127 : 255))
         {
             byte_terminal_range = 0;
             break;
         }
    }

    if (byte_terminal_range)
    {
        if (java_bit)
        {
            prnt_shorts("\n    public final static byte asr[] = {0,\n",
                        0, offset - 2, 10, action_symbols_range);
        }
        else
        {
            prnt_shorts("\nconst unsigned char  CLASS_HEADER asr[] = {0,\n",
                        0, offset - 2, 10, action_symbols_range);
        }
    }
    else
    {
        if (java_bit)
        {
            prnt_shorts("\n    public final static char asr[] = {0,\n",
                        0, offset - 2, 10, action_symbols_range);
        }
        else
        {
            prnt_shorts("\nconst unsigned short CLASS_HEADER asr[] = {0,\n",
                        0, offset - 2, 10, action_symbols_range);
        }
    }

    num_bytes = 2 * num_states;
    sprintf(msg_line,
            "    Storage required for ACTION_SYMBOLS_BASE map: "
            "%ld Bytes", num_bytes);
    PRNT(msg_line);

    if ((table_opt == OPTIMIZE_TIME) && (last_terminal <= (java_bit ? 127 : 255)))
        num_bytes = offset - 1;
    else if ((table_opt != OPTIMIZE_TIME) && (num_terminals <= (java_bit ? 127 : 255)))
        num_bytes = offset - 1;
    else
        num_bytes = 2 * (offset - 1);

    sprintf(msg_line,
            "    Storage required for ACTION_SYMBOLS_RANGE map: "
            "%ld Bytes", num_bytes);
    PRNT(msg_line);

    ffree(action_symbols_range);

    /***********************************************************************/
    /* We now repeat the same process for the domain of the GOTO table.    */
    /***********************************************************************/
    for ALL_STATES(state_no)
    {
        as_size[state_no] = gd_index[state_no + 1] - gd_index[state_no];

        for (i = gd_index[state_no]; i <= gd_index[state_no + 1] - 1; i++)
        {
             symbol = gd_range[i] - num_terminals;
             NTSET_BIT_IN(naction_symbols, state_no, symbol);
        }
    }

    partset(naction_symbols, as_size, state_list, state_start,
            state_stack, num_non_terminals);

    ffree(as_size);
    ffree(naction_symbols);

    for (i = 1; i <= gotodom_size; i++)  /* Remap non-terminals */
    {
        if (table_opt == OPTIMIZE_SPACE)
            gd_range[i] = symbol_map[gd_range[i]] - num_terminals;
        else
            gd_range[i] = symbol_map[gd_range[i]];
    }

/*************************************************************/
/* Compute and write out the base of the NACTION_SYMBOLS map.*/
/*************************************************************/
    naction_symbols_base = Allocate_short_array(num_states + 1);

    for ALL_STATES(i)
        naction_symbols_base[state_list[i]] =
                        ABS(state_start[state_list[i]]);

    if (java_bit)
    {
        prnt_shorts("\n    public final static char nasb[] = {0,\n",
                    1, num_states, 10, naction_symbols_base);
    }
    else
    {
        prnt_shorts("\nconst unsigned short CLASS_HEADER nasb[] = {0,\n",
                    1, num_states, 10, naction_symbols_base);
    }

    ffree(naction_symbols_base);

/**************************************************************/
/* Compute and write out the range of the NACTION_SYMBOLS map.*/
/**************************************************************/
    offset = state_start[num_states + 1];
    naction_symbols_range = Allocate_short_array(offset);

    compute_naction_symbols_range(state_start, state_stack,
                                  state_list, naction_symbols_range);

    if (java_bit)
    {
        prnt_shorts("\n    public final static char nasr[] = {0,\n",
                    0, offset - 2, 10, naction_symbols_range);
    }
    else
    {
        prnt_shorts("\nconst unsigned short CLASS_HEADER nasr[] = {0,\n",
                    0, offset - 2, 10, naction_symbols_range);
    }

    num_bytes = 2 * num_states;
    sprintf(msg_line,
            "    Storage required for NACTION_SYMBOLS_BASE map: "
            "%ld Bytes", num_bytes);
    PRNT(msg_line);
    num_bytes = 2 * (offset - 1);
    sprintf(msg_line,
            "    Storage required for NACTION_SYMBOLS_RANGE map: "
            "%ld Bytes", ((long) 2 * (offset - 1)));
    PRNT(msg_line);

    ffree(naction_symbols_range);

    /*********************************************************************/
    /* We write the name_index of each terminal symbol.  The array TEMP  */
    /* is used to remap the NAME_INDEX values based on the new symbol    */
    /* numberings. If time tables are requested, the terminals and non-  */
    /* terminals are mixed together.                                     */
    /*********************************************************************/
    temp = Allocate_short_array(num_symbols + 1);

    if (table_opt == OPTIMIZE_SPACE)
    {
        for ALL_TERMINALS(symbol)
            temp[symbol_map[symbol]] = symno[symbol].name_index;

        if (num_names <= (java_bit ? 127 : 255))
        {
            if (java_bit)
            {
                prnt_shorts
                   ("\n    public final static byte terminal_index[] = {0,\n",
                    1, num_terminals, 10, temp);
            }
            else
            {
                prnt_shorts
                   ("\nconst unsigned char  CLASS_HEADER terminal_index[] = {0,\n",
                    1, num_terminals, 10, temp);
            }
            num_bytes =  num_terminals;
        }
        else
        {
            if (java_bit)
            {
                prnt_shorts
                  ("\n    public final static char terminal_index[] = {0,\n",
                   1, num_terminals, 10, temp);
            }
            else
            {
                prnt_shorts
                  ("\nconst unsigned short CLASS_HEADER terminal_index[] = {0,\n",
                   1, num_terminals, 10, temp);
            }
            num_bytes = 2 * num_terminals;
        }
        /*****************************************************************/
        /* Compute and list space required for TERMINAL_INDEX map.       */
        /*****************************************************************/
        sprintf(msg_line,
                "    Storage required for TERMINAL_INDEX map: %d Bytes",
                num_bytes);
        PRNT(msg_line);

        /******************************************************************/
        /* We write the name_index of each non_terminal symbol. The array */
        /* TEMP is used to remap the NAME_INDEX values based on the new   */
        /* symbol numberings.                                             */
        /******************************************************************/
        for ALL_NON_TERMINALS(symbol)
            temp[symbol_map[symbol]] = symno[symbol].name_index;

        if (num_names <= (java_bit ? 127 : 255))
        {
            if (java_bit)
            {
                prnt_shorts
                    ("\n    public final static byte "
                     "non_terminal_index[] = {0,\n",
                     num_terminals + 1, num_symbols, 10, temp);
            }
            else
            {
                prnt_shorts
                    ("\nconst unsigned char  CLASS_HEADER "
                     "non_terminal_index[] = {0,\n",
                     num_terminals + 1, num_symbols, 10, temp);
            }
            num_bytes = num_non_terminals;
        }
        else
        {
            if (java_bit)
            {
                prnt_shorts
                    ("\n    public final static char "
                     "non_terminal_index[] = {0,\n",
                     num_terminals + 1, num_symbols, 10, temp);
            }
            else
            {
                prnt_shorts
                    ("\nconst unsigned short CLASS_HEADER "
                     "non_terminal_index[] = {0,\n",
                     num_terminals + 1, num_symbols, 10, temp);
            }
            num_bytes = 2 * num_non_terminals;
        }
        /*****************************************************************/
        /* Compute and list space required for NON_TERMINAL_INDEX map.   */
        /*****************************************************************/
        sprintf(msg_line,
                "    Storage required for NON_TERMINAL_INDEX map: %d Bytes",
                num_bytes);
        PRNT(msg_line);
    }
    else
    {
        for ALL_SYMBOLS(symbol)
            temp[symbol_map[symbol]] = symno[symbol].name_index;
        if (num_names <= (java_bit ? 127 : 255))
        {
            if (java_bit)
            {
                prnt_shorts
                   ("\n    public final static byte symbol_index[] = {0,\n",
                    1, num_symbols, 10, temp);
                mystrcpy("    public final static byte terminal_index[] = "
                         "symbol_index;\n");
                mystrcpy("    public final static byte non_terminal_index[] = "
                         "symbol_index;\n");
            }
            else
            {
                prnt_shorts
                   ("\nconst unsigned char  CLASS_HEADER symbol_index[] = {0,\n",
                    1, num_symbols, 10, temp);
                mystrcpy("const unsigned char  *CLASS_HEADER terminal_index[] = "
                         "&(symbol_index[0]);\n");
                mystrcpy("const unsigned char  *CLASS_HEADER non_terminal_index[] = "
                         "&(symbol_index[0]);\n");
            }
            num_bytes = num_symbols;
        }
        else
        {
            if (java_bit)
            {
                prnt_shorts
                   ("\n    public final static char symbol_index[] = {0,\n",
                    1, num_symbols, 10, temp);
                mystrcpy("    public final static char terminal_index[] = "
                         "symbol_index[0];\n");
                mystrcpy("    public final static char non_terminal_index[] = "
                         "symbol_index;\n");
            }
            else
            {
                prnt_shorts
                   ("\nconst unsigned short CLASS_HEADER symbol_index[] = {0,\n",
                    1, num_symbols, 10, temp);
                mystrcpy("const unsigned short *CLASS_HEADER terminal_index[] = "
                         "&(symbol_index[0]);\n");
                mystrcpy("const unsigned short *CLASS_HEADER non_terminal_index[] = "
                         "&(symbol_index[0]);\n");
            }
            num_bytes = 2 * num_symbols;
        }
        /*****************************************************************/
        /* Compute and list space required for SYMBOL_INDEX map.         */
        /*****************************************************************/
        sprintf(msg_line,
                "    Storage required for SYMBOL_INDEX map: %d Bytes",
                num_bytes);
        PRNT(msg_line);
    }
    if (num_scopes > 0)
    {
        short root = 0;
        short *list;
        list = Allocate_short_array(scope_rhs_size + 1);

        for (i = 1; i <= scope_rhs_size; i++)
        {
            if (scope_right_side[i] != 0)
                scope_right_side[i] = symbol_map[scope_right_side[i]];
        }

        for (i = 1; i <= num_scopes; i++)
        {
            scope[i].look_ahead = symbol_map[scope[i].look_ahead];
            if (table_opt == OPTIMIZE_SPACE)
                scope[i].lhs_symbol = symbol_map[scope[i].lhs_symbol]
                                      - num_terminals;
            else
                scope[i].lhs_symbol = symbol_map[scope[i].lhs_symbol];
        }
        /****************************************/
        /* Mark all elements of prefix strings. */
        /****************************************/
        for (i=1; i <= scope_rhs_size; i++)
             list[i] = -1;

        for (i = 1; i <= num_scopes; i++)
        {
            if (list[scope[i].suffix] < 0)
            {
                list[scope[i].suffix] = root;
                root = scope[i].suffix;
            }
        }

        for (; root != 0; root = list[root])
        {
            for (j = root; scope_right_side[j] != 0; j++)
            {
                k = scope_right_side[j];
                scope_right_side[j] = temp[k];
            }
        }
        ffree(list);
    }

    if (java_bit)
         print_java_names();
    else print_c_names();

    if (num_scopes > 0)
    {
        if (scope_rhs_size <= (java_bit ? 127 : 255))
        {
            if (java_bit)
                 mystrcpy("\n    public final static byte scope_prefix[] = {\n");
            else mystrcpy("\nconst unsigned char  CLASS_HEADER scope_prefix[] = {\n");
        }
        else
        {
            if (java_bit)
                 mystrcpy("\n    public final static char scope_prefix[] = {\n");
            else mystrcpy("\nconst unsigned short CLASS_HEADER scope_prefix[] = {\n");
        }

        padline();
        k = 0;
        for (i = 1; i <= num_scopes; i++)
        {
            itoc(scope[i].prefix);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && i != num_scopes)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }
        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                          };\n");

        if (scope_rhs_size <= (java_bit ? 127 : 255))
        {
            if (java_bit)
                 mystrcpy("\n    public final static byte scope_suffix[] = {\n");
            else mystrcpy("\nconst unsigned char  CLASS_HEADER scope_suffix[] = {\n");
        }
        else
        {
            if (java_bit)
                 mystrcpy("\n    public final static char scope_suffix[] = {\n");
            else mystrcpy("\nconst unsigned short CLASS_HEADER scope_suffix[] = {\n");
        }

        padline();
        k = 0;
        for (i = 1; i <= num_scopes; i++)
        {
            itoc(scope[i].suffix);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && i != num_scopes)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }
        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                          };\n");

        if (num_symbols <= (java_bit ? 127 : 255))
        {
            if (java_bit)
                 mystrcpy("\n    public final static byte scope_lhs[] = {\n");
            else mystrcpy("\nconst unsigned char  CLASS_HEADER scope_lhs[] = {\n");
        }
        else
        {
            if (java_bit)
                 mystrcpy("\n    public final static char scope_lhs[] = {\n");
            else mystrcpy("\nconst unsigned short CLASS_HEADER scope_lhs[] = {\n");
        }

        padline();
        k = 0;
        for (i = 1; i <= num_scopes; i++)
        {
            itoc(scope[i].lhs_symbol);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && i != num_scopes)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }
        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                          };\n");

        if (num_terminals <= (java_bit ? 127 : 255))
        {
            if (java_bit)
                 mystrcpy("\n    public final static byte scope_la[] = {\n");
            else mystrcpy("\nconst unsigned char  CLASS_HEADER scope_la[] = {\n");
        }
        else
        {
            if (java_bit)
                 mystrcpy("\n    public final static char scope_la[] = {\n");
            else mystrcpy("\nconst unsigned short CLASS_HEADER scope_la[] = {\n");
        }

        padline();
        k = 0;
        for (i = 1; i <= num_scopes; i++)
        {
            itoc(scope[i].look_ahead);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && i != num_scopes)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }
        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                          };\n");

        if (scope_state_size <= (java_bit ? 127 : 255))
        {
            if (java_bit)
                 mystrcpy("\n    public final static byte scope_state_set[] = {\n");
            else mystrcpy("\nconst unsigned char  CLASS_HEADER scope_state_set[] = {\n");
        }
        else
        {
            if (java_bit)
                 mystrcpy("\n    public final static char scope_state_set[] = {\n");
            else mystrcpy("\nconst unsigned short CLASS_HEADER scope_state_set[] = {\n");
        }

        padline();
        k = 0;
        for (i = 1; i <= num_scopes; i++)
        {
            itoc(scope[i].state_set);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && i != num_scopes)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }
        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                          };\n");

        if (num_symbols <= (java_bit ? 127 : 255))
        {
            if (java_bit)
            {
                prnt_shorts
                    ("\n    public final static byte scope_rhs[] = {0,\n",
                     1, scope_rhs_size, 10, scope_right_side);
            }
            else
            {
                prnt_shorts
                    ("\nconst unsigned char  CLASS_HEADER scope_rhs[] = {0,\n",
                     1, scope_rhs_size, 10, scope_right_side);
            }
        }
        else
        {
            if (java_bit)
            {
                prnt_shorts
                    ("\n    public final static char scope_rhs[] = {0,\n",
                     1, scope_rhs_size, 10, scope_right_side);
            }
            else
            {
                prnt_shorts
                    ("\nconst unsigned short CLASS_HEADER scope_rhs[] = {0,\n",
                     1, scope_rhs_size, 10, scope_right_side);
            }
        }

        if (java_bit)
             mystrcpy("\n    public final static char scope_state[] = {0,\n");
        else mystrcpy("\nconst unsigned short CLASS_HEADER scope_state[] = {0,\n");

        padline();
        k = 0;
        for (i = 1; i <= scope_state_size; i++)
        {
            if (scope_state[i] == 0)
                itoc(0);
            else
                itoc(state_index[scope_state[i]] + num_rules);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && i != scope_state_size)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }
        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                          };\n");

        if (num_symbols <= (java_bit ? 127 : 255))
        {
            if (java_bit)
                 mystrcpy("\n    public final static byte in_symb[] = {0,\n");
            else mystrcpy("\nconst unsigned char  CLASS_HEADER in_symb[] = {0,\n");
        }
        else
        {
            if (java_bit)
                 mystrcpy("\n    public final static char in_symb[] = {0,\n");
            else mystrcpy("\nconst unsigned short CLASS_HEADER in_symb[] = {0,\n");
        }

        /* Transition symbol */
        padline();
        *output_ptr++ = '0';
        *output_ptr++ = COMMA;
        k = 1;
        for (state_no = 2; state_no <= (int) num_states; state_no++)
        {
            struct node *q;
            int item_no;

            q = statset[state_no].kernel_items;
            if (q != NULL)
            {
                item_no = q -> value - 1;
                i = item_table[item_no].symbol;
            }
            else i = 0;

            itoc(symbol_map[i]);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && state_no != (int) num_states)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }
        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                          };\n");
    }

    return;
}


/****************************************************************************/
/*                               PRINT_SYMBOLS:                             */
/****************************************************************************/
static void print_symbols(void)
{
    int symbol;
    char line[SYMBOL_SIZE +       /* max length of a token symbol  */
              2 * MAX_PARM_SIZE + /* max length of prefix + suffix */
              64];                /* +64 for error messages lines  */
                                  /* or other fillers(blank, =,...)*/
    if (java_bit)
    {
         strcpy(line, "interface ");
         strcat(line, sym_tag);
         strcat(line, "\n{\n    public final static int\n");
    }
    else strcpy(line, "enum {\n");

    /*********************************************************/
    /* We write the terminal symbols map.                    */
    /*********************************************************/
    for ALL_TERMINALS(symbol)
    {
        char *tok = RETRIEVE_STRING(symbol);

        fprintf(syssym, "%s", line);

        if (tok[0] == '\n' || tok[0] == escape)
        {
            tok[0] = escape;
            sprintf(line, "Escaped symbol %s is an invalid C variable.\n",tok);
            PRNT(line);
        }
        else if (strpbrk(tok, "!%^&*()-+={}[];:\"`~|\\,.<>/?\'") != NULL)
        {
            sprintf(line, "%s may be an invalid variable name.\n", tok);
            PRNT(line);
        }

        sprintf(line, "      %s%s%s = %i,\n\0",
                      prefix, tok, suffix, symbol_map[symbol]);

        if (c_bit || cpp_bit)
        {
            while(strlen(line) > PARSER_LINE_SIZE)
            {
                fwrite(line, sizeof(char), PARSER_LINE_SIZE - 2, syssym);
                fprintf(syssym, "\\\n");
                strcpy(line, &line[PARSER_LINE_SIZE - 2]);
            }
        }
    }

    line[strlen(line) - 2] = '\0'; /* remove the string ",\n" from last line */
    fprintf(syssym, "%s%s", line, (java_bit ? ";\n}\n" : "\n     };\n"));

    return;
}


/****************************************************************************/
/*                            PRINT_DEFINITIONS:                            */
/****************************************************************************/
static void print_definitions(void)
{
    if (java_bit)
         fprintf(sysdef, "interface %s\n{\n    public final static int\n\n",
                         def_tag);
    else fprintf(sysdef, "enum {\n");

    if (error_maps_bit)
    {
        if (java_bit)
             fprintf(sysdef,
                     "      ERROR_SYMBOL      = %d,\n"
                     "      MAX_NAME_LENGTH   = %d,\n"
                     "      NUM_STATES        = %d,\n\n",
                     error_image,
                     max_name_length,
                     num_states);
        else
	{
	    if (! jikes_bit)
		fprintf(sysdef,
			"      ERROR_CODE,\n"
			"      BEFORE_CODE,\n"
			"      INSERTION_CODE,\n"
			"      INVALID_CODE,\n"
			"      SUBSTITUTION_CODE,\n"
			"      DELETION_CODE,\n"
			"      MERGE_CODE,\n"
			"      MISPLACED_CODE,\n"
			"      SCOPE_CODE,\n"
			"      MANUAL_CODE,\n"
			"      SECONDARY_CODE,\n"
			"      EOF_CODE,\n\n");
	    fprintf(sysdef,
                     "      ERROR_SYMBOL      = %d,\n"
                     "      MAX_DISTANCE      = %d,\n"
                     "      MIN_DISTANCE      = %d,\n"
                     "      MAX_NAME_LENGTH   = %d,\n"
                     "      MAX_TERM_LENGTH   = %d,\n"
                     "      NUM_STATES        = %d,\n\n",

                     error_image,
                     maximum_distance,
                     minimum_distance,
                     max_name_length,
                     max_name_length,
                     num_states);
	}
    }

    if (java_bit)
         fprintf(sysdef,
                 "      NT_OFFSET         = %d,\n"
                 "      SCOPE_UBOUND      = %d,\n"
                 "      SCOPE_SIZE        = %d,\n"
                 "      LA_STATE_OFFSET   = %d,\n"
                 "      MAX_LA            = %d,\n"
                 "      NUM_RULES         = %d,\n"
                 "      NUM_TERMINALS     = %d,\n"
                 "      NUM_NON_TERMINALS = %d,\n"
                 "      NUM_SYMBOLS       = %d,\n"
                 "      START_STATE       = %d,\n"
                 "      EOFT_SYMBOL       = %d,\n"
                 "      EOLT_SYMBOL       = %d,\n"
                 "      ACCEPT_ACTION     = %d,\n"
                 "      ERROR_ACTION      = %d;\n"
                 "};\n\n",


                 (table_opt == OPTIMIZE_SPACE ? num_terminals : num_symbols),
                 num_scopes - 1,
                 num_scopes,
                 (read_reduce_bit && lalr_level > 1
                                   ? error_act + num_rules : error_act),
                 lalr_level,
                 num_rules,
                 num_terminals,
                 num_non_terminals,
                 num_symbols,
                 state_index[1] + num_rules,
                 eoft_image,
                 eolt_image,
                 accept_act,
                 error_act);
    else fprintf(sysdef,
                 "      NT_OFFSET         = %d,\n"
                 "      BUFF_UBOUND       = %d,\n"
                 "      BUFF_SIZE         = %d,\n"
                 "      STACK_UBOUND      = %d,\n"
                 "      STACK_SIZE        = %d,\n"
                 "      SCOPE_UBOUND      = %d,\n"
                 "      SCOPE_SIZE        = %d,\n"
                 "      LA_STATE_OFFSET   = %d,\n"
                 "      MAX_LA            = %d,\n"
                 "      NUM_RULES         = %d,\n"
                 "      NUM_TERMINALS     = %d,\n"
                 "      NUM_NON_TERMINALS = %d,\n"
                 "      NUM_SYMBOLS       = %d,\n"
                 "      START_STATE       = %d,\n"
                 "      EOFT_SYMBOL       = %d,\n"
                 "      EOLT_SYMBOL       = %d,\n"
                 "      ACCEPT_ACTION     = %d,\n"
                 "      ERROR_ACTION      = %d\n"
                 "     };\n\n",


                 (table_opt == OPTIMIZE_SPACE ? num_terminals : num_symbols),
                 maximum_distance + lalr_level - 1,
                 maximum_distance + lalr_level,
                 stack_size - 1,
                 stack_size,
                 num_scopes - 1,
                 num_scopes,
                 (read_reduce_bit && lalr_level > 1
                                   ? error_act + num_rules : error_act),
                 lalr_level,
                 num_rules,
                 num_terminals,
                 num_non_terminals,
                 num_symbols,
                 state_index[1] + num_rules,
                 eoft_image,
                 eolt_image,
                 accept_act,
                 error_act);

    return;
}


/****************************************************************************/
/*                               PRINT_EXTERNS:                             */
/****************************************************************************/
static void print_externs(void)
{
    if (c_bit || cpp_bit)
    {
        fprintf(sysprs,
                "%s SCOPE_REPAIR\n"
                "%s DEFERRED_RECOVERY\n"
                "%s FULL_DIAGNOSIS\n"
                "%s SPACE_TABLES\n\n",

                (num_scopes > 0              ? "#define" : "#undef "),
                (deferred_bit                ? "#define" : "#undef "),
                (error_maps_bit              ? "#define" : "#undef "),
                (table_opt == OPTIMIZE_SPACE ? "#define" : "#undef "));
    }

    if (c_bit)
        fprintf(sysprs,
            "#define original_state(state) (-%s[state])\n"
            "#define asi(state)            asb[original_state(state)]\n"
            "#define nasi(state)           nasb[original_state(state)]\n"
            "#define in_symbol(state)      in_symb[original_state(state)]\n\n",
            (table_opt == OPTIMIZE_TIME  ? "check" : "base_check"));
    else if (cpp_bit)
    {
        fprintf(sysprs,
                "class LexStream;\n\n"
                "class %s_table\n"
                "{\n"
                "public:\n",

                prs_tag);

        if (error_maps_bit || debug_bit)
            fprintf(sysprs,
                    "    static int original_state(int state) "
                    "{ return -%s[state]; }\n",

                    (table_opt == OPTIMIZE_TIME  ? "check" : "base_check"));

        if (error_maps_bit)
        {
            fprintf(sysprs,
                    "    static int asi(int state) "
                    "{ return asb[original_state(state)]; }\n"
                    "    static int nasi(int state) "
                    "{ return nasb[original_state(state)]; }\n");
            if (num_scopes > 0)
                fprintf(sysprs,
                        "    static int in_symbol(int state) "
                        "{ return in_symb[original_state(state)]; }\n");
        }

        fprintf(sysprs, "\n");
    }
    else if (java_bit)
    {
        fprintf(sysprs,
                "abstract class %s extends %s implements %s\n{\n",
                prs_tag, dcl_tag, def_tag);

        if (error_maps_bit || debug_bit)
        {
            fprintf(sysprs,
                    "    public final static int original_state(int state) "
                    "{ return -%s(state); }\n",

                    (table_opt == OPTIMIZE_TIME  ? "check" : "base_check"));

            if (error_maps_bit)
            {
                fprintf(sysprs,
                        "    public final static int asi(int state) "
                        "{ return asb[original_state(state)]; }\n"
                        "    static int nasi(int state) "
                        "{ return nasb[original_state(state)]; }\n");
                if (num_scopes > 0)
                    fprintf(sysprs,
                            "    public final static int in_symbol(int state) "
                            "{ return in_symb[original_state(state)]; }\n");
            }
    
            fprintf(sysprs, "\n");
        }
    }

    if (c_bit || cpp_bit)
    {
        fprintf(sysprs, "%s const unsigned char  rhs[];\n",
                        (c_bit ? "extern" : "    static"));

        if (check_size > 0 || table_opt == OPTIMIZE_TIME)
        {
            BOOLEAN small = byte_check_bit && (! error_maps_bit);

            fprintf(sysprs, "%s const %s check_table[];\n"
                            "%s const %s *%s;\n",

                            (c_bit ? "extern" : "    static"),
                            (small ? "unsigned char " : "  signed short"),
                            (c_bit ? "extern" : "    static"),
                            (small ? "unsigned char " : "  signed short"),
                            (table_opt == OPTIMIZE_TIME
                                        ? "check" : "base_check"));
        }

        fprintf(sysprs, "%s const unsigned short lhs[];\n"
                        "%s const unsigned short *%s;\n",

                        (c_bit ? "extern" : "    static"),
                        (c_bit ? "extern" : "    static"),
                        (table_opt == OPTIMIZE_TIME
                                    ? "action" : "base_action"));

        if (goto_default_bit)
            fprintf(sysprs, "%s const unsigned short default_goto[];\n",
                            (c_bit ? "extern" : "    static"));

        if (table_opt == OPTIMIZE_SPACE)
        {
            fprintf(sysprs, "%s const unsigned %s term_check[];\n",
                            (c_bit ? "extern" : "    static"),
                            (num_terminals <= (java_bit ? 127 : 255) ? "char " : "short"));
            fprintf(sysprs, "%s const unsigned short term_action[];\n",
                            (c_bit ? "extern" : "    static"));

            if (shift_default_bit)
            {
                fprintf(sysprs, "%s const unsigned short default_reduce[];\n",
                                (c_bit ? "extern" : "    static"));
                fprintf(sysprs, "%s const unsigned short shift_state[];\n",
                                (c_bit ? "extern" : "    static"));
                fprintf(sysprs, "%s const unsigned %s shift_check[];\n",
                                (c_bit ? "extern" : "    static"),
                                (num_terminals <= (java_bit ? 127 : 255) ? "char " : "short"));
                fprintf(sysprs, "%s const unsigned short default_shift[];\n",
                                (c_bit ? "extern" : "    static"));
            }
        }

        if (error_maps_bit)
        {
            fprintf(sysprs,
                    "\n"
                    "%s const unsigned short asb[];\n"
                    "%s const unsigned %s asr[];\n"
                    "%s const unsigned short nasb[];\n"
                    "%s const unsigned short nasr[];\n"
                    "%s const unsigned short name_start[];\n"
                    "%s const unsigned char  name_length[];\n"
                    "%s const          char  string_buffer[];\n",

                    (c_bit ? "extern" : "    static"),
                    (c_bit ? "extern" : "    static"),
                    (byte_terminal_range <= (java_bit ? 127 : 255) ? "char " : "short"),
                    (c_bit ? "extern" : "    static"),
                    (c_bit ? "extern" : "    static"),
                    (c_bit ? "extern" : "    static"),
                    (c_bit ? "extern" : "    static"),
                    (c_bit ? "extern" : "    static"));

            if (table_opt == OPTIMIZE_SPACE)
            {
                fprintf(sysprs,
                        "%s const unsigned %s terminal_index[];\n"
                        "%s const unsigned %s non_terminal_index[];\n",
                        (c_bit ? "extern" : "    static"),
                        (num_names <= (java_bit ? 127 : 255) ? "char " : "short"),
                        (c_bit ? "extern" : "    static"),
                        (num_names <= (java_bit ? 127 : 255) ? "char " : "short"));
            }
            else
            {
                fprintf(sysprs, "%s const unsigned %s symbol_index[];\n"
                                "%s const unsigned %s *terminal_index;\n"
                                "%s const unsigned %s *non_terminal_index;\n",
                                (c_bit ? "extern" : "    static"),
                                (num_names <= (java_bit ? 127 : 255) ? "char " : "short"),
                                (c_bit ? "extern" : "    static"),
                                (num_names <= (java_bit ? 127 : 255) ? "char " : "short"),
                                (c_bit ? "extern" : "    static"),
                                (num_names <= (java_bit ? 127 : 255) ? "char " : "short"));
            }

            if (num_scopes > 0)
            {
                fprintf(sysprs, "%s const unsigned %s scope_prefix[];\n"
                                "%s const unsigned %s scope_suffix[];\n"
                                "%s const unsigned %s scope_lhs[];\n"
                                "%s const unsigned %s scope_la[];\n"
                                "%s const unsigned %s scope_state_set[];\n"
                                "%s const unsigned %s scope_rhs[];\n"
                                "%s const unsigned short scope_state[];\n"
                                "%s const unsigned %s in_symb[];\n",

                                (c_bit ? "extern" : "    static"),
                                (scope_rhs_size    <= (java_bit ? 127 : 255) ? "char " : "short"),
                                (c_bit ? "extern" : "    static"),
                                (scope_rhs_size    <= (java_bit ? 127 : 255) ? "char " : "short"),
                                (c_bit ? "extern" : "    static"),
                                (num_symbols       <= (java_bit ? 127 : 255) ? "char " : "short"),
                                (c_bit ? "extern" : "    static"),
                                (num_terminals     <= (java_bit ? 127 : 255) ? "char " : "short"),
                                (c_bit ? "extern" : "    static"),
                                (scope_state_size  <= (java_bit ? 127 : 255) ? "char " : "short"),
                                (c_bit ? "extern" : "    static"),
                                (num_symbols       <= (java_bit ? 127 : 255) ? "char " : "short"),
                                (c_bit ? "extern" : "    static"),
                                (c_bit ? "extern" : "    static"),
                                (num_symbols       <= (java_bit ? 127 : 255) ? "char " : "short"));
            }
        }

        fprintf(sysprs, "\n");
    }

    if (table_opt == OPTIMIZE_SPACE)
    {
        if (goto_default_bit)
            non_terminal_space_action();
        else
            non_terminal_no_goto_default_space_action();

        if (lalr_level > 1)
        {
            if (shift_default_bit)
                terminal_shift_default_space_lalr_k();
            else
                terminal_space_lalr_k();
        }
        else
        {
            if (shift_default_bit)
                terminal_shift_default_space_action();
            else
                terminal_space_action();
        }
    }
    else
    {
        if (goto_default_bit)
            non_terminal_time_action();
        else
            non_terminal_no_goto_default_time_action();

        if (lalr_level > 1)
            terminal_time_lalr_k();
        else
            terminal_time_action();
    }

    if (cpp_bit)
        fprintf(sysprs, "};\n");
    else if (java_bit)
        fprintf(sysprs, "}\n");

    return;
}


/**************************************************************************/
/*                           PRINT_SPACE_TABLES:                          */
/**************************************************************************/
static void print_space_tables(void)
{
    int *check,
        *action;

    int la_state_offset,
        i,
        j,
        k,
        indx,
        act,
        result_act,
        default_count = 0,
        goto_count = 0,
        goto_reduce_count = 0,
        reduce_count = 0,
        la_shift_count = 0,
        shift_count = 0,
        shift_reduce_count = 0;

    int rule_no,
        symbol,
        state_no;

    long offset;

    check = Allocate_int_array(table_size + 1);
    action = Allocate_int_array(table_size + 1);

    output_ptr = &output_buffer[0];

    /******************************************************************/
    /* Prepare header card with proper information, and write it out. */
    /******************************************************************/
    offset = error_act;

    if (lalr_level > 1)
    {
        if (read_reduce_bit)
            offset += num_rules;
        la_state_offset = offset;
    }
    else
        la_state_offset = error_act;

    if (offset > (MAX_TABLE_SIZE + 1))
    {
        sprintf(msg_line, "Table contains entries that are > "
                "%d; Processing stopped.", MAX_TABLE_SIZE + 1);
        PRNTERR(msg_line);
        exit(12);
    }

    for (i = 1; i <= check_size; i++)
         check[i] = DEFAULT_SYMBOL;

    for (i = 1; i <= (int) action_size; i++)
         action[i] = error_act;

    /********************************************************************/
    /*    Update the default non-terminal action of each state with the */
    /* appropriate corresponding terminal state starting index.         */
    /********************************************************************/
    for (i = 1; i <= num_terminal_states; i++)
    {
        indx = term_state_index[i];
        state_no = new_state_element[i].image;

    /*********************************************************************/
    /*   Update the action link between the non-terminal and terminal    */
    /* tables.  If error-maps are requested, an indirect linking is made */
    /* as follows:                                                       */
    /*  Each non-terminal row identifies its original state number, and  */
    /* a new vector START_TERMINAL_STATE indexable by state numbers      */
    /* identifies the starting point of each state in the terminal table.*/
    /*********************************************************************/
        if (state_no <= (int) num_states)
        {
            for (; state_no != NIL; state_no = state_list[state_no])
                action[state_index[state_no]] = indx;
        }
        else
        {
            for (; state_no != NIL; state_no = state_list[state_no])
            {
                act = la_state_offset + indx;
                state_index[state_no] = act;
            }
        }
    }

    /*********************************************************************/
    /*  Now update the non-terminal tables with the non-terminal actions.*/
    /*********************************************************************/
    for ALL_STATES(state_no)
    {
        struct goto_header_type go_to;

        indx = state_index[state_no];
        go_to = statset[state_no].go_to;
        for (j = 1; j <= go_to.size; j++)
        {
            symbol = GOTO_SYMBOL(go_to, j);
            i = indx + symbol;
            if (goto_default_bit || nt_check_bit)
                check[i] = symbol;
            act = GOTO_ACTION(go_to, j);
            if (act > 0)
            {
                action[i] = state_index[act] + num_rules;
                goto_count++;
            }
            else
            {
                action[i] = -act;
                goto_reduce_count++;
            }
        }
    }

    if (error_maps_bit || debug_bit)
    {
        if (check_size == 0)
        {
            check_size = action_size;
            for (i = 0; i <= check_size; i++)
                check[i] = 0;
        }

        for ALL_STATES(state_no)
            check[state_index[state_no]] = -state_no;
    }
    for (i = 1; i <= check_size; i++)
    {
        if (check[i] < 0 || check[i] > (java_bit ? 127 : 255))
             byte_check_bit = 0;
    }

    if (c_bit)
        mystrcpy("\n#define CLASS_HEADER\n\n");
    else if (cpp_bit)
    {
        mystrcpy("\n#define CLASS_HEADER ");
        mystrcpy(prs_tag);
        mystrcpy("_table::\n\n");
    }
    else
    {
        mystrcpy("abstract class ");
        mystrcpy(dcl_tag);
        mystrcpy(" implements ");
        mystrcpy(def_tag);
        mystrcpy("\n{\n");
    }

    /*********************************************************************/
    /* Write size of right hand side of rules followed by CHECK table.   */
    /*********************************************************************/
    if (java_bit)
         mystrcpy("    public final static byte rhs[] = {0,\n");
    else mystrcpy("const unsigned char  CLASS_HEADER rhs[] = {0,\n");

    padline();
    k = 0;
    for (i = 1; i <= num_rules; i++)
    {
        k++;
        if (k > 15)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(sysdcl);
            padline();
            k = 1;
        }
        itoc(RHS_SIZE(i));
        *output_ptr++ = COMMA;
    }
    *(output_ptr - 1) = '\n';
    BUFFER_CHECK(sysdcl);

    if (java_bit)
         mystrcpy("    };\n");
    else mystrcpy("                 };\n");

    *output_ptr++ = '\n';

    if (check_size > 0)
    {
        if (byte_check_bit && ! error_maps_bit)
        {
            if (java_bit)
                 mystrcpy("    public final static byte check_table[] = {\n");
            else mystrcpy("const unsigned char  CLASS_HEADER check_table[] = {\n");
        }
        else
        {
            if (java_bit)
                 mystrcpy("    public final static short check_table[] = {\n");
            else mystrcpy("const   signed short CLASS_HEADER check_table[] = {\n");
        }

        padline();
        k = 0;
        for (i = 1; i <= check_size; i++)
        {
            k++;
            if (k > 10)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 1;
            }
            itoc(check[i]);
            *output_ptr++ = COMMA;
        }
        *(output_ptr - 1) = '\n';
        BUFFER_CHECK(sysdcl);

        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                 };\n");
        *output_ptr++ = '\n';

        if (byte_check_bit && (! error_maps_bit))
        {
            if (java_bit)
                 mystrcpy("    public final static byte base_check(int i)"
                          "\n    {\n        return check_table[i - (NUM_RULES + 1)];\n    }\n");
            else mystrcpy("const unsigned char  *CLASS_HEADER base_check = "
                          "&(check_table[0]) - (NUM_RULES + 1);\n");
        }
        else
        {
            if (java_bit)
                 mystrcpy("    public final static short base_check(int i) "
                          "\n    {\n        return check_table[i - (NUM_RULES + 1)];\n    }\n");
            else mystrcpy("const   signed short *CLASS_HEADER base_check = "
                          "&(check_table[0]) - (NUM_RULES + 1);\n");
        }
        *output_ptr++ = '\n';
    }

    /*********************************************************************/
    /* Write left hand side symbol of rules followed by ACTION table.    */
    /*********************************************************************/
    if (java_bit)
         mystrcpy("    public final static char lhs[] = {0,\n");
    else mystrcpy("const unsigned short CLASS_HEADER lhs[] = {0,\n");

    padline();
    k = 0;
    for (i = 1; i <= num_rules; i++)
    {
        itoc(symbol_map[rules[i].lhs] - num_terminals);
        *output_ptr++ = COMMA;
        k++;
        if (k == 15)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(sysdcl);
            padline();
            k = 0;
        }
    }

    *output_ptr++ = '\n';
    *output_ptr++ = '\n';
    BUFFER_CHECK(sysdcl);
    padline();
    k = 0;

    if (error_maps_bit)
    {
        int max_indx;

        max_indx = accept_act - num_rules - 1;
        for (i = 1; i <= max_indx; i++)
            check[i] = OMEGA;
        for ALL_STATES(state_no)
            check[state_index[state_no]] = state_no;

        j = num_states + 1;
        for (i = max_indx; i >= 1; i--)
        {
            state_no = check[i];
            if (state_no != OMEGA)
            {
                j--;
                ordered_state[j] = i + num_rules;
                state_list[j] = state_no;
            }
        }
    }

    for (i = 1; i <= (int) action_size; i++)
    {
        itoc(action[i]);
        *output_ptr++ = COMMA;
        k++;
        if (k == 10 && i != (int) action_size)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(sysdcl);
            padline();
            k = 0;
        }
    }

    if (k != 0)
    {
        *(output_ptr - 1) = '\n';
        BUFFER_CHECK(sysdcl);
    }
    if (java_bit)
         mystrcpy("    };\n");
    else mystrcpy("                 };\n");
    *output_ptr++ = '\n';
    BUFFER_CHECK(sysdcl);

    if (java_bit)
         mystrcpy("    public final static char base_action[] = lhs;\n");
    else mystrcpy("const unsigned short *CLASS_HEADER base_action = lhs;\n");
    *output_ptr++ = '\n';

    /********************************************************************/
    /* Initialize the terminal tables,and update with terminal actions. */
    /********************************************************************/
    for (i = 1; i <= term_check_size; i++)
        check[i] = DEFAULT_SYMBOL;

    for (i = 1; i <= term_action_size; i++)
       action[i] = error_act;

    for (state_no = 1; state_no <= num_terminal_states; state_no++)
    {
        struct shift_header_type  sh;
        struct reduce_header_type red;

        indx = term_state_index[state_no];
        sh = shift[new_state_element[state_no].shift_number];
        for (j = 1; j <= sh.size; j++)
        {
            symbol = SHIFT_SYMBOL(sh, j);
            act = SHIFT_ACTION(sh, j);
            if ((! shift_default_bit) || (act != shiftdf[symbol]))
            {
                i = indx + symbol;
                check[i] = symbol;

                if (act > (int) num_states)
                {
                    result_act = state_index[act];
                    la_shift_count++;
                }
                else if (act > 0)
                {
                    result_act = state_index[act] + num_rules;
                    shift_count++;
                }
                else
                {
                    result_act = -act + error_act;
                    shift_reduce_count++;
                }

                if (result_act > (MAX_TABLE_SIZE + 1))
                {
                    sprintf(msg_line,
                        "Table contains look-ahead shift entry that is >"
                        " %d; Processing stopped.", MAX_TABLE_SIZE + 1);
                    PRNTERR(msg_line);
                    return;
                }

                action[i] = result_act;
            }
        }

        red = new_state_element[state_no].reduce;
        for (j = 1; j <= red.size; j++)
        {
            symbol = REDUCE_SYMBOL(red, j);
            rule_no = REDUCE_RULE_NO(red, j);
            i = indx + symbol;
            check[i] = symbol;
            action[i] = rule_no;
            reduce_count++;
        }

        rule_no = REDUCE_RULE_NO(red, 0);
        if (rule_no != error_act)
            default_count++;
        check[indx] = DEFAULT_SYMBOL;
        if (shift_default_bit)
            action[indx] = state_no;
        else
            action[indx] = rule_no;
    }

    PRNT("\n\nActions in Compressed Tables:");
    sprintf(msg_line,"     Number of Shifts: %d",shift_count);
    PRNT(msg_line);

    sprintf(msg_line,"     Number of Shift/Reduces: %d",shift_reduce_count);
    PRNT(msg_line);

    if (max_la_state > num_states)
    {
        sprintf(msg_line,
                "     Number of Look-Ahead Shifts: %d",
                la_shift_count);
        PRNT(msg_line);
    }

    sprintf(msg_line,"     Number of Gotos: %d",goto_count);
    PRNT(msg_line);

    sprintf(msg_line,"     Number of Goto/Reduces: %d",goto_reduce_count);
    PRNT(msg_line);

    sprintf(msg_line,"     Number of Reduces: %d",reduce_count);
    PRNT(msg_line);

    sprintf(msg_line,"     Number of Defaults: %d",default_count);
    PRNT(msg_line);

    /********************************************************************/
    /* Write Terminal Check Table.                                      */
    /********************************************************************/
    if (num_terminals <= (java_bit ? 127 : 255))
    {
        if (java_bit)
             prnt_ints("\n    public final static byte term_check[] = {0,\n",
                       1, term_check_size, 15, check);
        else prnt_ints("\nconst unsigned char  CLASS_HEADER term_check[] = {0,\n",
                       1, term_check_size, 15, check);
    }
    else
    {
        if (java_bit)
             prnt_ints("\n    public final static char term_check[] = {0,\n",
                       1, term_check_size, 15, check);
        else prnt_ints("\nconst unsigned short CLASS_HEADER term_check[] = {0,\n",
                       1, term_check_size, 15, check);
    }

    /********************************************************************/
    /* Write Terminal Action Table.                                      */
    /********************************************************************/
    if (java_bit)
         prnt_ints("\n    public final static char term_action[] = {0,\n",
                   1, term_action_size, 10, action);
    else prnt_ints("\nconst unsigned short CLASS_HEADER term_action[] = {0,\n",
                   1, term_action_size, 10, action);

    /********************************************************************/
    /* If GOTO_DEFAULT is requested, we print out the GOTODEF vector.   */
    /********************************************************************/
    if (goto_default_bit)
    {
        if (java_bit)
             mystrcpy("\n    public final static char default_goto[] = {0,\n");
        else mystrcpy("\nconst unsigned short CLASS_HEADER default_goto[] = {0,\n");

        padline();
        k = 0;
        for ALL_NON_TERMINALS(symbol)
        {
           act = gotodef[symbol];

           if (act < 0)
               result_act = -act;
           else if (act == 0)
               result_act = error_act;
           else
               result_act = state_index[act] + num_rules;

           itoc(result_act);
           *output_ptr++ = COMMA;
           k++;
           if (k == 10 && symbol != num_symbols)
           {
               *output_ptr++ = '\n';
               BUFFER_CHECK(sysdcl);
               padline();
               k = 0;
           }
       }

       if (k != 0)
       {
           *(output_ptr - 1) = '\n';
           BUFFER_CHECK(sysdcl);
       }
       if (java_bit)
            mystrcpy("    };\n");
       else mystrcpy("                 };\n");
    }

    if (shift_default_bit)
    {
        if (java_bit)
             mystrcpy("\n    public final static char default_reduce[] = {0,\n");
        else mystrcpy("\nconst unsigned short CLASS_HEADER default_reduce[] = {0,\n");

        padline();
        k = 0;
        for (i = 1; i <= num_terminal_states; i++)
        {
            struct reduce_header_type red;

            red = new_state_element[i].reduce;
            itoc(REDUCE_RULE_NO(red, 0));
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && i != num_terminal_states)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }

        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                 };\n");

        if (java_bit)
             mystrcpy("\n    public final static char shift_state[] = {0,\n");
        else mystrcpy("\nconst unsigned short CLASS_HEADER shift_state[] = {0,\n");

        padline();
        k = 0;
        for (i = 1; i <= num_terminal_states; i++)
        {
            itoc(shift_check_index[shift_image[i]]);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && i != num_terminal_states)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }

        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                 };\n");

        for (i = 1; i <= shift_check_size; i++)
            check[i] = DEFAULT_SYMBOL;

        for (i = 1; i <= shift_domain_count; i++)
        {
            struct shift_header_type sh;

            indx = shift_check_index[i];
            sh = shift[real_shift_number[i]];
            for (j = 1; j <= sh.size; j++)
            {
                symbol = SHIFT_SYMBOL(sh, j);
                check[indx + symbol] = symbol;
            }
        }

        if (num_terminals <= (java_bit ? 127 : 255))
        {
            if (java_bit)
                 mystrcpy("\n    public final static byte shift_check[] = {0,\n");
            else mystrcpy("\nconst unsigned char  CLASS_HEADER shift_check[] = {0,\n");
        }
        else
        {
            if (java_bit)
                 mystrcpy("\n    public final static char shift_check[] = {0,\n");
            else mystrcpy("\nconst unsigned short CLASS_HEADER shift_check[] = {0,\n");
        }

        padline();
        k = 0;
        for (i = 1; i <= shift_check_size; i++)
        {
            itoc(check[i]);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && i != shift_check_size)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }

        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                 };\n");

        if (java_bit)
             mystrcpy("\n    public final static char default_shift[] = {0,\n");
        else mystrcpy("\nconst unsigned short CLASS_HEADER default_shift[] = {0,\n");

        padline();
        k = 0;
        for ALL_TERMINALS(symbol)
        {
            act = shiftdf[symbol];
            if (act < 0)
                result_act = -act + error_act;
            else if (act == 0)
                result_act = error_act;
            else if (act > (int) num_states)
                result_act = state_index[act];
            else
                result_act = state_index[act] + num_rules;

            if (result_act > (MAX_TABLE_SIZE + 1))
            {
                sprintf(msg_line,
                    "Table contains look-ahead shift entry that is >"
                    " %d; Processing stopped.", MAX_TABLE_SIZE + 1);
                PRNTERR(msg_line);
                return;
            }

            itoc(result_act);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && i != num_terminals)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }

        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                 };\n");
    }

    ffree(check);
    ffree(action);

    if (error_maps_bit)
        print_error_maps();

    if (! byte_check_bit)
    {
        if (java_bit)
        {
            PRNT("\n***Warning: Base Check vector contains value "
                 "> 127. 16-bit words used.");
        }
        else
        {
            PRNT("\n***Warning: Base Check vector contains value "
                 "> 255. 16-bit words used.");
        }
    }

    if (! byte_terminal_range)
    {
        if (java_bit)
        {
            PRNT("***Warning: Terminal symbol > 127. 16-bit words used.");
        }
        else
        {
            PRNT("***Warning: Terminal symbol > 255. 16-bit words used.");
        }
    }

    if (java_bit)
        mystrcpy("}\n");

    fwrite(output_buffer, sizeof(char),
           output_ptr - &output_buffer[0], sysdcl);

    return;
}


/**************************************************************************/
/*                         PRINT_TIME_TABLES:                             */
/**************************************************************************/
static void print_time_tables(void)
{
    int *action,
        *check;

    int la_shift_count = 0,
        shift_count = 0,
        goto_count = 0,
        default_count = 0,
        reduce_count = 0,
        shift_reduce_count = 0,
        goto_reduce_count = 0;

    int indx,
        la_state_offset,
        act,
        result_act,
        i,
        j,
        k,
        symbol,
        state_no;

    short default_rule;

    long offset;

    state_list = Allocate_short_array(max_la_state + 1);

    output_ptr = &output_buffer[0];

    check  = next;
    action = previous;

    offset = error_act;

    if (lalr_level > 1)
    {
        if (read_reduce_bit)
            offset += num_rules;
        la_state_offset = offset;
    }
    else
        la_state_offset = error_act;

    if (offset > (MAX_TABLE_SIZE + 1))
    {
        sprintf(msg_line, "Table contains entries that are > "
                "%d; Processing stopped.", MAX_TABLE_SIZE + 1);
        PRNTERR(msg_line);
        exit(12);
    }

/*********************************************************************/
/* Initialize all unfilled slots with default values.                */
/* RECALL that the vector "check" is aliased to the vector "next".   */
/*********************************************************************/
    indx = first_index;
    for (i = indx; (i != NIL) && (i <= (int) action_size); i = indx)
    {
        indx = next[i];

        check[i]  = DEFAULT_SYMBOL;
        action[i] = error_act;
    }
    for (i = (int) action_size + 1; i <= (int) table_size; i++)
        check[i] = DEFAULT_SYMBOL;

/*********************************************************************/
/* We set the rest of the table with the proper table entries.       */
/*********************************************************************/
    for (state_no = 1; state_no <= (int) max_la_state; state_no++)
    {
        struct goto_header_type   go_to;
        struct shift_header_type  sh;
        struct reduce_header_type red;

        indx = state_index[state_no];
        if (state_no > (int) num_states)
        {
            sh  = shift[lastats[state_no].shift_number];
            red = lastats[state_no].reduce;
        }
        else
        {
            go_to = statset[state_no].go_to;
            for (j = 1; j <= go_to.size; j++)
            {
                symbol = GOTO_SYMBOL(go_to, j);
                i = indx + symbol;
                if (goto_default_bit || nt_check_bit)
                    check[i] = symbol;
                else
                    check[i] = DEFAULT_SYMBOL;
                act = GOTO_ACTION(go_to, j);
                if (act > 0)
                {
                    action[i] = state_index[act] + num_rules;
                    goto_count++;
                }
                else
                {
                    action[i] = -act;
                    goto_reduce_count++;
                }
            }
            sh = shift[statset[state_no].shift_number];
            red = reduce[state_no];
        }

        for (j = 1; j <= sh.size; j++)
        {
            symbol = SHIFT_SYMBOL(sh, j);
            i = indx + symbol;
            check[i] = symbol;
            act = SHIFT_ACTION(sh, j);
            if (act > (int) num_states)
            {
                result_act = la_state_offset + state_index[act];
                la_shift_count++;
            }
            else if (act > 0)
            {
                result_act = state_index[act] + num_rules;
                shift_count++;
            }
            else
            {
                result_act = -act + error_act;
                shift_reduce_count++;
            }

            if (result_act > (MAX_TABLE_SIZE + 1))
            {
                sprintf(msg_line,
                    "Table contains look-ahead shift entry that is >"
                    " %d; Processing stopped.", MAX_TABLE_SIZE + 1);
                PRNTERR(msg_line);
                return;
            }

            action[i] = result_act;
        }

/*********************************************************************/
/*   We now initialize the elements reserved for reduce actions in   */
/* the current state.                                                */
/*********************************************************************/
        default_rule = REDUCE_RULE_NO(red, 0);
        for (j = 1; j <= red.size; j++)
        {
            if (REDUCE_RULE_NO(red, j) != default_rule)
            {
                symbol = REDUCE_SYMBOL(red, j);
                i = indx + symbol;
                check[i] = symbol;
                act = REDUCE_RULE_NO(red, j);
                if (rules[act].lhs == accept_image)
                    action[i] = accept_act;
                else
                    action[i] = act;
                reduce_count++;
            }
        }

/*********************************************************************/
/*   We now initialize the element reserved for the DEFAULT reduce   */
/* action of the current state.  If error maps are requested,  the   */
/* default slot is initialized to the original state number, and the */
/* corresponding element of the DEFAULT_REDUCE array is initialized. */
/* Otherwise it is initialized to the rule number in question.       */
/*********************************************************************/
        i = indx + DEFAULT_SYMBOL;
        check[i] = DEFAULT_SYMBOL;
        act = REDUCE_RULE_NO(red, 0);
        if (act == OMEGA)
            action[i] = error_act;
        else
        {
            action[i] = act;
            default_count++;
        }
    }

    PRNT("\n\nActions in Compressed Tables:");
    sprintf(msg_line, "     Number of Shifts: %d", shift_count);
    PRNT(msg_line);

    sprintf(msg_line,
            "     Number of Shift/Reduces: %d",
            shift_reduce_count);
    PRNT(msg_line);

    if (max_la_state > num_states)
    {
        sprintf(msg_line,
                "     Number of Look-Ahead Shifts: %d",
                la_shift_count);
        PRNT(msg_line);
    }

    sprintf(msg_line, "     Number of Gotos: %d", goto_count);
    PRNT(msg_line);

    sprintf(msg_line,
            "     Number of Goto/Reduces: %d", goto_reduce_count);
    PRNT(msg_line);

    sprintf(msg_line, "     Number of Reduces: %d", reduce_count);
    PRNT(msg_line);

    sprintf(msg_line, "     Number of Defaults: %d", default_count);
    PRNT(msg_line);

    if (error_maps_bit || debug_bit)
    {
        for ALL_STATES(state_no)
            check[state_index[state_no]] = -state_no;
    }
    for (i = 1; i <= (int) table_size; i++)
    {
        if (check[i] < 0 || check[i] > (java_bit ? 127 : 255))
            byte_check_bit = 0;
    }

    if (c_bit)
        mystrcpy("\n#define CLASS_HEADER\n\n");
    else if (cpp_bit)
    {
        mystrcpy("\n#define CLASS_HEADER ");
        mystrcpy(prs_tag);
        mystrcpy("_table::\n\n");
    }
    else if (java_bit)
    {
        mystrcpy("abstract class ");
        mystrcpy(dcl_tag);
        mystrcpy(" implements ");
        mystrcpy(def_tag);
        mystrcpy("\n{\n");
    }

    /*********************************************************************/
    /* Write size of right hand side of rules followed by CHECK table.   */
    /*********************************************************************/
    if (java_bit)
         mystrcpy("    public final static byte rhs[] = {0,\n");
    else mystrcpy("const unsigned char  CLASS_HEADER rhs[] = {0,\n");

    padline();
    k = 0;
    for (i = 1; i <= num_rules; i++)
    {
        k++;
        if (k > 15)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(sysdcl);
            padline();
            k = 1;
        }
        itoc(RHS_SIZE(i));
        *output_ptr++ = COMMA;
    }
    *(output_ptr - 1) = '\n';
    BUFFER_CHECK(sysdcl);

    if (java_bit)
         mystrcpy("    };\n");
    else mystrcpy("                 };\n");
    *output_ptr++ = '\n';

    /*****************************************************************/
    /* Write CHECK table.                                            */
    /*****************************************************************/
    if (byte_check_bit && (! error_maps_bit))
    {
        if (java_bit)
             mystrcpy("    public final static byte check_table[] = {\n");
        else mystrcpy("const unsigned char  CLASS_HEADER check_table[] = {\n");
    }
    else
    {
        if (java_bit)
             mystrcpy("     public final static short check_table[] = {\n");
        else mystrcpy("const   signed short CLASS_HEADER check_table[] = {\n");
    }

    padline();
    k = 0;
    for (i = 1; i <= (int) table_size; i++)
    {
        k++;
        if (k > 10)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(sysdcl);
            padline();
            k = 1;
        }
        itoc(check[i]);
        *output_ptr++ = COMMA;
    }
    *(output_ptr - 1) = '\n';
    BUFFER_CHECK(sysdcl);

    if (java_bit)
         mystrcpy("    };\n");
    else mystrcpy("                 };\n");
    *output_ptr++ = '\n';
    BUFFER_CHECK(sysdcl);

    if (byte_check_bit && (! error_maps_bit))
    {
        if (java_bit)
             mystrcpy("    public final static byte check(int i) "
                      "\n    {\n        return check_table[i - (NUM_RULES + 1)];\n    }\n");
        else mystrcpy("const unsigned char  *CLASS_HEADER check = "
                      "&(check_table[0]) - (NUM_RULES + 1);\n");
    }
    else
    {
        if (java_bit)
             mystrcpy("    public final static short check(int i) "
                      "\n    {\n        return check_table[i - (NUM_RULES + 1)];\n    }\n");
        else mystrcpy("const   signed short *CLASS_HEADER check = "
                      "&(check_table[0]) - (NUM_RULES + 1);\n");
    }
    *output_ptr++ = '\n';

    /*********************************************************************/
    /* Write left hand side symbol of rules followed by ACTION table.    */
    /*********************************************************************/
    if (java_bit)
         mystrcpy("    public final static char lhs[] = {0,\n");
    else mystrcpy("const unsigned short CLASS_HEADER lhs[] = {0,\n");
    padline();
    k = 0;
    for (i = 1; i <= num_rules; i++)
    {
        itoc(symbol_map[rules[i].lhs]);
        *output_ptr++ = COMMA;
        k++;
        if (k == 15)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(sysdcl);
            padline();
            k = 0;
        }
    }

    *output_ptr++ = '\n';
    *output_ptr++ = '\n';
    BUFFER_CHECK(sysdcl);
    padline();
    k = 0;

    if (error_maps_bit)
    {
        int max_indx;

        /*************************************************************/
        /* Construct a map from new state numbers into original      */
        /*   state numbers using the array check[]                   */
        /*************************************************************/
        max_indx = accept_act - num_rules - 1;
        for (i = 1; i <= max_indx; i++)
            check[i] = OMEGA;
        for ALL_STATES(state_no)
            check[state_index[state_no]] = state_no;

        j = num_states + 1;
        for (i = max_indx; i >= 1; i--)
        {
            state_no = check[i];
            if (state_no != OMEGA)
            {
                ordered_state[--j] = i + num_rules;
                state_list[j] = state_no;
            }
        }
    }
    for (i = 1; i <= (int) action_size; i++)
    {
        itoc(action[i]);
        *output_ptr++ = COMMA;
        k++;
        if (k == 10 && i != (int) action_size)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(sysdcl);
            padline();
            k = 0;
        }
    }

    if (k != 0)
    {
        *(output_ptr - 1) = '\n';
        BUFFER_CHECK(sysdcl);
    }
    if (java_bit)
         mystrcpy("    };\n");
    else mystrcpy("                 };\n");
    *output_ptr++ = '\n';
    BUFFER_CHECK(sysdcl);

    if (java_bit)
         mystrcpy("    public final static char action[] = lhs;\n");
    else mystrcpy("const unsigned short *CLASS_HEADER action = lhs;\n");
    *output_ptr++ = '\n';

    /********************************************************************/
    /* If GOTO_DEFAULT is requested, we print out the GOTODEF vector.   */
    /********************************************************************/
    if (goto_default_bit)
    {
        short *default_map;

        default_map = Allocate_short_array(num_symbols + 1);

        if (java_bit)
             mystrcpy("\n    public final static char default_goto[] = {0,\n");
        else mystrcpy("\nconst unsigned short CLASS_HEADER default_goto[] = {0,\n");

        padline();
        k = 0;
        for (i = 0; i <= num_symbols; i++)
            default_map[i] = error_act;

        for ALL_NON_TERMINALS(symbol)
        {
            act = gotodef[symbol];
            if (act < 0)
                result_act = -act;
            else if (act > 0)
                result_act = state_index[act] + num_rules;
            else
                result_act = error_act;
            default_map[symbol_map[symbol]] = result_act;
        }

        for (symbol = 1; symbol <= num_symbols; symbol++)
        {
            itoc(default_map[symbol]);
            *output_ptr++ = COMMA;
            k++;
            if (k == 10 && symbol != num_symbols)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(sysdcl);
                padline();
                k = 0;
            }
        }

        if (k != 0)
        {
            *(output_ptr - 1) = '\n';
            BUFFER_CHECK(sysdcl);
        }
        if (java_bit)
             mystrcpy("    };\n");
        else mystrcpy("                 };\n");
    }

    ffree(next);
    ffree(previous);

    if (error_maps_bit)
        print_error_maps();

    if (! byte_check_bit)
    {
        if (java_bit)
        {
            PRNT("\n***Warning: Base Check vector contains value "
                 "> 127. 16-bit words used.");
        }
        else
        {
            PRNT("\n***Warning: Base Check vector contains value "
                 "> 255. 16-bit words used.");
        }
    }

    if (! byte_terminal_range)
    {
        if (java_bit)
        {
            PRNT("***Warning: Terminal symbol > 127. 16-bit words used.");
        }
        else
        {
            PRNT("***Warning: Terminal symbol > 255. 16-bit words used.");
        }
    }

    if (java_bit)
        mystrcpy("}\n");

    fwrite(output_buffer, sizeof(char),
           output_ptr - &output_buffer[0], sysdcl);

    return;
}


/*********************************************************************/
/*                         PRINT_SPACE_PARSER:                       */
/*********************************************************************/
void print_space_parser(void)
{
    init_parser_files();

    print_space_tables();
    print_symbols();
    print_definitions();
    print_externs();

    exit_parser_files();

    return;
}


/*********************************************************************/
/*                         PRINT_TIME_PARSER:                        */
/*********************************************************************/
void print_time_parser(void)
{
    init_parser_files();

    print_time_tables();
    print_symbols();
    print_definitions();
    print_externs();

    exit_parser_files();

    return;
}
