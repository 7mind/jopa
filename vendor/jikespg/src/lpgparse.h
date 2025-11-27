/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                        OPTIONS DECLARATIONS                   **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
/* The following static variables are used only in processing the  */
/* options.                                                        */
/*******************************************************************/
#define OUTPUT_PARM_SIZE MAX_PARM_SIZE + 7
#define MAXIMUM_LA_LEVEL 15
#define STRING_BUFFER_SIZE 8192

#if defined(VM) || defined(CW)
static char han[9] = "",
            hat[9] = "INCLUDE",
            ham[3] = "A",
            an[9]  = "",
            at[9]  = "ACTION",
            am[3]  = "A",
            pn[9]  = "",
            pt[9]  = "H",
            pm[3]  = "A",
            sn[9]  = "",
            st[9]  = "H",
            sm[3]  = "A";
#endif

static const char *oaction             = "ACTION",
                  *oactfile_name       = "ACTFILENAME",
                  *oactfile_name2      = "ACTFILE-NAME",
                  *oactfile_name3      = "ACTFILE_NAME",
#if defined(VM) || defined(CW)
                  *oactfile_type       = "ACTFILETYPE",
                  *oactfile_type2      = "ACTFILE-TYPE",
                  *oactfile_type3      = "ACTFILE_TYPE",
                  *oactfile_mode       = "ACTFILEMODE",
                  *oactfile_mode2      = "ACTFILE-MODE",
                  *oactfile_mode3      = "ACTFILE_MODE",
#endif
                  *oblockb             = "BLOCKB",
                  *oblocke             = "BLOCKE",
                  *obyte               = "BYTE",
                  *oconflicts          = "CONFLICTS",
                  *odebug              = "DEBUG",
                  *odefault            = "DEFAULT",
                  *odeferred           = "DEFERRED",
                  *oedit               = "EDIT",
/*
Option no longer used ...
                  *oerrproc3           = "ERROR-PROC",
                  *oerrproc2           = "ERROR_PROC",
                  *oerrproc            = "ERRORPROC",
*/
                  *oerrormaps2         = "ERROR_MAPS",
                  *oerrormaps3         = "ERROR-MAPS",
                  *oerrormaps          = "ERRORMAPS",
                  *oescape             = "ESCAPE",
                  *ofile_prefix2       = "FILE_PREFIX",
                  *ofile_prefix3       = "FILE-PREFIX",
                  *ofile_prefix        = "FILEPREFIX",
                  *ofirst              = "FIRST",
                  *ofixed              = "FIXED",
                  *ofollow             = "FOLLOW",
                  *ofull               = "FULL",
                  *ogenprsr3           = "GENERATE-PARSER",
                  *ogenprsr2           = "GENERATE_PARSER",
                  *ogenprsr            = "GENERATEPARSER",
                  *ogotodefault2       = "GOTO_DEFAULT",
                  *ogotodefault3       = "GOTO-DEFAULT",
                  *ogotodefault        = "GOTODEFAULT",
                  *ohactfile_name      = "HACTFILENAME",
                  *ohactfile_name2     = "HACTFILE-NAME",
                  *ohactfile_name3     = "HACTFILE_NAME",
#if defined(VM) || defined(CW)
                  *ohactfile_type      = "HACTFILETYPE",
                  *ohactfile_type2     = "HACTFILE-TYPE",
                  *ohactfile_type3     = "HACTFILE_TYPE",
                  *ohactfile_mode      = "HACTFILEMODE",
                  *ohactfile_mode2     = "HACTFILE-MODE",
                  *ohactfile_mode3     = "HACTFILE_MODE",
