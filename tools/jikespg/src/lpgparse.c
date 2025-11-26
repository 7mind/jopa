static char hostfile[] = __FILE__;

#include <time.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#undef scope_state
#undef SCOPE_UBOUND
#undef SCOPE_SIZE
#include "header.h"
#include "lpgsym.h"
#include "lpgdef.h"
#include "lpgdcl.h"
#include "lpgparse.h"

#define SPACE_CODE  1
#define DIGIT_CODE  2
#define ALPHA_CODE  3
#define IsSpace(c)  (code[c] == SPACE_CODE)
#define IsDigit(c)  (code[c] == DIGIT_CODE)
#define IsAlpha(c)  (code[c] == ALPHA_CODE)

static char code[256] = {0};

static int output_size = 80,
           line_no = 0;

/****************************************************************************/
/*                             PROCESS_INPUT:                               */
/****************************************************************************/
/* This procedures opens all relevant files and processes the input grammar.*/
/****************************************************************************/
void process_input(void)
{
    time_t ltime;
    unsigned c;

/****************************************************************************/
/* Open input grammar file. If the file cannot be opened and that file name */
/* did not have an extension, then the extension ".g" is added to the file  */
/* name and we try again. If no file can be found an error message is       */
/* issued and the program halts.                                            */
/****************************************************************************/
    if ((sysgrm = fopen(grm_file, "r")) == (FILE *) NULL)
    {
        register int i;

        for (i = strlen(grm_file); i > 0 &&
                                   grm_file[i] != '.' &&
                                   grm_file[i] != '/' && /* Unix */
                                   grm_file[i] != '\\';  /* Dos  */
                                   i--);

        if (grm_file[i] != '.')
        {
            strcat(grm_file, ".g");
            if ((sysgrm = fopen(grm_file, "r")) == (FILE *) NULL)
            {
                fprintf(stderr,
                        "***ERROR: Input file %s containing grammar "
                        "is empty, undefined, or invalid\n",grm_file);
                exit(12);
            }
        }
        else
        {
            fprintf(stderr,
                    "***ERROR: Input file %s containing grammar "
                    "is empty, undefined, or invalid\n",grm_file);
            exit(12);
        }
    }
#if !defined(C370) && !defined(CW)
    else
    {
        if (strrchr(grm_file, '.') == NULL)
        {
            sprintf(msg_line,
                    "A file named \"%s\" with no extension "
                    "is being opened", grm_file);
            PRNTWNG(msg_line);
        }
    }
#endif

/****************************************************************************/
/*               Assign timeptr to the local time.                          */
/****************************************************************************/
    time(&ltime);
    timeptr = ctime(&ltime);

/****************************************************************************/
/*                Open listing file for output.                             */
/****************************************************************************/
#if defined(C370) && !defined(CW)
    syslis = fopen(lis_file, "w, lrecl=85, recfm=VBA");
#else
    syslis = fopen(lis_file, "w");
#endif
    if (syslis  == (FILE *) NULL)
    {
        fprintf(stderr,
                "***ERROR: Listing file \"%s\" cannot be openned.\n",
                lis_file);
        exit(12);
    }

/****************************************************************************/
/* Complete the initialization of the code array used to replace the        */
/* builtin functions isalpha, isdigit and isspace.                          */
/****************************************************************************/
    for (c = 'a'; c <= 'z'; c++)
    {
        if (isalpha(c))
            code[c] = ALPHA_CODE;
    }
    for (c = 'A'; c <= 'Z'; c++)
    {
        if (isalpha(c))
            code[c] = ALPHA_CODE;
    }
    for (c = '0'; c <= '9'; c++)
    {
        if (isdigit(c))
            code[c] = DIGIT_CODE;
    }
    code[' ']  = SPACE_CODE;
    code['\n'] = SPACE_CODE;
    code['\t'] = SPACE_CODE;
    code['\r'] = SPACE_CODE;
    code['\v'] = SPACE_CODE;
    code['\f'] = SPACE_CODE;


/****************************************************************************/
/*          Print heading on terminal and in listing file                   */
/****************************************************************************/
    printf("\n %s %35.24s\n", HEADER_INFO, timeptr);
    PR_HEADING;

    init_process();

    process_grammar();

    exit_process();

    return;
}


/********************************************************************/
/*                          READ_INPUT:                             */
/********************************************************************/
/*         READ_INPUT fills the buffer from p1 to the end.          */
/********************************************************************/
static void read_input(void)
{
    long num_read;

    num_read = input_buffer + IOBUFFER_SIZE - bufend;
    if ((num_read = fread(bufend, 1, num_read, sysgrm)) == 0)
    {
        if (ferror(sysgrm) != 0)
        {
           fprintf(stderr,
                   "*** Error reading input file \"%s\".\n", grm_file);
           exit(12);
        }
    }
    bufend += num_read;
    *bufend = '\0';

    return;
}


/*****************************************************************************/
/*                              INIT_PROCESS:                                */
/*****************************************************************************/
/* This routine is invoked to allocate space for the global structures       */
/* needed to process the input grammar.                                      */
/*****************************************************************************/
static void init_process(void)
{
    /******************************************************************/
    /* Set up a a pool of temporary space.                            */
    /******************************************************************/
    reset_temporary_space();

    terminal = (struct terminal_type *)
               calloc(STACK_SIZE, sizeof(struct terminal_type));
    if (terminal == (struct terminal_type *) NULL)
        nospace(__FILE__, __LINE__);

    hash_table = (struct hash_type **)
                 calloc(HT_SIZE, sizeof(struct hash_type *));
    if (hash_table == (struct hash_type **) NULL)
        nospace(__FILE__, __LINE__);

    /***********************************************************************/
    /* Allocate space for input buffer and read in initial data in input   */
    /* file. Next, invoke PROCESS_OPTION_LINES to process all lines in     */
    /* input file that are options line.                                   */
    /***********************************************************************/
    input_buffer = (char *)
            calloc(IOBUFFER_SIZE + 1 + MAX_LINE_SIZE, sizeof(char));
    if (input_buffer == (char *) NULL)
        nospace(__FILE__, __LINE__);

    bufend = &input_buffer[0];
    read_input();

    p2 = &input_buffer[0];
    linestart = p2 - 1;
    p1 = p2;
    line_no++;

    if (*p2 == '\0')
    {
        fprintf(stderr,
                "Input file \"%s\" containing grammar is "
                "empty, undefined, or invalid\n",
                grm_file);
        exit(12);
    }

    process_options_lines();

    eolt_image = OMEGA;

    blockb_len = strlen(blockb);
    blocke_len = strlen(blocke);

    hblockb_len = strlen(hblockb);
    hblocke_len = strlen(hblocke);

    /*****************************************************/
    /* Keywords, Reserved symbols, and predefined macros */
    /*****************************************************/
    kdefine[0]            = escape;  /*Set empty first space to the default */
    kterminals[0]         = escape;  /* escape symbol.                      */
    kalias[0]             = escape;
    kstart[0]             = escape;
    krules[0]             = escape;
    knames[0]             = escape;
    kend[0]               = escape;
    krule_number[0]       = escape;
    krule_text[0]         = escape;
    krule_size[0]         = escape;
    knum_rules[0]         = escape;
    knum_terminals[0]     = escape;
    knum_non_terminals[0] = escape;
    knum_symbols[0]       = escape;
    kinput_file[0]        = escape;
    kcurrent_line[0]      = escape;
    knext_line[0]         = escape;
    kstart_nt[0]          = escape;
    keolt[0]              = escape;

    return;
}


/*****************************************************************************/
/*                              EXIT_PROCESS:                                */
/*****************************************************************************/
/* This routine is invoked to free all space used to process the input that  */
/* is no longer needed. Note that for the string_table, only the unused      */
/* space is released.                                                        */
/*****************************************************************************/
static void exit_process(void)
{
    if (string_offset > 0)
    {
        string_table = (char *)
            (string_table == (char *) NULL
             ? malloc((string_offset) * sizeof(char))
             : realloc(string_table, (string_offset) * sizeof(char)));

        if (string_table == (char *) NULL)
            nospace(__FILE__, __LINE__);
    }
    ffree(terminal);
    ffree(hash_table);
    ffree(input_buffer);
    ffree(rulehdr); /* allocated in action LPGACT when grammar is not empty */

    return;
}


/*****************************************************************************/
/*                                 VERIFY:                                   */
/*****************************************************************************/
/* VERIFY takes as argument a character string and checks whether or not each*/
/* character is a digit. If all are digits, then 1 is returned; if not, then */
/* 0 is returned.                                                            */
/*****************************************************************************/
static BOOLEAN verify(char *item)
{
    while (IsDigit(*item))
        item++;
    return (*item == '\0');
}


/*****************************************************************************/
/*                              TRANSLATE:                                   */
/*****************************************************************************/
/* TRANSLATE takes as arguments a character array, which it folds to upper   */
/* to uppercase and returns.                                                 */
/*****************************************************************************/
static char *translate(char *str, int len)
{
    register int i;

    for (i = 0; i < len; i++)
        str[i] = TOUPPER(str[i]);

    return(str);
} /* end translate */


/**********************************************************************/
/*                             STRXEQ:                                */
/**********************************************************************/
/* Compare two character strings s1 and s2 to check whether or not s2 */
/* is a substring of s1.  The string s2 is assumed to be in lowercase */
/* and NULL terminated. However, s1 does not have to (indeed, may not)*/
/* be NULL terminated.                                                */
/*                                                                    */
/* The test below may look awkward. For example, why not use:         */
/*                  if (tolower(s1[i]) != s2[i])  ?                   */
/* because tolower(ch) is sometimes implemented as (ch-'A'+'a') which */
/* does not work when "ch" is already a lower case character.         */
/*                                                                    */
/**********************************************************************/
static BOOLEAN strxeq(char *s1, char *s2)
{
    for (; *s2 != '\0'; s1++, s2++)
    {
        if (*s1 != *s2  && *s1 != toupper(*s2))
            return FALSE;
    }

    return TRUE;
}