#endif
                  *ohalfword2          = "HALF_WORD",
                  *ohalfword3          = "HALF-WORD",
                  *ohalfword           = "HALFWORD",
                  *ohblockb            = "HBLOCKB",
                  *ohblocke            = "HBLOCKE",
                  *ojikes              = "JIKES",
                  *olalr               = "LALR",
                  *olist               = "LIST",
                  *omax                = "MAXIMUM",
                  *omaximum_distance2  = "MAX_DISTANCE",
                  *omaximum_distance3  = "MAX-DISTANCE",
                  *omaximum_distance   = "MAXDISTANCE",
                  *omin                = "MINIMUM",
                  *ominimum_distance2  = "MIN_DISTANCE",
                  *ominimum_distance3  = "MIN-DISTANCE",
                  *ominimum_distance   = "MINDISTANCE",
                  *onames              = "NAMES",
                  *ontcheck2           = "NT_CHECK",
                  *ontcheck3           = "NT-CHECK",
                  *ontcheck            = "NTCHECK",
                  *ooptimized          = "OPTIMIZED",
                  *oormark             = "ORMARK",
                  *ooutputsize2        = "OUTPUT-SIZE",
                  *ooutputsize3        = "OUTPUT_SIZE",
                  *ooutputsize         = "OUTPUTSIZE",
                  *oprefix             = "PREFIX",
                  *oreadreduce2        = "READ_REDUCE",
                  *oreadreduce3        = "READ-REDUCE",
                  *oreadreduce         = "READREDUCE",
#if defined(VM) || defined(CW)
                  *orecordformat2      = "RECORD_FORMAT",
                  *orecordformat3      = "RECORD-FORMAT",
                  *orecordformat       = "RECORDFORMAT",
#endif
/*
Option no longer used ...
                  *ogettok             = "SCANNER_PROC",
                  *ogettok2            = "SCANNER-PROC",
                  *ogettok3            = "SCANNERPROC",
*/
                  *oscopes             = "SCOPES",
                  *oshiftdefault2      = "SHIFT-DEFAULT",
                  *oshiftdefault3      = "SHIFT_DEFAULT",
                  *oshiftdefault       = "SHIFTDEFAULT",
                  *osingleproductions2 = "SINGLE-PRODUCTIONS",
                  *osingleproductions3 = "SINGLE_PRODUCTIONS",
                  *osingleproductions  = "SINGLEPRODUCTIONS",
                  *oslr                = "SLR",
/*
Option no longer used ...
                  *osmactn2            = "SEMANTIC-PROC",
                  *osmactn3            = "SEMANTIC_PROC",
                  *osmactn             = "SEMANTICPROC",
*/
                  *ospace              = "SPACE",
                  *ostack_size2        = "STACK_SIZE",
                  *ostack_size3        = "STACK-SIZE",
                  *ostack_size         = "STACKSIZE",
                  *ostates             = "STATES",
                  *osuffix             = "SUFFIX",
                  *otable              = "TABLE",
                  *otime               = "TIME",
/*
Option no longer used ...
                  *otkactn3            = "TERMINAL-PROC",
                  *otkactn2            = "TERMINAL_PROC",
                  *otkactn             = "TERMINALPROC",
*/
                  *otrace              = "TRACE",
                  *ovariable           = "VARIABLE",
                  *overbose            = "VERBOSE",
                  *owarnings           = "WARNINGS",
                  *oxref               = "XREF";


/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                        PARSING DECLARATIONS                   **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
/* The following static variables are used only in processing the  */
/* the input source.                                               */
/*******************************************************************/
#define CTL_Z '\x1a'
#undef  min
#define min(x, y) ((x) < (y) ? (x) : (y))

/*******************************************************************/
/*                                                                 */
/*                             IO variables                        */
/*                                                                 */
/* The two character pointer variables, P1 and P2, are used in     */
/* processing the io buffer, INPUT_BUFFER.                         */
/*******************************************************************/
static char *p1,
            *p2,
            *input_buffer;

static char *linestart,
            *bufend,
            *ct_ptr;

static short ct            = 0, /* current token & related variables */
             ct_start_col  = 0,
             ct_end_col    = 0,
             ct_length     = 0;

static long ct_start_line = 0,
            ct_end_line   = 0;

static int num_acts = 0,     /* macro definition & action variables */
           num_defs = 0;

static long defelmt_size = 0, /* macro definition & action vars */
            actelmt_size = 0,
            rulehdr_size = 0;