/*****************************************************************************/
/*                              OPTIONS:                                     */
/*****************************************************************************/
/*   OPTION handles the decoding of options passed by the user and resets    */
/* them appropriately. "options" may be called twice: when a parameter line  */
/* is passed to the main program and when the user codes an %OPTIONS line in */
/* his grammar.                                                              */
/*    Basically, there are two kinds of options: switches which indicate a   */
/* certain setting just by their appearance, and valued options which are    */
/* followed by an equal sign and the value to be assigned to them.           */
/*****************************************************************************/
static void options(void)
{
    char *c,
         token[MAX_PARM_SIZE+1],
         temp [MAX_PARM_SIZE+1],
         delim;

    register int i,
                 j,
                 len;

    BOOLEAN flag;

/*******************************************************************/
/* If we scan the comment sign, we stop processing the rest of the */
/* parameter string.                                               */
/*******************************************************************/
    for (c = parm; *c != '\0'; c++)
    {
        if (*c == '-' && *(c+1) == '-')
            break;
    }
    *c = '\0';

    i = 0;

    while ((parm[i] != '\0') &&    /* Clean front of string */
           ((parm[i] == ',') ||
            (parm[i] == '/') ||
            (parm[i] == ' ')))
         i++;

    while (parm[i] != '\0')     /* Repeat until parm line is exhausted */
    {
        strcpy(parm, parm + i);       /* Remove garbage in front */

        i = 0;

        while ((parm[i] != '\0')&&    /* Search for delimiter */
               ((parm[i] != ',') &&
                (parm[i] != '/') &&
                (parm[i] != '=') &&
                (parm[i] != ' ')))
            i++;

        for (j = 0; j < i; j++)   /* Fold actual parameter */
        {
            token[j] = TOUPPER(parm[j]);
            temp[j] = parm[j];
        }
        token[i] = '\0';
        temp[i] = '\0';

        /***********************************/
        /* find first non-blank after parm */
        /***********************************/
        while (parm[i] != '\0' && parm[i] == ' ')
            i++;

        if (parm[i] != '\0')  /* not end of parameter line */
            delim = parm[i];
        else
            delim = ' ';

        len = strlen(token);
        if (len > MAX_PARM_SIZE)
            token[MAX_PARM_SIZE] = '\0';

/*****************************************************************************/
/*  We check whether we have a switch or a value parameter.                  */
/* Each category is checked separately.  A match is made whenever            */
/* a minimum unambiguous prefix of the token in question matches an          */
/* option...                                                                 */
/*                                                                           */
/* At this stage, TEMP contains the value of the switch as specified         */
/* and TOKEN contains the upper-case folded value of TEMP.                   */
/*****************************************************************************/
        if (delim != '=')  /* if switch parameter then process */
        {
            if ((len > 2) &&
                (memcmp(token, "NO", 2) == 0)) /* option has "NO" */
            {                                  /* prefix?         */
                flag = FALSE;
                len = len-2;
                strcpy(token, token + 2);  /* get rid of "NO" prefix */
            }
            else
                flag = TRUE;

            if (memcmp(oaction, token, len) == 0)
                action_bit = flag;
            else if (memcmp(obyte, token, len) == 0)
                byte_bit = flag;
            else if (memcmp(oconflicts, token, len) == 0)
                conflicts_bit = flag;
            else if (len >=4 && memcmp(odefault, token, len) == 0)
            {
                if (flag)
                    default_opt = 5;
                else
                    default_opt = 0;
            }
            else if (len >= 3 && memcmp(odebug, token, len) == 0)
                debug_bit = flag;
            else if (len >= 4 && memcmp(odeferred, token, len) == 0)
                deferred_bit = flag;
            else if (len >= 2 && memcmp(oedit, token, len) == 0)
                edit_bit = flag;
            else if ((len >= 2) &&
                      ((memcmp(oerrormaps,token, len) == 0) ||
                       (strcmp("EM", token) == 0) ||
                       (memcmp(oerrormaps2, token, len) == 0) ||
                       (memcmp(oerrormaps3, token, len) == 0)))
                error_maps_bit = flag;
            else if ((len >= 2) && (memcmp(ofirst, token, len) == 0))
                first_bit = flag;
            else if ((len >= 2) && (memcmp(ofollow, token, len) == 0))
                follow_bit = flag;
            else if (len >= 2 &&
                     ((strcmp(token, "GP") == 0) ||
                      (memcmp(ogenprsr, token, len) == 0) ||
                      (memcmp(ogenprsr2, token, len) == 0) ||
                      (memcmp(ogenprsr3, token, len) == 0)))
            {
                c_bit = flag;
                cpp_bit = FALSE;
                java_bit = FALSE;
            }
            else if (len >= 2 &&
                     ((strcmp(token, "GD") == 0) ||
                      (memcmp(ogotodefault, token, len) == 0) ||
                      (memcmp(ogotodefault2, token, len) == 0) ||
                      (memcmp(ogotodefault3, token, len) == 0)))
                goto_default_bit = flag;
            else if  ((strcmp(token, "HW") == 0) ||
                      (memcmp(ohalfword, token, len) == 0) ||
                      (memcmp(ohalfword2, token, len) == 0) ||
                      (memcmp(ohalfword3, token, len) == 0))
                byte_bit = NOT(flag);
	    else if ((strcmp(token, "JI") == 0) ||
		     (memcmp(ojikes, token, len) == 0))
		jikes_bit = 1;
            else if (len >= 2 && memcmp(olalr, token, len) == 0)
            {
                slr_bit = NOT(flag);
                lalr_level = 1;
            }
            else if (len >= 2 && memcmp(olist, token, len) == 0)
                list_bit = flag;
            else if  ((strcmp(token, "NC") == 0) ||
                      (memcmp(ontcheck, token, len) == 0) ||
                      (memcmp(ontcheck2, token, len) == 0) ||
                      (memcmp(ontcheck3, token, len) == 0))
                nt_check_bit = flag;
            else if ((strcmp(token, "RR") == 0) ||
                     (memcmp(oreadreduce, token, len) == 0) ||
                     (memcmp(oreadreduce2, token, len) == 0) ||
                     (memcmp(oreadreduce3, token, len) == 0))
                read_reduce_bit = flag;
            else if (len >=2 && memcmp(oscopes, token, len) == 0)
                scopes_bit = flag;
            else if ((len >= 2) &&
                     ((strcmp(token, "SD") == 0) ||
                      (memcmp(oshiftdefault, token, len) == 0)  ||
                      (memcmp(oshiftdefault2, token, len) == 0) ||
                      (memcmp(oshiftdefault3, token, len) == 0)))
                shift_default_bit = flag;
            else if ((len >= 2) &&
                     ((strcmp(token, "SP") == 0) ||
                      (memcmp(osingleproductions, token, len) == 0) ||
                      (memcmp(osingleproductions2, token, len) == 0) ||
                      (memcmp(osingleproductions3, token, len) == 0)))
                single_productions_bit = flag;
            else if (len >= 2 && memcmp(oslr, token, len) == 0)
            {
                slr_bit = flag;
                lalr_level = 1;
            }
            else if (len >= 2 && memcmp(ostates, token, len) == 0)
                states_bit = flag;
            else if (len >= 2 && memcmp(otable, token, len) == 0)
            {
                if (flag)
                    table_opt = OPTIMIZE_SPACE;
                else
                    table_opt = 0;
            }
            else if (len >= 2 && memcmp(otrace, token, len) == 0)
            {
                if (flag)
                    trace_opt = TRACE_CONFLICTS;
                else
                    trace_opt = NOTRACE;
            }
            else if (memcmp(overbose, token, len) == 0)
                verbose_bit = flag;
            else if (memcmp(owarnings, token, len) == 0)
                warnings_bit = flag;
            else if (memcmp(oxref, token, len) == 0)
                xref_bit = flag;
            else if ((strcmp(token, "D") == 0) ||
                     (strcmp(token, "DE") == 0))
            {
                sprintf(msg_line,
                        "\"%s\" is an ambiguous option: "
                        "DEBUG, DEFAULT, DEFERRED ?", temp);
                PRNTERR(msg_line);
            }
            else if (strcmp(token, "DEF") == 0)
            {
                PRNTERR("\"DEF\" is an ambiguous option: "
                        "DEFAULT, DEFERRED ?");
            }
            else if (strcmp(token, "E") == 0)
            {
                PRNTERR("\"E\" is an ambiguous option: "
                        "EDIT, ERROR-MAPS ?");
            }
            else if (strcmp(token, "F") == 0)
            {
                PRNTERR("\"F\" is an ambiguous option: FOLLOW, FIRST ?");
            }
            else if (strcmp(token, "G") == 0)
            {
                PRNTERR("\"G\" is an ambiguous option: "
                        "GENERATE-PARSER, GOTO-DEFAULT ?");
            }
            else if (strcmp(token, "L") == 0)
            {
                PRNTERR("\"L\" is an ambiguous option: LALR, LIST ?");
            }
            else if (strcmp(token, "S") == 0)
            {
                PRNTERR("\"S\" is an ambiguous option:\n "
                        "SCOPES, SEARCH, STATES, SLR, "
                        "SHIFT-DEFAULT, SINGLE-PRODUCTIONS  ?");
            }
            else if (strcmp(token, "T") == 0)
            {
                PRNTERR("\"T\" is an ambiguous option:  TABLE, TRACE ?");
            }
            else
            {
                sprintf(msg_line, "\"%s\" is an invalid option", temp);
                PRNTERR(msg_line);
            }
        }

        /****************************************/
        /* We now process the valued-parameter. */
        /****************************************/
        else /* value parameter. pick value after "=" and process */
        {
            i++;
            if (IsSpace(parm[i]) || parm[i] == '\0')/* no value specified */
            {
                sprintf(msg_line,
                        "Null string or blank is invalid for parameter %s",
                        token);
                PRNTERR(msg_line);
                continue;
            }

            j = i;
            while ((parm[i] != '\0')&&   /* find next delimeter */
                   ((parm[i] !=  ',') &&
                    (parm[i] !=  '/') &&
                    (parm[i] !=  ' ')))
                i++;

            memcpy(temp, parm+j, i - j);  /* copy into TEMP */
            temp[i - j] = '\0';

#if (defined(C370) || defined(CW)) && (!defined(MVS))
            if ((strcmp(token, "AN") == 0) ||
                 ((len >= 9) &&
                  ((memcmp(token, oactfile_name, len) == 0) ||
                   (memcmp(token, oactfile_name2, len) == 0) ||
                   (memcmp(token, oactfile_name3, len) == 0))))
            {
                memcpy(an, temp, 8);
                pn[MIN(strlen(temp), 8)] = '\0';
                strupr(an);
            }
            else if ((strcmp(token, "AT") == 0) ||
                      ((len >= 9) &&
                       ((memcmp(token, oactfile_type, len) == 0) ||
                        (memcmp(token, oactfile_type2, len) == 0) ||
                        (memcmp(token, oactfile_type3, len) == 0))))
            {
                memcpy(at, temp, 8);
                at[MIN(strlen(temp), 8)] = '\0';
                strupr(at);
            }
            else if ((strcmp(token, "AM") == 0) ||
                     ((len >= 9) &&
                      ((memcmp(token, oactfile_mode, len) == 0) ||
                       (memcmp(token, oactfile_mode2, len) == 0) ||
                       (memcmp(token, oactfile_mode3, len) == 0))))
            {
                memcpy(am, temp, 2);
                am[MIN(strlen(temp), 2)] = '\0';
                strupr(am);
            }
#else
            if ((strcmp(token, "AN") == 0) ||
                (memcmp(token, oactfile_name, len) == 0) ||
                (memcmp(token, oactfile_name2, len) == 0) ||
                (memcmp(token, oactfile_name3, len) == 0))
               strcpy(act_file, temp);
#endif
            else if (strcmp(token, oblockb) == 0)
                strcpy(blockb, temp);
            else if (strcmp(token, oblocke) == 0)
                strcpy(blocke, temp);
            else if (memcmp(odefault, token, len) == 0)
            {
                if (verify(temp))
                    default_opt = MIN(atoi(temp), 5);
                else
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
            }
            else if (len >= 2 && memcmp(token, oescape, len) == 0)
                escape = temp[0];
            else if (((strcmp(token, "FP") == 0) ||
                      (memcmp(token, ofile_prefix, len) == 0) ||
                      (memcmp(token, ofile_prefix2, len) == 0) ||
                      (memcmp(token, ofile_prefix3, len) == 0)))
            {
                memcpy(file_prefix, temp, 5);
                file_prefix[MIN(5, strlen(temp))] = '\0';
            }
            else if ((strcmp(token, "GP") == 0) ||
                     (memcmp(ogenprsr, token, len) == 0) ||
                     (memcmp(ogenprsr2, token, len) == 0) ||
                     (memcmp(ogenprsr3, token, len) == 0))
            {
                BOOLEAN invalid_language = TRUE;

                if (temp[0] == 'c' || temp[0] == 'C')
                {
                    if (temp[1] == '\0')
                    {
                        c_bit = TRUE;
                        cpp_bit = FALSE;
                        java_bit = FALSE;
                        invalid_language = FALSE;
                    }
                    else if (((temp[1] == '+' && temp[2] == '+') ||
                              ((temp[1] == 'p' || temp[1] == 'P') &&
                               (temp[2] == 'p' || temp[2] == 'P')))
                             && temp[3] == '\0')
                    {
                        c_bit = FALSE;
                        cpp_bit = TRUE;
                        java_bit = FALSE;
                        invalid_language = FALSE;
                    }
                }
                else if ((len == 1 && (*temp == 'j' || *temp == 'J')) ||
                         (len == 2 && strxeq(temp, "ja")) ||
                         (len == 3 && strxeq(temp, "jav")) ||
                         (len == 4 && strxeq(temp, "java")))
                {
                    c_bit = FALSE;
                    cpp_bit = FALSE;
                    java_bit = TRUE;
                    invalid_language = FALSE;
                }

                if (invalid_language)
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
            }
#if (defined(C370) || defined(CW)) && (!defined(MVS))
            else if ((strcmp(token, "HN") == 0) ||
                 ((len >= 9) &&
                  ((memcmp(token, ohactfile_name,  len) == 0) ||
                   (memcmp(token, ohactfile_name2, len) == 0) ||
                   (memcmp(token, ohactfile_name3, len) == 0))))
            {
                memcpy(han, temp, 8);
                pn[MIN(strlen(temp), 8)] = '\0';
                strupr(han);
            }
            else if ((strcmp(token, "HT") == 0) ||
                      ((len >= 9) &&
                       ((memcmp(token, ohactfile_type,  len) == 0) ||
                        (memcmp(token, ohactfile_type2, len) == 0) ||
                        (memcmp(token, ohactfile_type3, len) == 0))))
            {
                memcpy(hat, temp, 8);
                hat[MIN(strlen(temp), 8)] = '\0';
                strupr(hat);
            }
            else if ((strcmp(token, "HM") == 0) ||
                     ((len >= 9) &&
                      ((memcmp(token, ohactfile_mode,  len) == 0) ||
                       (memcmp(token, ohactfile_mode2, len) == 0) ||
                       (memcmp(token, ohactfile_mode3, len) == 0))))
            {
                memcpy(ham, temp, 2);
                ham[MIN(strlen(temp), 2)] = '\0';
                strupr(ham);
            }
#else
            else if ((strcmp(token, "HN") == 0) ||
                 ((len >= 2) &&
                  ((memcmp(token, ohactfile_name,  len) == 0) ||
                   (memcmp(token, ohactfile_name2, len) == 0) ||
                   (memcmp(token, ohactfile_name3, len) == 0))))
               strcpy(hact_file, temp);
#endif
            else if (len >= 2 && strcmp(token, ohblockb) == 0)
                strcpy(hblockb, temp);
            else if (len >= 2 && strcmp(token, ohblocke) == 0)
                strcpy(hblocke, temp);
            else if (memcmp (olalr, token, len) == 0)
            {
                len = strlen(temp);
                if (len > MAX_PARM_SIZE)
                    temp[MAX_PARM_SIZE - 1] = '\0';
                if ((! verify(temp)) &&
                    (memcmp(translate(temp, len), omax, len) != 0))
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
                else
                {
                    slr_bit = FALSE;
                    if (memcmp(omax, translate(temp, len), len) == 0)
                        lalr_level = MAXIMUM_LA_LEVEL;
                    else
                    {
                        lalr_level =  atoi(temp);
                        if (lalr_level > MAXIMUM_LA_LEVEL)
                        {
                            sprintf(msg_line,
                                    "\"%s\" exceeds maximum value "
                                    "of %d allowed for %s",
                                    temp, MAXIMUM_LA_LEVEL, token);
                            PRNTWNG(msg_line);
                            lalr_level = MAXIMUM_LA_LEVEL;
                        }
                    }
                }
            }
            else if ((len >= 2) &&
                     ((memcmp(token, omaximum_distance,len) == 0) ||
                      (memcmp(token, omaximum_distance2, len) == 0) ||
                      (memcmp(token, omaximum_distance3, len) == 0)))
            {
                if (verify(temp))
                    maximum_distance =  atoi(temp);
                else
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
            }
            else if ((len >= 2) &&
                     ((memcmp(token, ominimum_distance,len) == 0) ||
                      (memcmp(token, ominimum_distance2, len) == 0) ||
                      (memcmp(token, ominimum_distance3, len) == 0)))
            {
                if (verify(temp))
                    minimum_distance =  atoi(temp);
                else
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
            }
            else if ((memcmp(onames, token, len) == 0))
            {
                len = strlen(temp);
                if (len >= 2 && memcmp(omax, translate(temp, len), len) == 0)
                    names_opt = MAXIMUM_NAMES;
                else if (len >= 2 &&
                         memcmp(omin, translate(temp, len), len) == 0)
                    names_opt = MINIMUM_NAMES;
                else if (memcmp(translate(temp, len), ooptimized, len) == 0)
                    names_opt = OPTIMIZE_PHRASES;
                else if ((temp[0] == 'm' || temp[0] == 'M') &&
                                            temp[1] != '\0')
                {
                    sprintf(msg_line,
                            "\"M\" is an ambiguous value for %s: "
                            "MAXIMUM, MINIMUM?", token);
                    PRNTERR(msg_line);
                }
                else
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
            }
            else if (len >= 2 && memcmp(token, oormark, len) == 0)
                ormark = temp[0];
            else if ((len >= 2) &&
                     ((strcmp(token, "OS") == 0) ||
                      (memcmp(token, ooutputsize, len) == 0) ||
                      (memcmp(token, ooutputsize2, len) == 0) ||
                      (memcmp(token, ooutputsize3, len) == 0)))
            {
                if (verify(temp))
                {
                    int tmpval;
                    tmpval =  atoi(temp);
                    if (tmpval > MAX_LINE_SIZE)
                    {
                        sprintf(msg_line, "OUTPUT_SIZE cannot exceed %d",
                                MAX_LINE_SIZE);
                        PRNTERR(msg_line);
                    }
                    else
                        output_size =  tmpval;
                }
                else
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
            }
            else if (memcmp(token, oprefix, len) == 0)
                strcpy(prefix,temp);
#if defined(C370) || defined(CW)
            else if ((strcmp(token, "RF") == 0) ||
                     (strcmp(token, "RECFM") == 0) ||
                     (memcmp(token, orecordformat, len) == 0) ||
                     (memcmp(token, orecordformat2, len) == 0) ||
                     (memcmp(token, orecordformat3, len) == 0))
            {
                len = strlen(temp);
                if (len > MAX_PARM_SIZE)
                    temp[MAX_PARM_SIZE-1] = '\0';

                if (memcmp(ofixed, translate(temp, len), len) == 0)
                    record_format = 'F';
                else if (memcmp(translate(temp, len), ovariable,len) == 0)
                    record_format = 'V';
                else
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
            }
#endif
            else if (strcmp(token, "SS") == 0 ||
                     ((memcmp(token, ostack_size,len) == 0) ||
                      (memcmp(token, ostack_size2, len) == 0) ||
                      (memcmp(token, ostack_size3, len) == 0)))
            {
                if (verify(temp))
                    stack_size =  atoi(temp);
                else
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
            }
            else if (len >= 2 && memcmp(token, osuffix, len) == 0)
                strcpy(suffix,temp);
            else if (len >= 2 && memcmp(token,otable, len) == 0)
            {
                len = strlen(temp);
                if (len > MAX_PARM_SIZE)
                    temp[MAX_PARM_SIZE - 1] = '\0';

                if (memcmp(ospace, translate(temp, len), len) == 0)
                    table_opt = OPTIMIZE_SPACE;
                else if (memcmp(translate(temp, len), otime, len) == 0)
                    table_opt = OPTIMIZE_TIME;
                else
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
            }
            else if (len >= 2 && memcmp(token, otrace, len) == 0)
            {
                len = strlen(temp);
                if (len > MAX_PARM_SIZE)
                    temp[MAX_PARM_SIZE-1] = '\0';

                if (memcmp(oconflicts,  translate(temp, len), len) == 0)
                    trace_opt = TRACE_CONFLICTS;
                else if (memcmp(translate(temp, len), ofull, len) == 0)
                    trace_opt = TRACE_FULL;
                else
                {
                    sprintf(msg_line,
                            "\"%s\" is an invalid value for %s",
                            temp, token);
                    PRNTERR(msg_line);
                }
            }
#if (defined(C370) || defined(CW)) && (!defined(MVS))
            else if (memcmp(token, oactfile_name2, len) == 0 ||
                     memcmp(token, oactfile_name3, len) == 0)
            {
                sprintf(msg_line,
                        "Option \"%s\" is ambiguous: "
                        "ACTFILE-NAME, ACTFILE-TYPE, ACTFILE-MODE?",
                        token);
                PRNTERR(msg_line);
            }
#endif
            else if (strcmp(token, "BLOCK") == 0)
            {
                PRNTERR("Option \"BLOCK\" is ambiguous: BLOCKB, BLOCKE ?");
            }
            else if (strcmp("E",token) == 0)
            {
                PRNTERR("Option \"E\" is ambiguous: "
                        "ERROR-PROC, ERROR-MAPS ?");
            }
            else if (strcmp(token, "H") == 0)
            {
                PRNTERR("Option \"H\" is ambiguous: HBLOCKB, HBLOCKE ?");
            }
#if (defined(C370) || defined(CW)) && (!defined(MVS))
            else if (memcmp(token, ohactfile_name2, len) == 0 ||
                     memcmp(token, ohactfile_name3, len) == 0)
            {
                sprintf(msg_line,
                        "Option \"%s\" is ambiguous: "
                        "HACTFILE-NAME, HACTFILE-TYPE, HACTFILE-MODE?",
                        token);
                PRNTERR(msg_line);
            }
#endif
            else if (strcmp(token, "HBLOCK") == 0)
            {
                PRNTERR("Option \"HBLOCK\" is ambiguous: HBLOCKB, HBLOCKE ?");
            }
            else if (strcmp("M",token) == 0 || strcmp("MD",token) == 0)
            {
                sprintf(msg_line,
                        "Option \"%s\" is ambiguous: "
                        "MAX-DISTANCE, MIN-DISTANCE?", token);
                PRNTERR(msg_line);
            }
            else if (strcmp("O",token) == 0)
            {
                PRNTERR("Option \"O\" is ambiguous: OUTPUT-SIZE, ORMARK ?");
            }
            else if (strcmp("T", token) == 0)
            {
                PRNTERR("Option \"T\" is ambiguous: TABLE, TRACE?");
            }
            else
            {
                sprintf(msg_line, "\"%s\" is an invalid option", token);
                PRNTERR(msg_line);
            }
        }

        while ((parm[i] != '\0')&&          /* clean after paramter */
                ((parm[i] == ',') ||
                 (parm[i] == '/') ||
                 (parm[i] == ' ')))
            i++;
    }

    return;
}


/****************************************************************************/
/*                       PROCESS_OPTIONS_LINES:                             */
/****************************************************************************/
/*    In this function, we read the first line(s) of the input text to see  */
/* if they are (it is an) "options" line(s).  If so, the options are        */
/* processed.  Then, we process user-supplied options if there are any.  In */
/* any case, the options in effect are printed.                             */
/****************************************************************************/
static void process_options_lines(void)
{
    char old_parm[MAX_LINE_SIZE + 1],
         output_line[PRINT_LINE_SIZE + 1],
         opt_string[60][OUTPUT_PARM_SIZE + 1],
         *line_end,
         temp[SYMBOL_SIZE + 1];

    static char ooptions[9] = " OPTIONS";

    int top = 0,
        i;

    strcpy(old_parm, parm); /* Save new options passed to program */
    ooptions[0] = escape;   /* "ooptions" always uses default escape symbol */

    while (p1 != '\0') /* Until end-of-file is reached, process */
    {                 /* all comment and %options lines.       */
        while(IsSpace(*p2))    /* skip all space symbols */
        {
            if (*p2 == '\n')
            {
                line_no++;
                linestart = p2;
                p1 = p2 + 1;
            }
            p2++;
        }
        line_end = strchr(p2, '\n'); /* find end-of-line */

        /*********************************************************************/
        /* First, check if line is a comment line. If so, skip it.  Next,    */
        /* check if line is an options line. If so, process it. Otherwise,   */
        /* break out of the loop.                                            */
        /* Note that no length check is necessary before checking for "--"   */
        /* or "%options" since the buffer is always extended by              */
        /* MAX_LINE_SIZE elements past its required length. (see read_input) */
        /*********************************************************************/
        if (*p2 == '-' && *(p2 + 1) == '-')
            /* skip comment line */ ;
        else if (memcmp(ooptions, translate(p2, 8), 8) == 0)
        {
            *line_end = '\0';
            PRNT(p2);    /* Print options line */
            strcpy(parm, p2 + strlen(ooptions));
            options();   /* Process hard-coded options */
        }
        else
        {
            p2 = p1; /* make p2 point to first character */
            break;
        }

        /*********************************************************************/
        /* If the line was a comment or an option line, check the following  */
        /* line.  If we are at the end of the buffer, read in more data...   */
        /*********************************************************************/
        p1 = line_end + 1;
        if (bufend == input_buffer + IOBUFFER_SIZE)
        {
            i = bufend - p1;
            if (i < MAX_LINE_SIZE)
            {
                strcpy(input_buffer, p1);
                bufend = &input_buffer[i];
                read_input();
                p1 = &input_buffer[0];
            }
        }
        line_no++;
        linestart = p1 - 1;
        p2 = p1;
    }

    fprintf(syslis, "\n");
    strcpy(parm, old_parm);
    options();  /* Process new options passed directly to  program */

    if (! error_maps_bit)  /* Deferred parsing without error maps is useless*/
        deferred_bit = FALSE;

#if (defined(C370) || defined(CW)) && (!defined(MVS))
        strupr(file_prefix);
        if (pn[0] == '\0')
        {
            strcpy(pn,file_prefix);
            strcat(pn, "PRS");
        }
        if (sn[0] == '\0')
        {
            strcpy(sn,file_prefix);
            strcat(sn, "SYM");
        }
        if (an[0] == '\0')
        {
            strcpy(an,file_prefix);
            strcat(an, "ACT");
        }
        if (han[0] == '\0')
        {
            strcpy(han,file_prefix);
            strcat(han, "HDR");
        }
        sprintf(act_file, "%s.%s.%s", an, at, am);
        sprintf(hact_file, "%s.%s.%s", han, hat, ham);
        sprintf(sym_file, "%s.%s.%s", sn, st, sm);
        sprintf(def_file, "%sDEF.%s.%s", file_prefix, (java_bit ? "JAVA" : "H"), sm);
        sprintf(prs_file, "%s.%s.%s", pn, pt, pm);
        sprintf(dcl_file, "%sDCL.%s.%s", file_prefix, (java_bit ? "JAVA" : "H"), sm);
#else
#if defined(MVS)
        if (act_file[0] == '\0')
            sprintf(act_file, "%sACT.%s", file_prefix, (java_bit ? "JAVA" : "H"));
        if (hact_file[0] == '\0')
            sprintf(hact_file, "%sHDR.%s", file_prefix, (java_bit ? "JAVA" : "H"));
        sprintf(sym_file, "%sSYM.%s", file_prefix, (java_bit ? "JAVA" : "H"));
        sprintf(def_file, "%sDEF.%s", file_prefix, (java_bit ? "JAVA" : "H"));
        sprintf(prs_file, "%sPRS.%s", file_prefix, (java_bit ? "JAVA" : "H"));
        sprintf(dcl_file, "%sDCL.%s", file_prefix, (java_bit ? "JAVA" : "H"));
#else
        if (act_file[0] == '\0')
            sprintf(act_file, "%sact.%s", file_prefix, (java_bit ? "java" : "h"));
        if (hact_file[0] == '\0')
            sprintf(hact_file, "%shdr.%s", file_prefix, (java_bit ? "java" : "h"));
        sprintf(sym_file, "%ssym.%s", file_prefix, (java_bit ? "java" : "h"));
        sprintf(def_file, "%sdef.%s", file_prefix, (java_bit ? "java" : "h"));
        sprintf(prs_file, "%sprs.%s", file_prefix, (java_bit ? "java" : "h"));
        sprintf(dcl_file, "%sdcl.%s", file_prefix, (java_bit ? "java" : "h"));
#endif
#endif

    if (verbose_bit)   /* turn everything on */
    {
        first_bit = TRUE;
        follow_bit = TRUE;
        list_bit = TRUE;
        states_bit = TRUE;
        xref_bit = TRUE;
        warnings_bit = TRUE;
    }


/*****************************************************************************/
/*                          PRINT OPTIONS:                                   */
/*****************************************************************************/
/* Here we print all options set by the user.  As of now, only about 48      */
/* different options and related aliases are allowed.  In case that number   */
/* goes up, the bound of the array, opt_string, should be changed.           */
/* BLOCKB, BLOCKE, HBLOCKB and HBLOCKE can generate the longest strings      */
/* since their value can be up to MAX_PARM_SIZE characters long.             */
/*****************************************************************************/
    if (action_bit)
        strcpy(opt_string[++top], "ACTION");
    else
        strcpy(opt_string[++top], "NOACTION");

#if (defined(C370) || defined(CW)) && (!defined(MVS))
    sprintf(opt_string[++top], "ACTFILE-NAME=%s",an);
    sprintf(opt_string[++top], "ACTFILE-TYPE=%s",at);
    sprintf(opt_string[++top], "ACTFILE-MODE=%s",am);
#else
    sprintf(opt_string[++top], "ACTFILE-NAME=%s",act_file);
#endif

    sprintf(opt_string[++top], "BLOCKB=%s",blockb);

    sprintf(opt_string[++top], "BLOCKE=%s", blocke);

    if (byte_bit)
        strcpy(opt_string[++top], "BYTE");

    if (conflicts_bit)
        strcpy(opt_string[++top], "CONFLICTS");
    else
        strcpy(opt_string[++top], "NOCONFLICTS");

    if (default_opt == 0)
        strcpy(opt_string[++top], "NODEFAULT");
    else
        sprintf(opt_string[++top], "DEFAULT=%d",default_opt);

    if (debug_bit)
        strcpy(opt_string[++top], "DEBUG");
    else
        strcpy(opt_string[++top], "NODEBUG");

    if (deferred_bit)
        strcpy(opt_string[++top], "DEFERRED");
    else
        strcpy(opt_string[++top], "NODEFERRED");

    if (edit_bit)
        strcpy(opt_string[++top], "EDIT");
    else
        strcpy(opt_string[++top], "NOEDIT");

    if (error_maps_bit)
        strcpy(opt_string[++top], "ERROR-MAPS");
    else
        strcpy(opt_string[++top], "NOERROR-MAPS");

    sprintf(opt_string[++top], "ESCAPE=%c", escape);

    sprintf(opt_string[++top], "FILE-PREFIX=%s", file_prefix);
    if (first_bit)
        strcpy(opt_string[++top], "FIRST");
    else
        strcpy(opt_string[++top], "NOFIRST");

    if (follow_bit)
        strcpy(opt_string[++top], "FOLLOW");
    else
        strcpy(opt_string[++top], "NOFOLLOW");

    if (c_bit)
        sprintf(opt_string[++top], "GENERATE-PARSER=C");
    else if (cpp_bit)
        sprintf(opt_string[++top], "GENERATE-PARSER=C++");
    else if (java_bit)
        sprintf(opt_string[++top], "GENERATE-PARSER=JAVA");
    else
        strcpy(opt_string[++top], "NOGENERATE-PARSER");

    if (goto_default_bit)
        strcpy(opt_string[++top], "GOTO-DEFAULT");
    else
        strcpy(opt_string[++top], "NOGOTO-DEFAULT");

#if defined(C370) || defined(CW)
    sprintf(opt_string[++top], "HACTFILE-NAME=%s", han);
    sprintf(opt_string[++top], "HACTFILE-TYPE=%s", hat);
    sprintf(opt_string[++top], "HACTFILE-MODE=%s", ham);
#else
    sprintf(opt_string[++top], "HACTFILE-NAME=%s", hact_file);
#endif

    if (! byte_bit)
        strcpy(opt_string[++top], "HALF-WORD");

    sprintf(opt_string[++top], "HBLOCKB=%s", hblockb);

    sprintf(opt_string[++top], "HBLOCKE=%s", hblocke);

    if (! slr_bit)
        sprintf(opt_string[++top], "LALR=%d", lalr_level);

    if (list_bit)
        strcpy(opt_string[++top], "LIST");
    else
        strcpy(opt_string[++top], "NOLIST");

    sprintf(opt_string[++top], "MAX-DISTANCE=%d",maximum_distance);
    sprintf(opt_string[++top], "MIN-DISTANCE=%d",minimum_distance);
    if (names_opt == MAXIMUM_NAMES)
        strcpy(opt_string[++top], "NAMES=MAXIMUM");
    else if (names_opt == MINIMUM_NAMES)
        strcpy(opt_string[++top], "NAMES=MINIMUM");
    else
        strcpy(opt_string[++top], "NAMES=OPTIMIZED");

    if (nt_check_bit)
        strcpy(opt_string[++top], "NT-CHECK");
    else
        strcpy(opt_string[++top], "NONT-CHECK");

    sprintf(opt_string[++top], "ORMARK=%c", ormark);
    sprintf(opt_string[++top], "OUTPUT-SIZE=%d", output_size);
    sprintf(opt_string[++top], "PREFIX=%s",prefix);

    if (read_reduce_bit)
        strcpy(opt_string[++top], "READ-REDUCE");
    else
        strcpy(opt_string[++top], "NOREAD-REDUCE");

#if defined(C370) || defined(CW)
    if (record_format == 'F')
        strcpy(opt_string[++top], "RECORD-FORMAT=F");
    else
        strcpy(opt_string[++top], "RECORD-FORMAT=V");
#endif

    if (scopes_bit)
        strcpy(opt_string[++top], "SCOPES");
    else
        strcpy(opt_string[++top], "NOSCOPES");

    if (shift_default_bit)
        strcpy(opt_string[++top], "SHIFT-DEFAULT");
    else
        strcpy(opt_string[++top], "NOSHIFT-DEFAULT");

    if (single_productions_bit)
        strcpy(opt_string[++top], "SINGLE-PRODUCTIONS");
    else
        strcpy(opt_string[++top], "NOSINGLE-PRODUCTIONS");

    if (slr_bit)
        strcpy(opt_string[++top], "SLR");

    sprintf(opt_string[++top], "STACK-SIZE=%d",stack_size);
    if (states_bit)
        strcpy(opt_string[++top], "STATES");
    else
        strcpy(opt_string[++top], "NOSTATES");

    sprintf(opt_string[++top], "SUFFIX=%s",suffix);

    if (table_opt == 0)
        strcpy(opt_string[++top], "NOTABLE");
    else if (table_opt == OPTIMIZE_SPACE)
        strcpy(opt_string[++top], "TABLE=SPACE");
    else
        strcpy(opt_string[++top], "TABLE=TIME");

    if (trace_opt == NOTRACE)
        strcpy(opt_string[++top], "NOTRACE");
    else if (trace_opt == TRACE_CONFLICTS)
        strcpy(opt_string[++top], "TRACE=CONFLICTS");
    else
        strcpy(opt_string[++top], "TRACE=FULL");

    if (verbose_bit)
        strcpy(opt_string[++top], "VERBOSE");
    else
        strcpy(opt_string[++top], "NOVERBOSE");

    if (warnings_bit)
        strcpy(opt_string[++top], "WARNINGS");
    else
        strcpy(opt_string[++top], "NOWARNINGS");

    if (xref_bit)
        strcpy(opt_string[++top], "XREF");
    else
        strcpy(opt_string[++top], "NOXREF");

    PRNT("Options in effect:");
    strcpy(output_line, "    ");
    for (i = 1; i <= top; i++)
    {
        if (strlen(output_line) + strlen(opt_string[i]) > PRINT_LINE_SIZE-1)
        {
            PRNT(output_line);
            strcpy(output_line, "    ");
        }
        strcat(output_line, opt_string[i]);
        if (strlen(output_line) + 2 < PRINT_LINE_SIZE-1)
            strcat(output_line, "  ");
    }
    PRNT(output_line);

    PRNT("");
    if (warnings_bit)
    {
        if (table_opt == OPTIMIZE_SPACE)
        {
            if (default_opt < 4)
            {
                PRNTWNG("DEFAULT_OPTion requested must be >= 4");
            }
        }
        else if (shift_default_bit)
        {
            PRNTWNG("SHIFT-DEFAULT option is only valid for Space tables")
        }
    }

/*********************************************************************/
/* Check if there are any conflicts in the options.                  */
/*********************************************************************/
    temp[0] = '\0';

    if (minimum_distance <= 1)
    {
        PRNT("MIN_DISTANCE must be > 1");
        exit(12);
    }
    if (maximum_distance <= minimum_distance + 1)
    {
        PRNT("MAX_DISTANCE must be > MIN_DISTANCE + 1");
        exit(12);
    }
    if (strcmp(blockb, blocke) == 0)
        strcpy(temp, "BLOCKB and BLOCKE");
    else if (strlen(blockb) == 1 && blockb[0] == escape)
        strcpy(temp, "BLOCKB and ESCAPE");
    else if (strlen(blockb) == 1 && blockb[0] == ormark)
        strcpy(temp, "BLOCKB and ORMARK");
    else if (strlen(blocke) == 1 && blocke[0] == escape)
        strcpy(temp, "ESCAPE and BLOCKE");
    else if (strlen(blocke) == 1 && blocke[0] == ormark)
        strcpy(temp, "ORMARK and BLOCKE");
    else if (strcmp(hblockb, hblocke) == 0)
        strcpy(temp, "HBLOCKB and HBLOCKE");
    else if (strlen(hblockb) == 1 && hblockb[0] == escape)
        strcpy(temp, "HBLOCKB and ESCAPE");
    else if (strlen(hblockb) == 1 && hblockb[0] == ormark)
        strcpy(temp, "HBLOCKB and ORMARK");
    else if (strlen(hblocke) == 1 && hblocke[0] == escape)
        strcpy(temp, "ESCAPE and HBLOCKE");
    else if (strlen(hblocke) == 1 && hblocke[0] == ormark)
        strcpy(temp, "ORMARK and HBLOCKE");
    else if (ormark == escape)
        strcpy(temp, "ORMARK and ESCAPE");

    if (temp[0] != '\0')
    {
        sprintf(msg_line,
                "The options %s cannot have the same value", temp);
        PRNTERR(msg_line);
        sprintf(msg_line,
                "Input process aborted at line %d ...", line_no);
        PRNT(msg_line);
        exit(12);
    }

    if (strlen(hblockb) <= strlen(blockb) &&
        memcmp(hblockb, blockb, strlen(hblockb)) == 0)
    {
        sprintf(msg_line,
                "Hblockb value, %s, cannot be a suffix of blockb: %s",
                hblockb, blockb);
        PRNTERR(msg_line);
        sprintf(msg_line,
                "Input process aborted at line %d ...", line_no);
        PRNT(msg_line);
        exit(12);
    }

    return;
} /* end process_options_lines */