struct rulehdr_type       /* structure to store rule in first pass */
{
    struct node *rhs_root;
    short        lhs;
    BOOLEAN      sp;
};

struct defelmt_type   /* structure to store location of macro def. */
{
    short                next,
                         length,
                         start_column,
                         end_column;
    char                *macro;
    long                 start_line,
                         end_line;
    char                 name[SYMBOL_SIZE + 1];
};

struct actelmt_type       /* structure to store location of action */
{
    long    start_line,
            end_line;
    short   rule_number,
            start_column,
            end_column;
    BOOLEAN header_block;
};

struct hash_type         /* structure used to hash grammar symbols */
{
    struct hash_type *link;
    short             number,
                      name_index;
    int               st_ptr;
};

struct terminal_type   /* structure used to hold token information */
{
    long  start_line,
          end_line;
    short start_column,
          end_column,
          length,
          kind;
    char  name[SYMBOL_SIZE + 1];
};

static struct rulehdr_type   *rulehdr = NULL;
static struct defelmt_type   *defelmt = NULL;
static struct actelmt_type   *actelmt = NULL;

static struct node           *start_symbol_root;
static struct hash_type      **hash_table;
static struct terminal_type  *terminal;

/******************************************/
/* The following variables hold the names */
/*  of keywords and predefined macros.    */
/******************************************/
static char kdefine[8]             = " define",
            kterminals[11]         = " terminals",
            kalias[7]              = " alias",
            kstart[7]              = " start",
            krules[7]              = " rules",
            knames[7]              = " names",
            kend[5]                = " end",
            krule_text[11]         = " rule_text",
            krule_number[13]       = " rule_number",
            knum_rules[11]         = " num_rules",
            krule_size[11]         = " rule_size",
            knum_terminals[15]     = " num_terminals",
            knum_non_terminals[19] = " num_non_terminals",
            knum_symbols[13]       = " num_symbols",
            kinput_file[12]        = " input_file",
            kcurrent_line[14]      = " current_line",
            knext_line[11]         = " next_line",

     /*****************************************************************/
     /* Note that the next four keywords start with \n instead of     */
     /* the escape character.  This is to prevent the user from       */
     /* declaring a grammar symbol with the same name.  The           */
     /* end-of-line character was chosen since that character can     */
     /* never appear in the input without being interpreted as        */
     /* marking the end of an input line.  When printing such a       */
     /* keyword, the \n is properly replaced by the escape character. */
     /* See RESTORE_SYMBOL in the file LPGUTIL.C.                     */
     /*****************************************************************/
            kempty[7]              = "\nempty",
            kerror[7]              = "\nerror",
            keoft[5]               = "\neof",
            kaccept[5]             = "\nacc",

            kstart_nt[7]           = " start",
            keolt[5]               = " eol";

static struct line_elemt
{
    struct line_elemt *link;
    char line[MAX_LINE_SIZE + 1];
}  *line_pool_root = NULL;

static int blockb_len,
           blocke_len,
           hblockb_len,
           hblocke_len;

static int stack_top = -1;

static void init_process(void);
static void exit_process(void);
static BOOLEAN verify(char *item);
static char *translate(char *str, int len);
static void options(void);
static void process_options_lines(void);
static int hash(char *symbl);
static void insert_string(struct hash_type *q, char *string);
static void assign_symbol_no(char *string_ptr, int image);
static void alias_map(char *stringptr, int image);
static int symbol_image(char *item);
static int name_map(char *symb);
static void process_grammar(void);
static void scanner(void);
static void token_action(void);
static void error_action(void);
static void accept_action(void);
static void build_symno(void);

static struct hash_type *alias_root = NULL;
static short *macro_table;

static void make_rules_map(void);
static void make_names_map(void);
static void process_actions(void);
static void process_action_line(FILE *sysout, char *text,
                                int line_no, int rule_no);
static struct line_elemt *alloc_line(void);
static void free_line(struct line_elemt *p);
static void mapmacro(int def_index);
static struct line_elemt *find_macro(char *name);
static void process_aliases(void);
static void display_input(void);