/*****************************************************************************/
/*                                 HASH:                                     */
/*****************************************************************************/
/*    HASH takes as argument a symbol and hashes it into a location in       */
/* HASH_TABLE.                                                               */
/*****************************************************************************/
static int hash(char *symb)
{
    register unsigned short k;
    register unsigned long hash_value = 0;

    for (; *symb != '\0'; symb++)
    {
        k = *symb;
        symb++;
        hash_value += ((k << 7) + *symb);
        if (*symb == '\0')
            break;
    }

    return hash_value % HT_SIZE;
}


/*****************************************************************************/
/*                           INSERT_STRING:                                  */
/*****************************************************************************/
/*   INSERT_STRING takes as an argument a pointer to a ht_elemt structure and*/
/* a character string.  It inserts the string into the string table and sets */
/* the value of node to the index into the string table.                     */
/*****************************************************************************/
static void insert_string(struct hash_type *q, char *string)
{
    if (string_offset + strlen(string) >= string_size)
    {
        string_size += STRING_BUFFER_SIZE;
        INT_CHECK(string_size);
        string_table = (char *)
            (string_table == (char *) NULL
             ? malloc(string_size * sizeof(char))
             : realloc(string_table, string_size * sizeof(char)));
        if (string_table == (char *) NULL)
            nospace(__FILE__, __LINE__);
    }

    q -> st_ptr = string_offset;
    while(string_table[string_offset++] = *string++); /* Copy until NULL */
                                                      /* is copied.      */
    return;
}


/*****************************************************************************/
/*                             ASSIGN_SYMBOL_NO:                             */
/*****************************************************************************/
/* PROCESS_SYMBOL takes as an argument a pointer to the most recent token    */
/* which would be either a symbol or a macro name and then processes it. If  */
/* the token is a a macro name then a check is made to see if it is a pre-   */
/* defined macro. If it is then an error message is printed and the program  */
/* is halted. If not, or if the token is a symbol then it is hashed into the */
/* hash_table and its string is copied into the string table.  A struct is   */
/* created for the token. The ST_PTR field contains the index into the string*/
/* table and the NUMBER field is set to zero. Later on if the token is a     */
/* symbol, the value of the NUMBER field is changed to the appropriate symbol*/
/* number. However, if the token is a macro name, its value will remain zero.*/
/* The NAME_INDEX field is set to OMEGA and will be assigned a value later.  */
/*****************************************************************************/
/*   ASSIGN_SYMBOL_NO takes as arguments a pointer to a node and an image    */
/* number and assigns a symbol number to the symbol pointed to by the node.  */
/*****************************************************************************/
static void assign_symbol_no(char *string_ptr, int image)
{
    register int i;
    register struct hash_type *p;

    i = hash(string_ptr);
    for (p = hash_table[i]; p != NULL; p = p -> link)
    {
        if (EQUAL_STRING(string_ptr, p)) /* Are they the same */
            return;
    }

    p = (struct hash_type *) talloc(sizeof(struct hash_type));
    if (p == (struct hash_type *) NULL)
        nospace(__FILE__, __LINE__);

    if (image == OMEGA)
    {
        num_symbols++;
        p -> number = num_symbols;
    }
    else
        p -> number = -image;
    p -> name_index = OMEGA;
    insert_string(p, string_ptr);
    p -> link = hash_table[i];
    hash_table[i] = p;

    return;
}


/*****************************************************************************/
/*                              ALIAS_MAP:                                   */
/*****************************************************************************/
/* ALIAS_MAP takes as input a symbol and an image. It searcheds the hash     */
/* table for stringptr and if it finds it, it turns it into an alias of the  */
/* symbol whose number is IMAGE. Otherwise, it invokes PROCESS_SYMBOL and    */
/* ASSIGN SYMBOL_NO to enter stringptr into the table and then we alias it.  */
/*****************************************************************************/
static void alias_map(char *stringptr, int image)
{
    register struct hash_type *q;

    for (q = hash_table[hash(stringptr)]; q != NULL; q = q -> link)
    {
        if (EQUAL_STRING(stringptr, q))
        {
            q -> number = -image;      /* Mark alias of image */
            return;
        }
    }

    assign_symbol_no(stringptr, image);

    return;
}


/*****************************************************************************/
/*                              SYMBOL_IMAGE:                                */
/*****************************************************************************/
/*    SYMBOL_IMAGE takes as argument a symbol.  It searches for that symbol  */
/* in the HASH_TABLE, and if found, it returns its image; otherwise, it      */
/* returns OMEGA.                                                            */
/*****************************************************************************/
static int symbol_image(char *item)
{
    register struct hash_type *q;

    for (q = hash_table[hash(item)]; q != NULL; q = q -> link)
    {
        if (EQUAL_STRING(item, q))
            return ABS(q -> number);
    }

    return(OMEGA);
}


/*****************************************************************************/
/*                                NAME_MAP:                                  */
/*****************************************************************************/
/* NAME_MAP takes as input a symbol and inserts it into the HASH_TABLE if it */
/* is not yet in the table. If it was already in the table then it is        */
/* assigned a NAME_INDEX number if it did not yet have one.  The name index  */
/* assigned is returned.                                                     */
/*****************************************************************************/
static int name_map(char *symb)
{
    register int i;
    register struct hash_type *p;

    i = hash(symb);
    for (p = hash_table[i]; p != NULL; p = p -> link)
    {
        if (EQUAL_STRING(symb, p))
        {
            if (p -> name_index != OMEGA)
                return(p -> name_index);
            else
            {
                num_names++;
                p -> name_index = num_names;
                return(num_names);
            }
        }
    }

    p = (struct hash_type *) talloc(sizeof(struct hash_type));
    if (p == (struct hash_type *) NULL)
        nospace(__FILE__, __LINE__);
    p -> number = 0;
    insert_string(p, symb);
    p -> link = hash_table[i];
    hash_table[i] = p;

    num_names++;
    p -> name_index = num_names;

    return num_names;
}


/*****************************************************************************/
/*                            PROCESS_GRAMMAR:                               */
/*****************************************************************************/
/*    PROCESS_GRAMMAR is invoked to process the source input. It uses an     */
/* LALR(1) parser table generated by LPG to recognize the grammar which it   */
/* places in the rulehdr structure.                                          */
/*****************************************************************************/
#include "lpgact.i"
#include "lpgact.h"
#include "lpgprs.h"
static void process_grammar(void)
{
    short state_stack[STACK_SIZE];
    register int act;

    scanner();  /* Get first token */
    act = START_STATE;

process_terminal:
/******************************************************************/
/* Note that this driver assumes that the tables are LPG SPACE    */
/* tables with no GOTO-DEFAULTS.                                  */
/******************************************************************/
    state_stack[++stack_top] = act;
    act = t_action(act, ct, ?);

    if (act <= NUM_RULES)             /* Reduce */
        stack_top--;
    else if ((act > ERROR_ACTION) || /* Shift_reduce */
             (act < ACCEPT_ACTION))  /* Shift */
    {
        token_action();
        scanner();
        if (act < ACCEPT_ACTION)
            goto process_terminal;
        act -= ERROR_ACTION;
    }
    else if (act == ACCEPT_ACTION)
    {
        accept_action();
        return;
    }
    else error_action();

process_non_terminal:
    do
    {
        register int lhs_sym = lhs[act]; /* to bypass IBMC12 bug */

        stack_top -= (rhs[act] - 1);
        rule_action[act]();
        act = nt_action(state_stack[stack_top], lhs_sym);
    }   while(act <= NUM_RULES);

    goto process_terminal;
}


/*****************************************************************************/
/*                                SCANNER:                                   */
/*****************************************************************************/
/* SCANNER scans the input stream and returns the next input token.          */
/*****************************************************************************/
static void scanner(void)
{
    register int i;
    register char *p3;

    char  tok_string[SYMBOL_SIZE + 1];

scan_token:
    /*****************************************************************/
    /* Skip "blank" spaces.                                          */
    /*****************************************************************/
    p1 = p2;
    while(IsSpace(*p1))
    {
        if (*(p1++) == '\n')
        {
            if (bufend == input_buffer + IOBUFFER_SIZE)
            {
                i = bufend - p1;
                if (i < MAX_LINE_SIZE)
                {
                    strcpy(input_buffer, p1);
                    bufend = &input_buffer[i];
                    read_input();
                    p1 = &input_buffer[0];
                }
            }
            line_no++;
            linestart = p1 - 1;
        }
    }

    if (strncmp(p1, hblockb, hblockb_len) == 0) /* check block opener */
    {
        p1 = p1 + hblockb_len;
        ct_length = 0;
        ct_ptr = p1;
        if (*p1 == '\n')
        {
            ct_ptr++;
            ct_length--;
            ct_start_line = line_no + 1;
            ct_start_col = 1;
        }
        else
        {
            ct_start_line = line_no;
            ct_start_col = p1 - linestart;
        }

        while(strncmp(p1, hblocke, hblocke_len) != 0)
        {
            if (*p1 == '\0')
            {
                sprintf(msg_line,
                        "End of file encountered while "
                        "scanning header action block in rule %d",
                        num_rules);
                PRNTERR(msg_line);
                exit(12);
            }
            if (*(p1++) == '\n')
            {
                if (bufend == input_buffer + IOBUFFER_SIZE)
                {
                    i = bufend - p1;
                    if (i < MAX_LINE_SIZE)
                    {
                        strcpy(input_buffer, p1);
                        bufend = &input_buffer[i];
                        read_input();
                        p1 = &input_buffer[0];
                    }
                }
                line_no++;
                linestart = p1 - 1;
            }
            ct_length++;
        }
        ct = HBLOCK_TK;
        ct_end_line = line_no;
        ct_end_col = p1 - linestart - 1;
        p2 = p1 + hblocke_len;

        return;
    }
    else if (strncmp(p1, blockb, blockb_len) == 0) /* check block  */
    {
        p1 = p1 + blockb_len;
        ct_length = 0;
        ct_ptr = p1;
        if (*p1 == '\n')
        {
            ct_ptr++;
            ct_length--;
            ct_start_line = line_no + 1;
            ct_start_col = 1;
        }
        else
        {
            ct_start_line = line_no;
            ct_start_col = p1 - linestart;
        }

        while(strncmp(p1, blocke, blocke_len) != 0)
        {
            if (*p1 == '\0')
            {
                sprintf(msg_line,
                        "End of file encountered while "
                        "scanning action block in rule %d", num_rules);
                PRNTERR(msg_line);
                exit(12);
            }
            if (*(p1++) == '\n')
            {
                if (bufend == input_buffer + IOBUFFER_SIZE)
                {
                    i = bufend - p1;
                    if (i < MAX_LINE_SIZE)
                    {
                        strcpy(input_buffer, p1);
                        bufend = &input_buffer[i];
                        read_input();
                        p1 = &input_buffer[0];
                    }
                }
                line_no++;
                linestart = p1 - 1;
            }
            ct_length++;
        }
        ct = BLOCK_TK;
        ct_end_line = line_no;
        ct_end_col = p1 - linestart - 1;
        p2 = p1 + blocke_len;

        return;
    }

/*********************************************************************/
/* Scan the next token.                                              */
/*********************************************************************/
    ct_ptr = p1;
    ct_start_line = line_no;
    ct_start_col = p1 - linestart;

    p2 = p1 + 1;
    switch(*p1)
    {
        case '<':
            if (IsAlpha(*p2))
            {
                p2++;
                while(*p2 != '\n')
                {
                    if (*(p2++) == '>')
                    {
                        ct = SYMBOL_TK;
                        ct_length = p2 - p1;
                        goto check_symbol_length;
                    }
                }
                i = min(SYMBOL_SIZE, p2 - p1);
                memcpy(tok_string, p1, i);
                tok_string[i] = '\0';
                sprintf(msg_line,
                        "Symbol \"%s\""
                        " has been referenced in line %d "
                        " without the closing \">\"",
                        tok_string, ct_start_line);
                PRNTERR(msg_line);
                exit(12);
            }
            break;

        case '\'':
            ct_ptr = p2;
            ct = SYMBOL_TK;
            while(*p2 != '\n')
            {
                if (*p2 == '\'')
                {
                    p2++;
                    if (*p2 != '\'')
                    {
                        ct_length = p2 - p1 - 2;
                        goto remove_quotes;
                    }
                }
                p2++;
            }
            ct_length = p2 - p1 - 1;
            if (warnings_bit)
            {
                memcpy(tok_string, p1, ct_length);
                tok_string[ct_length] = '\0';
                sprintf(msg_line,
                        "Symbol \"%s\" referenced in line %d"
                        " requires a closing quote",
                        tok_string, ct_start_line);
                PRNTWNG(msg_line);
            }

remove_quotes:
            if (ct_length == 0)  /* Empty symbol? disregard it */
                goto scan_token;

            i = 0;
            p1 = ct_ptr;
            do
            {
                *(p1++) = ct_ptr[i++];
                if (ct_ptr[i] == '\'')
                    i++;                       /* skip next quote */
            }   while(i < ct_length);
            ct_length = p1 - ct_ptr;

            goto check_symbol_length;

        case '-':                      /* scan possible comment  */
            if (*p2 == '-')
            {
                p2++;
                while(*p2 != '\n')
                    p2++;
                goto scan_token;
            }
            else if (*p2 == '>' && IsSpace(*(p2+1)))
            {
                ct = ARROW_TK;
                ct_length = 2;
                p2++;
                return;
            }
            break;

        case ':':
            if (*p2 == ':' && *(p2+1) == '=' && IsSpace(*(p2+2)))
            {
                ct = EQUIVALENCE_TK;
                ct_length = 3;
                p2 = p1 + 3;
                return;
            }
            break;

        case '\0': case CTL_Z:                 /* END-OF-FILE? */
            ct = EOF_TK;
            ct_length = 0;
            p2 = p1;

            return;

        default:
            if (*p1 == ormark && IsSpace(*p2))
            {
                ct = OR_TK;
                ct_length = 1;
                return;
            }
            else if (*p1 == escape)       /* escape character? */
            {
                p3 = p2 + 1;
                switch(*p2)
                {
                    case 't': case 'T':
                        if (strxeq(p3, "erminals") && IsSpace(*(p1+10)))
                        {
                            ct = TERMINALS_KEY_TK;
                            ct_length = 10;
                            p2 = p1 + 10;
                            return;
                        }
                        break;

                    case 'd': case 'D':
                        if (strxeq(p3, "efine") && IsSpace(*(p1+7)))
                        {
                            ct = DEFINE_KEY_TK;
                            ct_length = 7;
                            p2 = p1 + 7;
                            return;
                        }
                        break;

                    case 'e': case 'E':
                        if (strxeq(p3, "mpty") && IsSpace(*(p1+6)))
                        {
                            ct = EMPTY_SYMBOL_TK;
                            ct_length = 6;
                            p2 = p1 + 6;
                            return;
                        }
                        if (strxeq(p3, "rror") && IsSpace(*(p1+6)))
                        {
                            ct = ERROR_SYMBOL_TK;
                            ct_length = 6;
                            p2 = p1 + 6;
                            return;
                        }
                        if (strxeq(p3, "ol") && IsSpace(*(p1+4)))
                        {
                            ct = EOL_SYMBOL_TK;
                            ct_length = 4;
                            p2 = p1 + 4;
                            return;
                        }
                        if (strxeq(p3, "of") && IsSpace(*(p1+4)))
                        {
                            ct = EOF_SYMBOL_TK;
                            ct_length = 4;
                            p2 = p1 + 4;
                            return;
                        }
                        if (strxeq(p3, "nd") && IsSpace(*(p1+4)))
                        {
                            ct = END_KEY_TK;
                            ct_length = 4;
                            p2 = p1 + 4;
                            return;
                        }
                        break;

                    case 'r': case 'R':
                        if (strxeq(p3, "ules") && IsSpace(*(p1+6)))
                        {
                            ct = RULES_KEY_TK;
                            ct_length = 6;
                            p2 = p1 + 6;
                            return;
                        }
                        break;

                    case 'a': case 'A':
                        if (strxeq(p3, "lias") && IsSpace(*(p1+6)))
                        {
                            ct = ALIAS_KEY_TK;
                            ct_length = 6;
                            p2 = p1 + 6;
                            return;
                        }
                        break;

                    case 's': case 'S':
                        if (strxeq(p3, "tart") && IsSpace(*(p1+6)))
                        {
                            ct = START_KEY_TK;
                            ct_length = 6;
                            p2 = p1 + 6;
                            return;
                        }
                        break;

                    case 'n': case 'N':
                        if (strxeq(p3, "ames") && IsSpace(*(p1+6)))
                        {
                            ct = NAMES_KEY_TK;
                            ct_length = 6;
                            p2 = p1 + 6;
                            return;
                        }
                        break;

                    default:
                        break;
                }

                ct = MACRO_NAME_TK;
                while(! IsSpace(*p2))
                    p2++;
                ct_length = p2 - p1;
                goto check_symbol_length;
            }
    }

    ct = SYMBOL_TK;
    while(! IsSpace(*p2))
        p2++;
    ct_length = p2 - p1;

check_symbol_length:
    if (ct_length > SYMBOL_SIZE)
    {
        ct_length = SYMBOL_SIZE;
        memcpy(tok_string, p1, ct_length);
        tok_string[ct_length] = '\0';
        if (warnings_bit)
        {
            if (symbol_image(tok_string) == OMEGA)
            {
                sprintf(msg_line,
                        "Length of Symbol \"%s\""
                        " in line %d exceeds maximum of ",
                        tok_string,  line_no);
                PRNTWNG(msg_line);
            }
        }
    }

    return;

}


/****************************************************************************/
/*                             TOKEN_ACTION:                                */
/****************************************************************************/
/*    This function, TOKEN_ACTION, pushes the current token onto the        */
/* parse stack called TERMINAL. Note that in case of a BLOCK_, the name of  */
/* the token is not copied since blocks are processed separately on a       */
/* second pass.                                                             */
/****************************************************************************/
static void token_action(void)
{
    register int top = stack_top + 1;

    terminal[top].kind         = ct;
    terminal[top].start_line   = ct_start_line;
    terminal[top].start_column = ct_start_col;
    terminal[top].end_line     = ct_end_line;
    terminal[top].end_column   = ct_end_col;
    terminal[top].length       = ct_length;

    if (ct != BLOCK_TK)
    {
        memcpy(terminal[top].name, ct_ptr, ct_length);
        terminal[top].name[ct_length] = '\0';
    }
    else terminal[top].name[0] = '\0';

    return;
}

/****************************************************************************/
/*                              ERROR_ACTION:                               */
/****************************************************************************/
/*  Error messages to be printed if an error is encountered during parsing. */
/****************************************************************************/
static void error_action(void)
{
    char tok_string[SYMBOL_SIZE + 1];

    ct_ptr[ct_length] = '\0';

    if (ct == EOF_TK)
        sprintf(msg_line, "End-of file reached prematurely");
    else if (ct == MACRO_NAME_TK)
    {
        sprintf(msg_line,
                "Misplaced macro name \"%s\" found "
                "in line %d, column %d",
                ct_ptr, line_no, ct_start_col);
    }
    else if (ct == SYMBOL_TK)
    {
        restore_symbol(tok_string, ct_ptr);
        sprintf(msg_line,
                "Misplaced symbol \"%s\" found "
                "in line %d, column %d",
                tok_string, line_no, ct_start_col);
    }
    else
    {
        sprintf(msg_line,
                "Misplaced keyword \"%s\" found "
                "in line %d, column %d",
                ct_ptr, line_no, ct_start_col);
    }

    PRNTERR(msg_line);

    exit(12);
}


/****************************************************************************/
/*                            ACCEPT_ACTION:                                */
/****************************************************************************/
/*          Actions to be taken if grammar is successfully parsed.          */
/****************************************************************************/
static void accept_action(void)
{
    if (rulehdr == NULL)
    {
        printf("Informative: Empty grammar read in. "
               "Processing stopped.\n");
        fprintf(syslis, "***Informative: Empty grammar read in. "
                        "Processing stopped.\n");
        fclose(sysgrm);
        fclose(syslis);
        exit(12);
    }

    num_non_terminals = num_symbols - num_terminals;

    if (error_maps_bit)
        make_names_map();

    if (list_bit)
        process_aliases();

    make_rules_map();      /* Construct rule table      */

    fclose(sysgrm);        /* Close grammar input file. */

    if (action_bit)
        process_actions();
    else if (list_bit)
        display_input();

    return;
}


/*****************************************************************************/
/*                             BUILD_SYMNO:                                  */
/*****************************************************************************/
/* BUILD_SYMNO constructs the SYMNO table which is a mapping from each       */
/* symbol number into that symbol.                                           */
/*****************************************************************************/
static void build_symno(void)
{
    register int i,
                 symbol;

    register struct hash_type *p;

    symno_size = num_symbols + 1;
    symno = (struct symno_type *)
            calloc(symno_size, sizeof(struct symno_type));
    if (symno == (struct symno_type *) NULL)
        nospace(__FILE__, __LINE__);

    /***********************************************************************/
    /* Go thru entire hash table. For each non_empty bucket, go through    */
    /* linked list in that bucket.                                         */
    /***********************************************************************/
    for (i = 0; i < HT_SIZE; ++i)
    {
        for (p = hash_table[i]; p != NULL; p = p -> link)
        {
            symbol = p -> number;
            if (symbol >= 0) /* Not an alias */
            {
                symno[symbol].name_index = OMEGA;
                symno[symbol].ptr = p -> st_ptr;
            }
        }
    }

    return;
}


/****************************************************************************/
/*                              PROCESS_ACTIONS:                            */
/****************************************************************************/
/*     Process all semantic actions and generate action file.               */
/****************************************************************************/
static void process_actions(void)
{
    register int i,
                 j,
                 k,
                 len;
    register char *p;

    char  line[MAX_LINE_SIZE + 1];

#if defined(C370) && !defined(CW)
    sprintf(msg_line, "w, recfm=%cB, lrecl=%d",
                      record_format, output_size);
    if (record_format != 'F')
        output_size -= 4;
    sysact  = fopen(act_file,  msg_line);
    syshact = fopen(hact_file, msg_line);
#else
    sysact  = fopen(act_file,  "w");
    syshact = fopen(hact_file, "w");
#endif

    if (sysact == (FILE *) NULL)
    {
        fprintf(stderr,
                "***ERROR: Action file \"%s\" cannot be opened.\n",
                act_file);
        exit(12);
    }

    if (syshact == (FILE *) NULL)
    {
        fprintf(stderr,
                "***ERROR: Header Action file "
                "\"%s\" cannot be opened.\n", hact_file);
        exit(12);
    }

    if ((sysgrm = fopen(grm_file, "r")) == (FILE *) NULL)
    {
        fprintf(stderr,
                "***ERROR: Input file %s containing grammar "
                "is empty, undefined, or invalid\n",grm_file);
        exit(12);
    }

    macro_table = Allocate_short_array(HT_SIZE);
    for (i = 0; i < HT_SIZE; i++)
        macro_table[i] = NIL;

    bufend = &input_buffer[0];
    read_input();

    p2 = &input_buffer[0];
    linestart = p2 - 1;
    p1 = p2;
    line_no = 1;

/***********************************************************************/
/* Read in all the macro definitions and insert them into macro_table. */
/***********************************************************************/
    for (i = 0; i < num_defs; i++)
    {
        defelmt[i].macro = (char *)
                           calloc(defelmt[i].length + 2, sizeof(char));
        if (defelmt[i].macro == (char *) NULL)
            nospace(__FILE__, __LINE__);

        for (; line_no < defelmt[i].start_line; line_no++)
        {
            while (*p1 != '\n')
                p1++;
            p1++;
            if (bufend == input_buffer + IOBUFFER_SIZE)
            {
                k = bufend - p1;
                if (k < MAX_LINE_SIZE)
                {
                    strcpy(input_buffer, p1);
                    bufend = &input_buffer[k];
                    read_input();
                    p1 = &input_buffer[0];
                }
            }
            linestart = p1 - 1;
        }

        p1 = linestart + defelmt[i].start_column;

        for (j = 0; j < defelmt[i].length; j++)
        {
            defelmt[i].macro[j] = *p1;
            if (*(p1++) == '\n')
            {
                if (bufend == input_buffer + IOBUFFER_SIZE)
                {
                    k = bufend - p1;
                    if (k < MAX_LINE_SIZE)
                    {
                        strcpy(input_buffer, p1);
                        bufend = &input_buffer[k];
                        read_input();
                        p1 = &input_buffer[0];
                    }
                }
                line_no++;
                linestart = p1 - 1;
            }
        }
        defelmt[i].macro[defelmt[i].length] = '\n';
        defelmt[i].macro[defelmt[i].length + 1] = '\0';

        for (p = defelmt[i].name; *p != '\0'; p++)
        {
            *p = (isupper(*p) ? tolower(*p) : *p);
        }

        mapmacro(i);
    }

/****************************************************************************/
/* If LISTING was requested, invoke listing procedure.                      */
/****************************************************************************/
    if (list_bit)
        display_input();

/****************************************************************************/
/* Read in all the action blocks and process them.                          */
/****************************************************************************/
    for (i = 0; i < num_acts; i++)
    {
        for (; line_no < actelmt[i].start_line; line_no++)
        {
            while (*p1 != '\n')
                p1++;
            p1++;
            if (bufend == input_buffer + IOBUFFER_SIZE)
            {
                k = bufend - p1;
                if (k < MAX_LINE_SIZE)
                {
                    strcpy(input_buffer, p1);
                    bufend = &input_buffer[k];
                    read_input();
                    p1 = &input_buffer[0];
                }
            }
            linestart = p1 - 1;
        }

        if (actelmt[i].start_line == actelmt[i].end_line)
        {
            len = actelmt[i].end_column -
                  actelmt[i].start_column + 1;
            memcpy(line, linestart + actelmt[i].start_column, len);
            line[len] = '\0';

            while (*p1 != '\n')
                p1++;
        }
        else
        {
            p = line;
            p1 = linestart + actelmt[i].start_column;
            while (*p1 != '\n')
                *(p++) = *(p1++);
            *p = '\0';
        }

        if (actelmt[i].header_block)
             process_action_line(syshact, line, line_no,
                                 actelmt[i].rule_number);
        else process_action_line(sysact, line, line_no,
                                 actelmt[i].rule_number);

        if (line_no != actelmt[i].end_line)
        {
            while (line_no < actelmt[i].end_line)
            {
                p1++;
                if (bufend == input_buffer + IOBUFFER_SIZE)
                {
                    k = bufend - p1;
                    if (k < MAX_LINE_SIZE)
                    {
                        strcpy(input_buffer, p1);
                        bufend = &input_buffer[k];
                        read_input();
                        p1 = &input_buffer[0];
                    }
                }
                line_no++;
                linestart = p1 - 1;

                if (line_no < actelmt[i].end_line)
                {
                    p = line;
                    while (*p1 != '\n')
                        *(p++) = *(p1++);
                    *p = '\0';

                    if (actelmt[i].header_block)
                         process_action_line(syshact, line, line_no,
                                             actelmt[i].rule_number);
                    else process_action_line(sysact, line, line_no,
                                             actelmt[i].rule_number);
                }
            }

            if (actelmt[i].end_column != 0)
            {
                len = actelmt[i].end_column;
                memcpy(line, p1, len);
                line[len] = '\0';

                if (actelmt[i].header_block)
                     process_action_line(syshact, line, line_no,
                                         actelmt[i].rule_number);
                else process_action_line(sysact, line, line_no,
                                         actelmt[i].rule_number);
            }
        }
    }

    for (i = 0; i < num_defs; i++)
    {
        ffree(defelmt[i].macro);
    }

    ffree(defelmt);
    ffree(actelmt);
    fclose(sysgrm);  /* Close grammar file and reopen it. */
    fclose(sysact);
    fclose(syshact);

    return;
}



/************************************************************************/
/*                          MAKE_NAMES_MAP:                             */
/************************************************************************/
/*  Construct the NAME map, and update the elements of SYMNO with their */
/* names.                                                               */
/************************************************************************/
static void make_names_map(void)
{
    register int i,
                 symbol;

    register struct hash_type *p;

    symno[accept_image].name_index = name_map("");

    if (error_image == DEFAULT_SYMBOL)
        symno[DEFAULT_SYMBOL].name_index = symno[accept_image].name_index;

    for ALL_TERMINALS(symbol)
    {
        if (symno[symbol].name_index == OMEGA)
            symno[symbol].name_index = name_map(RETRIEVE_STRING(symbol));
    }

    for ALL_NON_TERMINALS(symbol)
    {
        if (symno[symbol].name_index == OMEGA)
        {
            if (names_opt == MAXIMUM_NAMES)
                symno[symbol].name_index = name_map(RETRIEVE_STRING(symbol));
            else if (names_opt == OPTIMIZE_PHRASES)
                symno[symbol].name_index = - name_map(RETRIEVE_STRING(symbol));
            else
                symno[symbol].name_index = symno[error_image].name_index;
        }
    }

    /************************/
    /* Allocate NAME table. */
    /************************/
    name = (int *) calloc(num_names + 1, sizeof(int));
    if (name == (int *) NULL)
        nospace(__FILE__, __LINE__);

    for (i = 0; i < HT_SIZE; i++)
    {
        for (p = hash_table[i]; p != NULL; p = p -> link)
        {
            if (p -> name_index != OMEGA)
                name[p -> name_index] = p -> st_ptr;
        }
    }

    return;
}



/*****************************************************************************/
/*                             MAKE_RULES_MAP:                               */
/*****************************************************************************/
/*   Construct the rule table.  At this stage, NUM_ITEMS is equal to the sum */
/* of the right-hand side lists of symbols.  It is used in the declaration of*/
/* RULE_TAB.  After RULE_TAB is allocated, we increase NUM_ITEMS to its      */
/* correct value.  Recall that the first rule is numbered 0; therefore we    */
/* increase the number of items by 1 to reflect this numbering.              */
/*****************************************************************************/
static void make_rules_map(void)
{
    register struct node *ptr, *q;

    char temp[SYMBOL_SIZE + 1];

    register int i = 0,
                 rhs_ct = 0;

    rules = (struct ruletab_type *)
            calloc(num_rules + 2, sizeof(struct ruletab_type));
    if (rules == (struct ruletab_type *) NULL)
        nospace(__FILE__, __LINE__);

    rhs_sym = Allocate_short_array(num_items + 1);

    num_items += (num_rules + 1);
    SHORT_CHECK(num_items);

/*****************************************************************************/
/* Put starting rules from start symbol linked list in rule and rhs table    */
/*****************************************************************************/
    if (start_symbol_root != NULL)
    {
        /* Turn circular list into linear */
        q = start_symbol_root;
        start_symbol_root = q -> next;
        q -> next = NULL;

        for (ptr = start_symbol_root; ptr != NULL; ptr = ptr -> next)
        {
            rules[i].lhs = accept_image;
            rules[i].sp = 0;
            rules[i++].rhs = rhs_ct;
            if (ptr -> value != empty)
                rhs_sym[rhs_ct++] = ptr -> value;
        }

        free_nodes(start_symbol_root, q);
    }

/*****************************************************************************/
/*   In this loop, the grammar is placed in the rule table structure and the */
/* right-hand sides are placed in the RHS table.  A check is made to prevent */
/* terminals from being used as left hand sides.                             */
/*****************************************************************************/
    for (i = i; i <= num_rules; i++)
    {
        rules[i].rhs = rhs_ct;
        ptr = rulehdr[i].rhs_root;
        if (ptr != NULL) /* not am empty right-hand side? */
        {
            do
            {
                ptr = ptr -> next;
                rhs_sym[rhs_ct++] = ptr -> value;
            }  while (ptr != rulehdr[i].rhs_root);
            ptr = ptr -> next;                /* point to 1st element */
            rules[i].sp = (rulehdr[i].sp && ptr == rulehdr[i].rhs_root);
            if (rules[i].sp)
                num_single_productions++;
            free_nodes(ptr, rulehdr[i].rhs_root);
        }
        else
            rules[i].sp = FALSE;

        if (rulehdr[i].lhs == OMEGA)
        {
            if (list_bit) /* Proper LHS will be updated after printing */
                rules[i].lhs = OMEGA;
            else rules[i].lhs = rules[i - 1].lhs;
        }
        else if (rulehdr[i].lhs IS_A_TERMINAL)
        {
            restore_symbol(temp, RETRIEVE_STRING(rulehdr[i].lhs));
            sprintf(msg_line,
                    "In rule %d: terminal \"%s\" used as left hand side",
                    i, temp);
            PRNTERR(msg_line);
            PRNTERR("Processing terminated due to input errors.");
            exit(12);
        }
        else rules[i].lhs = rulehdr[i].lhs;
    }

    rules[num_rules + 1].rhs = rhs_ct; /* Fence !! */

    return;
}


/****************************************************************************/
/*                             ALLOC_LINE:                                  */
/****************************************************************************/
/*  This function allocates a line_elemt structure and returns a pointer    */
/* to it.                                                                   */
/****************************************************************************/
static struct line_elemt *alloc_line(void)
{
    register struct line_elemt *p;

    p = line_pool_root;
    if (p != NULL)
        line_pool_root = p -> link;
    else
    {
        p = (struct line_elemt *) talloc(sizeof(struct line_elemt));
        if (p == (struct line_elemt *) NULL)
            nospace(__FILE__, __LINE__);
    }

    return(p);
}


/****************************************************************************/
/*                              FREE_LINE:                                  */
/****************************************************************************/
/*  This function frees a line_elemt structure which is returned to a free  */
/* pool.                                                                    */
/****************************************************************************/
static void free_line(struct line_elemt *p)
{
    p -> link = line_pool_root;
    line_pool_root = p;

    return;
}


/*****************************************************************************/
/*                            PROCESS_ACTION_LINE:                           */
/*****************************************************************************/
/* PROCESS_ACTION_LINE takes as arguments a line of text from an action      */
/* block and the rule number with which thwe block is associated.            */
/* It first scans the text for predefined macro names and then for           */
/* user defined macro names. If one is found, the macro definition is sub-   */
/* stituted for the name. The modified action text is then printed out in    */
/* the action file.                                                          */
/*****************************************************************************/
static void process_action_line(FILE *sysout, char *text,
                                int line_no, int rule_no)
{
    register int j,
                 k,
                 text_len;

    char temp1[MAX_LINE_SIZE + 1];
    char temp2[MAX_LINE_SIZE + 1];
    char suffix[MAX_LINE_SIZE + 1];
    char symbol[SYMBOL_SIZE + 1];

    struct line_elemt *q;
    struct line_elemt *root = NULL;
    struct line_elemt *input_line_root = NULL;

next_line:
   text_len = strlen(text);
   k = 0; /* k is the cursor */
   while (k < text_len)
   {
       if (text[k] == escape) /* all macro names begin with the ESCAPE */
       {                      /* character                             */
           if (k + 12 <= text_len) /* 12 is length of %rule_number and */
                                   /* %num_symbols.                    */
           {
               if (strxeq(text + k, krule_number))
               {
                   strcpy(temp1, text + k + 12);
                   if (k + 12 != text_len)
                       sprintf(text + k, "%d%s", rule_no, temp1);
                   else
                       sprintf(text + k, "%d", rule_no);
                   goto proceed;
               }

               if (strxeq(text + k, knum_symbols))
               {
                   strcpy(temp1, text + k + 12);
                   if (k + 12 != text_len)
                       sprintf(text + k, "%d%s", num_symbols, temp1);
                   else
                       sprintf(text + k, "%d", num_symbols);
                   goto proceed;
               }
           }

           if (k + 11 <= text_len) /* 11 is the length of %input_file    */
           {
               if (strxeq(text + k, kinput_file))
               {
                   strcpy(temp1, text + k + 11);
                   if (k + 11 != text_len)
                       sprintf(text + k, "%s%s", grm_file, temp1);
                   else
                       sprintf(text + k, "%s", grm_file);
                   goto proceed;
               }
           }

           if (k + 10 <= text_len) /* 10 is the length of %rule_size and */
                                /* %rule_text, %num_rules and %next_line */
           {
               if (strxeq(text + k, krule_text))
               {
                   register int max_len;

                   if (k + 10 != text_len)
                   {
                       strcpy(temp1, text + k + 10);

                       /* Remove trailing blanks */
                       for (j = strlen(temp1) - 1;
                            j >= 0 && temp1[j] == ' '; j--);

                       if (j != 0)  /* if not a string of blanks */
                           temp1[++j] = '\0';
                       else
                           temp1[0] = '\0';
                   }
                   else
                   {
                       temp1[0] = '\0';
                       j = 0;
                   }
                   max_len = output_size - k - j;

                   restore_symbol(temp2,
                                  RETRIEVE_STRING(rules[rule_no].lhs));
                   if (rules[rule_no].sp) /* if a single production */
                       strcat(temp2, " ->");
                   else
                       strcat(temp2, " ::=");

                   if (strlen(temp2) > max_len)
                       strcpy(temp2, " ... ");
                   else /* Copy right-hand-side symbols to temp2 */
                   {
                       for ENTIRE_RHS(j, rule_no)
                       {
                           restore_symbol(symbol, RETRIEVE_STRING(rhs_sym[j]));
                           if (strlen(temp2) + strlen(symbol) + 1 < max_len)
                           {
                               strcat(temp2, BLANK);
                               strcat(temp2, symbol);
                           }
                           else
                           {
                                if (strlen(temp2) + 3 < max_len)
                                    strcat(temp2, "...");
                                break;
                           }
                       }
                   }
                   text[k] = '\0';
                   strcat(text, temp2);
                   strcat(text, temp1);
                   k = k - 1 + strlen(temp2); /* Adjust cursor */
                   goto proceed;
               }

               if (strxeq(text + k, krule_size))
               {
                   strcpy(temp1, text + k + 10);
                   if (k + 10 != text_len)
                       sprintf(text + k, "%d%s", RHS_SIZE(rule_no), temp1);
                   else
                       sprintf(text + k, "%d", RHS_SIZE(rule_no));
                   goto proceed;
               }

               if (strxeq(text + k, knext_line))
               {
                   strcpy(temp1, text + k + 10);
                   if (k + 10 != text_len)
                       sprintf(text + k, "%d%s", line_no + 1, temp1);
                   else
                       sprintf(text + k, "%d", line_no + 1);
                   goto proceed;
               }

               if (strxeq(text + k, knum_rules))
               {
                   strcpy(temp1, text + k + 10);
                   if (k + 10 != text_len)
                       sprintf(text + k, "%d%s", num_rules, temp1);
                   else
                       sprintf(text + k, "%d", num_rules);
                   goto proceed;
               }
           }

           if (k + 13 <= text_len) /* 13 is length of %current_line  */
           {
               if (strxeq(text + k, kcurrent_line))
               {
                   strcpy(temp1, text + k + 13);
                   if (k + 13 != text_len)
                       sprintf(text + k, "%d%s", line_no, temp1);
                   else
                       sprintf(text + k, "%d", line_no);
                   goto proceed;
               }
           }

           if (k + 14 <= text_len) /* 14 is length of %num_terminals */
           {
               if (strxeq(text + k, knum_terminals))
               {
                   strcpy(temp1, text + k + 14);
                   if (k + 14 != text_len)
                       sprintf(text + k, "%d%s", num_terminals, temp1);
                   else
                       sprintf(text + k, "%d", num_terminals);
                   goto proceed;
               }
           }

           if (k + 18 <= text_len) /* 18 is length of %num_non_terminals */
           {
               if (strxeq(text + k, knum_non_terminals))
               {
                   strcpy(temp1, text + k + 18);
                   if (k + 18 != text_len)
                       sprintf(text + k, "%d%s", num_non_terminals, temp1);
                   else
                       sprintf(text + k, "%d", num_non_terminals);
                   goto proceed;
               }
           }

/*****************************************************************************/
/* Macro in question is not one of the predefined macros. Try user-defined   */
/* macro list.                                                               */
/*****************************************************************************/
           /* find next delimeter */
           for (j = k + 1; j < text_len && !IsSpace(text[j]); ++j)
               ;

           memcpy(symbol, text + k, j - k); /* copy macro name into symbol */
           symbol[j - k] = '\0';

           if (j < text_len) /* Is there any text after macro ? */
               strcpy(suffix, text + j); /* Copy rest of text into "suffix". */
           else
               suffix[0] = '\0';
           text[k] = '\0';  /* prefix before macro */

           root = find_macro(symbol); /* "root" points to a circular  */
                                      /* linked list of line_elemt(s) */
                                      /* containing macro definition. */

           if (root != NULL)  /* if macro name was found */
           {
               struct line_elemt *tail;

               q = root;
               root = root -> link;
               if (suffix[0] != '\0')
               {
                   /**********************************************/
                   /* If there is room to add the suffix to the  */
                   /* last macro line then do it. Or else        */
                   /* allocate a new line_elemt, copy the suffix */
                   /* into it and add it to the list of lines to */
                   /* be processed.                              */
                   /**********************************************/
                   if (strlen(q -> line) + strlen(suffix) < output_size)
                   {
                       strcat(q -> line, suffix);
                       tail = q;
                   }
                   else
                   {
                       tail = alloc_line();
                       strcpy(tail -> line, suffix);
                       q -> link = tail;
                   }
               }
               else
                   tail = q;
               tail -> link = NULL; /* make circular list linear */

               /****************************************************/
               /* If there is space for the first macro line to be */
               /* added to the prefix then do it.                  */
               /****************************************************/
               if (strlen(text) + strlen(root -> line) < output_size)
               {
                   strcat(text, root -> line);
                   q = root;
                   root = root -> link;
                   free_line(q);
               }

               /*********************************************/
               /* if there are more macro lines to process, */
               /* add list to list headed by INPUT_LINE_ROOT*/
               /*********************************************/
               if (root != NULL)
               {
                   tail -> link = input_line_root;
                   input_line_root = root;
               }
               k--;
           }
           else /* If macro name is not found then rebuild line.*/
           {
               strcat(text, symbol);
               if (suffix[0] != '\0')
                   strcat(text, suffix);
               k = j;
           }

proceed:
           text_len = strlen(text);
       }
       ++k;
   }

/*****************************************************************************/
/* If text is greater than output size, print error message and truncate     */
/* line.                                                                     */
/*****************************************************************************/
   if (strlen(text) > output_size)
   {
       for (j = strlen(text) - 1; j >= output_size; j--)
       {
           if (text[j] != ' ')
           {
               sprintf(msg_line,
                       "Size of output line \"%s\""
                       " is greater than OUTPUT_SIZE", text);
               PRNTERR(msg_line);
               break;
           }
       }
       text[output_size] = '\0';
   }

   fprintf(sysout, "%s\n", text);

   /* If there is another macro line copy it to TEXT and then process it. */
   if (input_line_root != NULL)
   {
       strcpy(text, input_line_root -> line);
       q = input_line_root;
       input_line_root = input_line_root -> link;

       free_line(q);

       goto next_line;
   }
   return;
}


/****************************************************************************/
/*                                MAPMACRO:                                 */
/****************************************************************************/
/* This procedure takes as argument a macro definition.  If the name of the */
/* macro is one of the predefined names, it issues an error.  Otherwise, it */
/* inserts the macro definition into the table headed by MACRO_TABLE.       */
/****************************************************************************/
static void mapmacro(int def_index)
{
    register int i,
                 j;

    if (strcmp(defelmt[def_index].name, krule_text)         == 0 ||
        strcmp(defelmt[def_index].name, krule_number)       == 0 ||
        strcmp(defelmt[def_index].name, knum_rules)         == 0 ||
        strcmp(defelmt[def_index].name, krule_size)         == 0 ||
        strcmp(defelmt[def_index].name, knum_terminals)     == 0 ||
        strcmp(defelmt[def_index].name, knum_non_terminals) == 0 ||
        strcmp(defelmt[def_index].name, knum_symbols)       == 0 ||
        strcmp(defelmt[def_index].name, kinput_file)        == 0 ||
        strcmp(defelmt[def_index].name, kcurrent_line)      == 0 ||
        strcmp(defelmt[def_index].name, knext_line)         == 0)
    {
        sprintf(msg_line, "predefined macro \"%s\""
                          " cannot be redefined. Line %d",
                          defelmt[def_index].name,
                          defelmt[def_index].start_line);
        PRNTWNG(msg_line);
        return;
    }

    i = hash(defelmt[def_index].name);
    for (j = macro_table[i]; j != NIL; j = defelmt[j].next)
    {
        if (strcmp(defelmt[j].name, defelmt[def_index].name) == 0)
        {
            if (warnings_bit)
            {
                sprintf(msg_line, "Redefinition of macro \"%s\""
                                  " in line %d",
                                  defelmt[def_index].name,
                                  defelmt[def_index].start_line);
                PRNTWNG(msg_line);
                break;
            }
        }
    }

    defelmt[def_index].next = macro_table[i];
    macro_table[i] = def_index;

    return;
}


/*****************************************************************************/
/*                                FIND_MACRO:                                */
/*****************************************************************************/
/* FIND_MACRO takes as argument a pointer to a macro name. It searches for   */
/* the macro name in the hash table based on MACRO_TABLE. If the macro name  */
/* is found then the macro definition associated with it is returned.        */
/* If the name is not found, then a message is printed, a new definition is  */
/* entered so as to avoid more messages and NULL is returned.                */
/*****************************************************************************/
static struct line_elemt *find_macro(char *name)
{
    register int i,
                 j;
    register char *ptr,
                  *s;

    register struct line_elemt *q,
                               *root = NULL;
    char macro_name[MAX_LINE_SIZE + 1];

    s = macro_name;
    for (ptr = name; *ptr != '\0'; ptr++)
    {
        *(s++) = (isupper(*ptr) ? tolower(*ptr) : *ptr);
    }
    *s = '\0';

    i = hash(macro_name);
    for (j = macro_table[i]; j != NIL; j = defelmt[j].next)
    {
        if (strcmp(macro_name, defelmt[j].name) == 0)
        {
            ptr = defelmt[j].macro;

            if (ptr) /* undefined macro? */
            {
                while(*ptr != '\0')
                {
                    q = alloc_line();
                    s = q -> line;
                    while(*ptr != '\n')
                        *(s++) = *(ptr++);
                    *s = '\0';
                    ptr++; /* skip newline marker */

                    if (root == NULL)
                        q -> link = q; /* make circular */
                    else
                    {
                        q -> link = root -> link;
                        root -> link = q;
                    }
                    root = q;
                }
            }

            return root;
        }
    }

    /**********************************************************/
    /* Make phony definition for macro so as to avoid future  */
    /* errors.                                                */
    /**********************************************************/
    if (num_defs >= (int)defelmt_size)
    {
        defelmt_size += DEFELMT_INCREMENT;
        defelmt = (struct defelmt_type *)
            (defelmt == (struct defelmt_type *) NULL
             ? malloc(defelmt_size * sizeof(struct defelmt_type))
             : realloc(defelmt, defelmt_size*sizeof(struct defelmt_type)));
        if (defelmt == (struct defelmt_type *) NULL)
            nospace(__FILE__, __LINE__);
    }

    strcpy(defelmt[num_defs].name, macro_name);
    defelmt[num_defs].length = 0;
    defelmt[num_defs].macro = NULL;

    defelmt[num_defs].next = macro_table[i];
    macro_table[i] = num_defs;
    num_defs++;

    return NULL;
}



/*************************************************************************/
/*                            PROCESS_ALIASES:                           */
/*************************************************************************/
/* Aliases are placed in a separate linked list.  NOTE!! After execution */
/* of this loop the hash_table is destroyed because the LINK field of    */
/* alias symbols is used to construct a list of the alias symbols.       */
/*************************************************************************/
static void process_aliases(void)
{
    register int i;
    register struct hash_type *p,
                              *tail;

    for (i = 0; i < HT_SIZE; i++)
    {
        tail = hash_table[i];
        for (p = tail; p != NULL; p = tail)
        {
            tail = p -> link;

            if (p -> number < 0)
            {
                p -> link = alias_root;
                alias_root = p;
            }
        }
    }

    return;
}


/*****************************************************************************/
/*                             DISPLAY_INPUT:                                */
/*****************************************************************************/
/*   If a listing is requested, this prints all the macros(if any), followed */
/* by the aliases(if any), followed by the terminal symbols, followed by the */
/* rules.                                                                    */
/*   This grammar information is printed on lines no longer than             */
/* PRINT_LINE_SIZE characters long.  If all the symbols in a rule cannot fit */
/* on one line, it is continued on a subsequent line beginning at the        */
/* position after the equivalence symbol (::= or ->) or the middle of the    */
/* print_line, whichever is smaller.  If a symbol cannot fit on a line       */
/* beginning at the proper offset, it is laid out on successive lines,       */
/* beginning at the proper offset.                                           */
/*****************************************************************************/
static void display_input(void)
{
    register int j,
                 len,
                 offset,
                 symb,
                 rule_no;
    char line[PRINT_LINE_SIZE + 1],
         temp[SYMBOL_SIZE + 1];

    /******************************************/
    /* Print the Macro definitions, if any.   */
    /******************************************/
    if (num_defs > 0)
    {
        PR_HEADING;
        fprintf(syslis, "\nDefined Symbols:\n\n");
        output_line_no += 3;
        for (j = 0; j < num_defs; j++)
        {
            char *ptr;

            fill_in(line, (PRINT_LINE_SIZE - (strlen(blockb)+1)), '-');
            fprintf(syslis, "\n\n%s\n%s%s\n",
                            defelmt[j].name, blockb, line);

            output_line_no += 4;

            for (ptr = defelmt[j].macro; *ptr != '\0'; ptr++)
            {
                for (; *ptr != '\n'; ptr++)
                {
                    putc(*ptr, syslis);
                }
                putc(*ptr, syslis);
                ENDPAGE_CHECK;
            }

            fill_in(line, (PRINT_LINE_SIZE - (strlen(blocke) + 1)), '-');
            fprintf(syslis, "%s%s\n", blocke, line);
            ENDPAGE_CHECK;
        }
    }


    /**********************************/
    /*   Print the Aliases, if any.   */
    /**********************************/
    if (alias_root != NULL)
    {
        struct hash_type *p;

        PR_HEADING;
        if (alias_root -> link == NULL)
        {
            fprintf(syslis, "\nAlias:\n\n");
            output_line_no += 3;
        }
        else
        {
            fprintf(syslis, "\nAliases:\n\n");
            output_line_no += 3;
        }

        for (p = alias_root; p != NULL; p = p -> link)
        {
            restore_symbol(temp, EXTRACT_STRING(p -> st_ptr));

            len = PRINT_LINE_SIZE - 5;
            print_large_token(line, temp, "", len);
            strcat(line, " ::= ");
            symb = -(p -> number);
            restore_symbol(temp, RETRIEVE_STRING(symb));

            if (strlen(line) + strlen(temp) > PRINT_LINE_SIZE)
            {
                fprintf(syslis, "%s\n", line);
                ENDPAGE_CHECK;
                len = PRINT_LINE_SIZE - 4;
                print_large_token(line, temp, "    ", len);
            }
            else
                strcat(line, temp);
            fprintf(syslis, "%s\n", line);
            ENDPAGE_CHECK;
        }
    }

 /***************************************************************************/
 /*   Print the terminals.                                                  */
 /*   The first symbol (#1) represents the empty string.  The last terminal */
 /* declared by the user is followed by EOFT which may be followed by the   */
 /* ERROR symbol.  See LPG GRAMMAR for more details.                        */
 /***************************************************************************/
    PR_HEADING;
    fprintf(syslis, "\nTerminals:\n\n");
    output_line_no += 3;
    strcpy(line, "        ");  /* 8 spaces */
    len = PRINT_LINE_SIZE - 4;

    for (symb = 2; symb <= num_terminals; symb++)
    {
        restore_symbol(temp, RETRIEVE_STRING(symb));

        if (strlen(line) + strlen(temp) > PRINT_LINE_SIZE)
        {
            fprintf(syslis, "\n%s", line);
            ENDPAGE_CHECK;
            print_large_token(line, temp, "    ", len);
        }
        else
            strcat(line, temp);
        strcat(line, BLANK);
    }
    fprintf(syslis, "\n%s", line);
    ENDPAGE_CHECK;

    /**************************/
    /*    Print the Rules     */
    /**************************/
    PR_HEADING;
    fprintf(syslis, "\nRules:\n\n");
    output_line_no += 3;
    for (rule_no = 0; rule_no <= num_rules; rule_no++)
    {
        register int i;

        symb = rules[rule_no].lhs;
        sprintf(line, "%-4d  ", rule_no);
        if (symb != OMEGA)
        {
            restore_symbol(temp, RETRIEVE_STRING(symb));
            if (strlen(temp) > PRINT_LINE_SIZE - 12)
            {
                strncat(line, temp, PRINT_LINE_SIZE - 12);
                fprintf(syslis, "\n%s", line);
                ENDPAGE_CHECK;
                strcpy(temp, temp + (PRINT_LINE_SIZE - 12));
                i = PRINT_LINE_SIZE - 12;
                print_large_token(line, temp, "       ", i);
            }
            else
                strcat(line, temp);

            if (rules[rule_no].sp)
                strcat(line, " -> ");
            else
                strcat(line, " ::= ");

            i = (PRINT_LINE_SIZE / 2) + 1;
            offset = MIN(strlen(line) - 1, i);
            len = PRINT_LINE_SIZE - offset - 1;
        }
        else
        {
            symb = rules[rule_no - 1].lhs;
            rules[rule_no].lhs = symb; /* update rules map */
            if (rules[rule_no].sp)
            {
                restore_symbol(temp, RETRIEVE_STRING(symb));
                if (strlen(temp) > PRINT_LINE_SIZE - 12)
                {
                    strncat(line, temp, PRINT_LINE_SIZE - 12);
                    fprintf(syslis, "\n%s", line);
                    ENDPAGE_CHECK;
                    strcpy(temp, temp + (PRINT_LINE_SIZE - 12));
                    i = PRINT_LINE_SIZE - 12;
                    print_large_token(line, temp, "       ", i);
                }
                else
                    strcat(line, temp);
                strcat(line, "  -> ");
            }
            else
            {
                for (i = 1; i <= offset - 7; i++)
                    strcat(line, BLANK);
                strcat(line, "| ");
            }
        }

        for ENTIRE_RHS(i, rule_no)
        {
            restore_symbol(temp, RETRIEVE_STRING(rhs_sym[i]));
            if (strlen(temp) + strlen(line) > PRINT_LINE_SIZE - 1)
            {
                char tempbuffer1[SYMBOL_SIZE + 1];

                fprintf(syslis, "\n%s", line);
                ENDPAGE_CHECK;
                strcpy(tempbuffer1, BLANK);
                for (j = 1; j < offset + 1; j++)
                     strcat(tempbuffer1, BLANK);
                print_large_token(line, temp, tempbuffer1, len);
            }
            else
                strcat(line, temp);
            strcat(line, BLANK);
        }
        fprintf(syslis, "\n%s", line);
        ENDPAGE_CHECK;
    }

    return;
}
