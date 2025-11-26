#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

/*******************************************************************/
/* One of the switches below may have to be set prior to building  */
/* JIKES PG. OS2 is for all C compilers running under OS2. DOS is  */
/* for all C compilers running under DOS. Note that to run under   */
/* DOS, the compiler used must support the Huge model (32-bit ptr  */
/* simulation). C370 is for the IBM C compiler running under CMS   */
/* or MVS. CW is for the Waterloo C compiler running under VM/CMS. */
/*                                                                 */
/* This system was built to run on a vanilla Unix or AIX system.   */
/* No switch need to be set for such an environment.  Set other    */
/* switch(es) as needed. Note that the switch C370 should always   */
/* be accompanied by either the switch VM or the switch MVS.       */
/*                                                                 */
/*******************************************************************/
/*
#define DOS
#define OS2
#define MVS
#define CW
#define C370
#define VM
*/

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "c370.h"

/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                         GLOBAL CONSTANTS                      **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
#define PR_HEADING  \
    { \
        fprintf(syslis, "\f\n\n %-39s%s %-30.24s Page %d\n\n",\
                        HEADER_INFO, VERSION, timeptr, ++page_no);\
                        output_line_no = 4;\
    }
#define MAX_PARM_SIZE      22
#define SYMBOL_SIZE        256
#define MAX_MSG_SIZE       (256 + SYMBOL_SIZE)
#define PRINT_LINE_SIZE    80
#define PARSER_LINE_SIZE   80
#define MAX_LINE_SIZE      256

#undef  PAGE_SIZE
#define PAGE_SIZE          55
#define OPTIMIZE_TIME      1
#define OPTIMIZE_SPACE     2
#define MINIMUM_NAMES      1
#define MAXIMUM_NAMES      2
#define OPTIMIZE_PHRASES   3
#define NOTRACE            0
#define TRACE_CONFLICTS    1
#define TRACE_FULL         2
#define STATE_TABLE_UBOUND 1020
#define STATE_TABLE_SIZE   (STATE_TABLE_UBOUND + 1) /* 1021 is a prime */
#define SHIFT_TABLE_UBOUND 400
#define SHIFT_TABLE_SIZE   (SHIFT_TABLE_UBOUND + 1) /* 401 is a prime */
#define SCOPE_UBOUND       100
#define SCOPE_SIZE         (SCOPE_UBOUND + 1)   /* 101 is prime */
#undef  FALSE
#undef  TRUE
#define FALSE              0
#define TRUE               1
#define IS_A_TERMINAL      <= num_terminals
#define IS_A_NON_TERMINAL  > num_terminals

#define SPACE          ' '
#define COMMA          ','
#define INFINITY       ((short) SHRT_MAX)
#define OMEGA          ((short) SHRT_MIN)
#define NIL            ((short) SHRT_MIN + 1)
#define DEFAULT_SYMBOL 0

/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                       ALLOCATE/FREE MACROS                    **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
/* The following macro definitions are used to preprocess calls to */
/* allocate routines that require locations. The FFREE macro is    */
/* normally an invocation to the FREE routine. It is encoded as    */
/* a macro here in case we need to do some debugging on dynamic    */
/* storage.                                                        */
/*******************************************************************/
#define Allocate_node()           allocate_node(hostfile, __LINE__)
#define Allocate_int_array(n)     allocate_int_array(n, hostfile, __LINE__)
#define Allocate_short_array(n)   allocate_short_array(n, hostfile, __LINE__)
#define Allocate_boolean_array(n) allocate_boolean_array(n, hostfile, __LINE__)
#define Allocate_goto_map(n)      allocate_goto_map(n, hostfile, __LINE__)
#define Allocate_shift_map(n)     allocate_shift_map(n, hostfile, __LINE__)
#define Allocate_reduce_map(n)    allocate_reduce_map(n, hostfile, __LINE__)

#define ffree(x) free(x) /* { free(x); x = (void *) ULONG_MAX; } */

/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                          PARSING MACROS                       **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
/* The following macro definitions are used only in processing the */
/* input source.                                                   */
/*******************************************************************/
#define EQUAL_STRING(symb, p) \
        (strcmp((symb), string_table + (p) -> st_ptr) == 0)

#define EXTRACT_STRING(indx) (&string_table[indx])

#define HT_SIZE           701     /* 701 is a prime */
#define RULEHDR_INCREMENT 1024
#define ACTELMT_INCREMENT 1024
#define DEFELMT_INCREMENT 16

#ifdef DOS
#define IOBUFFER_SIZE 8192
#else
#define IOBUFFER_SIZE 65536
#endif

/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                         BIT SET MACROS                        **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
/* The following macros are used to define operations on sets that */
/* are represented as bit-strings.  BOOLEAN_CELL is a type that is */
/* used as the elemental unit used to construct the sets.  For     */
/* example, if BOOLEAN_CELL consists of four bytes and assumming   */
/* that each byte contains 8 bits then the constant SIZEOF_BC      */
/* represents the total number of bits that is contained in each   */
/* elemental unit.                                                 */
/*                                                                 */
/* In general, a parameter called "set" or "set"i, where i is an   */
/* integer, is a pointer to a set or array of sets; a parameter    */
/* called "i" or "j" represents an index in an array of sets; a    */
/* parameter called "b" represents a particular element (or bit)   */
/* within a set.                                                   */
/*                                                                 */
/*******************************************************************/
#define SIZEOF_BC (sizeof(BOOLEAN_CELL) * CHAR_BIT)
#define BC_OFFSET (SIZEOF_BC - 1)

/*******************************************************************/
/* This macro takes as argument an array of bit sets called "set", */
/* an integer "nt" indicating the index of a particular set in the */
/* array and an integer "t" indicating a particular element within */
/* the set. IS_IN_SET check whether ot not the element "t" is in   */
/* the set "set(nt)".                                              */
/*                                                                 */
/* The value (nt*term_set_size) is used to determine the starting  */
/* address of the set element in question.  The value              */
/* (??? / SIZEOF_BC) is used to determine the actual BOOLEAN_CELL  */
/* containing the bit in question.  Finally, the value             */
/* (SIZEOF_BC - (t % SIZEOF_BC)) identifies the actual bit in the  */
/* unit. The bit in question is pushed to the first position and   */
/* and-ed with the value 01. This operation yields the value TRUE  */
/* if the bit is on. Otherwise, the value FALSE is obtained.       */
/* Recall that in C, one cannot shift (left or right) by 0. This   */
/* is why the ? is used here.                                      */
/*******************************************************************/
#define IS_IN_SET(set, i, b)    /* is b in set[i] ? */ \
    ((set)[(i) * term_set_size + (((b) - 1) / SIZEOF_BC)] & \
          (((b) + BC_OFFSET) % SIZEOF_BC ? \
           (BOOLEAN_CELL) 1 << (((b) + BC_OFFSET) % SIZEOF_BC) : \
           (BOOLEAN_CELL) 1))

/*******************************************************************/
/* The macro SET_UNION takes as argument two arrays of sets:       */
/* "set1" and "set2", and two integers "i" and "j" which are       */
/* indices to be used to access particular sets in "set1" and      */
/* "set2", respectively.  SET_UNION computes the union of the two  */
/* sets in question and places the result in the relevant set in   */
/* "set1".                                                         */
/*                                                                 */
/* The remaining macros are either analoguous to IS_IN_SET or      */
/* SET_UNION.                                                      */
/*                                                                 */
/* Note that a macro with the variable "kji" declared in its body  */
/* should not be invoked with a parameter of the same name.        */
/*                                                                 */
/*******************************************************************/
#define SET_UNION(set1, i, set2, j)    /* set[i] union set2[j] */ \
    { \
        register int kji; \
        for (kji = 0; kji < term_set_size; kji++) \
             (set1)[(i) * term_set_size + kji] |= \
                   (set2)[(j) * term_set_size + kji]; \
    }

#define INIT_SET(set)    /* set = {} */ \
    { \
        register int kji; \
        for (kji = 0; kji < term_set_size; kji++) \
             (set)[kji] = 0; \
    }

#define ASSIGN_SET(set1, i, set2, j)    /* set1[i] = set2[j] */ \
    { \
        register int kji; \
        for (kji = 0; kji < term_set_size; kji++) \
             (set1)[(i) * term_set_size + kji] = \
                   (set2)[(j) * term_set_size + kji]; \
    }

#define SET_BIT_IN(set, i, b)    /* set[i] = set[i] with b; */ \
    (set)[(i) * term_set_size + (((b) - 1) / SIZEOF_BC)] |= \
         (((b) + BC_OFFSET) % SIZEOF_BC ? \
          (BOOLEAN_CELL) 1 << (((b) + BC_OFFSET) % SIZEOF_BC) : \
          (BOOLEAN_CELL) 1);

#define RESET_BIT_IN(set, i, b)    /* set[i] = set[i] less b; */ \
    (set)[(i) * term_set_size + (((b) - 1) / SIZEOF_BC)] &= \
         ~(((b) + BC_OFFSET) % SIZEOF_BC ? \
           (BOOLEAN_CELL) 1 << (((b) + BC_OFFSET) % SIZEOF_BC): \
           (BOOLEAN_CELL) 1);

/*******************************************************************/
/* The following macros are analogous to the ones above, except    */
/* that they deal with sets of non-terminals instead of sets of    */
/* terminals.                                                      */
/*******************************************************************/
#define IS_IN_NTSET(set, i, b)    /* is b in set[i] ? */ \
    ((set)[(i) * non_term_set_size + (((b) - 1) / SIZEOF_BC)] & \
          (((b) + BC_OFFSET) % SIZEOF_BC ? \
           (BOOLEAN_CELL) 1 << (((b) + BC_OFFSET) % SIZEOF_BC) : \
           (BOOLEAN_CELL) 1))

#define NTSET_UNION(set1, i, set2, j)    /* set1[i] union set2[j] */ \
    { \
        register int kji; \
        for (kji = 0; kji < non_term_set_size; kji++) \
             (set1)[(i) * non_term_set_size + kji] |= \
                   (set2)[(j) * non_term_set_size + kji]; \
    }

#define INIT_NTSET(set)    /* set = {} */ \
    { \
        register int kji; \
        for (kji = 0; kji < non_term_set_size; kji++) \
             (set)[kji] = 0; \
    }

#define ASSIGN_NTSET(set1, i, set2, j)    /* set1[i] = set2[j] */ \
    { \
        register int kji; \
        for (kji = 0; kji < non_term_set_size; kji++) \
             (set1)[(i) * non_term_set_size + kji] = \
                   (set2)[(j) * non_term_set_size + kji]; \
    }

#define NTSET_BIT_IN(set, i, b)    /* set[i] = set[i] with b; */ \
    (set)[(i) * non_term_set_size + (((b) - 1) / SIZEOF_BC)] |= \
         (((b) + BC_OFFSET) % SIZEOF_BC ? \
          (BOOLEAN_CELL) 1 << (((b) + BC_OFFSET) % SIZEOF_BC) : \
          (BOOLEAN_CELL) 1);

#define NTRESET_BIT_IN(set, i, b)    /* set[i] = set[i] less b; */ \
    (set)[(i) * non_term_set_size + (((b) - 1) / SIZEOF_BC)] &= \
         ~(((b) + BC_OFFSET) % SIZEOF_BC ? \
           (BOOLEAN_CELL) 1 << (((b) + BC_OFFSET) % SIZEOF_BC): \
           (BOOLEAN_CELL) 1);

/*******************************************************************/
/* The following macros are analogous to the ones above, except    */
/* that they deal with sets of states instead of sets of terminals */
/* or non-terminals.                                               */
/*******************************************************************/
#define SET_COLLECTION_BIT(i, b) \
    collection[(i) * state_set_size + (((b) - 1) / SIZEOF_BC)] |= \
         (((b) + BC_OFFSET) % SIZEOF_BC ? \
          (BOOLEAN_CELL) 1 << (((b) + BC_OFFSET) % SIZEOF_BC) : \
          (BOOLEAN_CELL) 1);

#define EMPTY_COLLECTION_SET(i) \
    { \
        register int kji; \
        for (kji = 0; kji < state_set_size; kji++) \
             collection[(i) * state_set_size + kji] = 0; \
    }

/*******************************************************************/
/* The following macros can be used to check, set, or reset a bit  */
/* in a bit-string of any length.                                  */
/*******************************************************************/
#define SET_BIT(set, b) \
    (set)[((b) - 1) / SIZEOF_BC] |= \
         (((b) + BC_OFFSET) % SIZEOF_BC ? \
          (BOOLEAN_CELL) 1 << (((b) + BC_OFFSET) % SIZEOF_BC) : \
          (BOOLEAN_CELL) 1);

#define RESET_BIT(set, b) \
    (set)[((b) - 1) / SIZEOF_BC] &= \
         ~(((b) + BC_OFFSET) % SIZEOF_BC ? \
           (BOOLEAN_CELL) 1 << (((b) + BC_OFFSET) % SIZEOF_BC): \
           (BOOLEAN_CELL) 1);

#define IS_ELEMENT(set, b)    /* is b in set ? */ \
    ((set)[((b) - 1) / SIZEOF_BC] & \
          (((b) + BC_OFFSET) % SIZEOF_BC ? \
           (BOOLEAN_CELL) 1 << (((b) + BC_OFFSET) % SIZEOF_BC) : \
           (BOOLEAN_CELL) 1))

/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                         ITERATION MACROS                      **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
/* The following macros (ALL_) are used to iterate over a sequence.*/
/*******************************************************************/
#define ALL_LA_STATES(indx) \
        (indx = num_states + 1; indx <= (int) max_la_state; indx++)

#define ALL_TERMINALS(indx) \
        (indx = 1; indx <= num_terminals; indx++)
#define ALL_TERMINALS_BACKWARDS(indx) \
        (indx = num_terminals; indx >= 1; indx--)

#define ALL_NON_TERMINALS(indx) \
        (indx = num_terminals + 1; indx <= num_symbols; indx++)
#define ALL_NON_TERMINALS_BACKWARDS(indx) \
        (indx = num_symbols; indx >= num_terminals + 1; indx--)

#define ALL_SYMBOLS(indx) (indx = 1; indx <= num_symbols; indx++)

#define ALL_ITEMS(indx) (indx = 1; indx <= (int) num_items; indx++)

#define ALL_STATES(indx) (indx = 1; indx <= (int) num_states; indx++)

#define ALL_RULES(indx) (indx = 0; indx <= num_rules; indx++)
#define ALL_RULES_BACKWARDS(indx) \
        (indx = num_rules; indx >= 0; indx--)

#define ENTIRE_RHS(indx, rule_no) (indx = rules[rule_no].rhs;\
                                   indx < rules[(rule_no) + 1].rhs;\
                                   indx++)
#define RHS_SIZE(rule_no) (rules[(rule_no) + 1].rhs - rules[rule_no].rhs)
#define RETRIEVE_STRING(indx) (&string_table[symno[indx].ptr])
#define RETRIEVE_NAME(indx) (&string_table[name[indx]])

/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                      MISCELLANEOUS MACROS                     **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
#define TOUPPER(c) (islower(c) ? toupper(c) : c)
#define MAX(a,b)   (((a) > (b)) ? (a) : (b))
#define MIN(a,b)   (((a) < (b)) ? (a) : (b))

#define ABS(x) (((x) < 0) ? -(x) : (x))

#define NOT(item) (! item)

/**********************************************************************/
/* The following two macros check whether or not the value of an      */
/* integer variable exceeds the maximum limit for a short or a long   */
/* integer, respectively. Note that the user should declare the       */
/* variable in question as a long integer. In the case of INT_CHECK,  */
/* this check is meaningful only if INT and SHORT are the same size.  */
/* Otherwise, if INT and LONG are of the same size, as is usually the */
/* case on large systems, this check is meaningless - too late !!!    */
/**********************************************************************/
#define SHORT_CHECK(var) \
    if (var > SHRT_MAX) \
    { \
        PRNTERR("The limit of a short int value" \
                " has been exceeded by " #var); \
        exit(12); \
    }

#define INT_CHECK(var) \
    if (var > INT_MAX) \
    { \
        PRNTERR("The limit of an int value" \
                " has been exceeded by " #var); \
        exit(12); \
    }

#define ENDPAGE_CHECK if (++output_line_no >= PAGE_SIZE) \
                          PR_HEADING

#define PRNT(msg) \
    { \
        printf("%s\n",msg); \
        fprintf(syslis,"%s\n",msg); \
        ENDPAGE_CHECK; \
    }

#define PRNTWNG(msg) \
    { \
        printf("***WARNING: %s\n",msg);\
        fprintf(syslis,"***WARNING: %s\n",msg);\
        ENDPAGE_CHECK; \
    }

#define PRNTERR(msg) \
    { \
        printf("***ERROR: %s\n",msg);\
        fprintf(syslis,"***ERROR: %s\n",msg);\
        ENDPAGE_CHECK; \
    }

/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**     MACROS FOR DEREFERENCING AUTOMATON HEADER STRUCTURES      **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
#define SHIFT_SYMBOL(hdr, indx)   (((hdr).map)[indx].symbol)
#define SHIFT_ACTION(hdr, indx)   (((hdr).map)[indx].action)

#define GOTO_SYMBOL(hdr, indx)    (((hdr).map)[indx].symbol)
#define GOTO_ACTION(hdr, indx)    (((hdr).map)[indx].action)
#define GOTO_LAPTR(hdr, indx)     (((hdr).map)[indx].laptr)

#define REDUCE_SYMBOL(hdr, indx)  (((hdr).map)[indx].symbol)
#define REDUCE_RULE_NO(hdr, indx) (((hdr).map)[indx].rule_number)

/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                         OUTPUT MACROS                         **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
/* The following macro definitions are used only in processing the */
/* output.                                                         */
/*******************************************************************/
#define BUFFER_CHECK(file) \
    if ((IOBUFFER_SIZE - (output_ptr - &output_buffer[0])) < 73) \
    { \
        fwrite(output_buffer, sizeof(char), \
               output_ptr - &output_buffer[0], file); \
        output_ptr = &output_buffer[0]; \
    }

/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                      GLOBAL DECLARATIONS                      **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
typedef unsigned int BOOLEAN_CELL; /* Basic unit used to represent */
                                   /* Bit sets                     */
typedef BOOLEAN_CELL *SET_PTR;

typedef char BOOLEAN;

extern const char HEADER_INFO[];
extern const char VERSION[];
extern const char BLANK[];
extern const long MAX_TABLE_SIZE;

struct node
{
    struct node *next;
    int          value;
};

/*******************************************************************/
/* RULES is the structure that contain the rules of the grammar.   */
/* Every rule of the grammar is mapped into an integer, and given  */
/* rule, and we have access to a value RHS which is the index      */
/* location in the vector RHS where the right-hand-side of the rule*/
/* begins.  The right hand side of a certain rule represented by an*/
/* integer I starts at index location RULES[I].RHS in RHS, and     */
/* ends at index location RULES[I + 1].RHS - 1.  An extra          */
/* NUM_RULES + 1 element is used as a "fence" for the last rule.   */
/* The RHS vector as mentioned above is used to hold a complete    */
/* list of allthe right-hand-side symbols specified in the grammar.*/
/*******************************************************************/
struct ruletab_type
{
    short   lhs,
            rhs;
    BOOLEAN sp;
};

struct shift_type
{
    short symbol,
          action;
};

struct shift_header_type
{
    struct shift_type *map;
    short              size;
};

struct reduce_type
{
    short symbol,
          rule_number;
};

struct reduce_header_type
{
    struct reduce_type *map;
    short              size;
};

struct goto_type
{
    int   laptr;
    short symbol,
          action;
};

struct goto_header_type
{
    struct goto_type *map;
    short             size;
};

struct lastats_type
{
    struct reduce_header_type reduce;
    short                     shift_number,
                              in_state;
};

struct statset_type
{
    struct node             *kernel_items,
                            *complete_items;
    struct goto_header_type  go_to;
    short                    shift_number;
};

extern char *timeptr;

extern long output_line_no;

extern char grm_file[],
            lis_file[],
            act_file[],
            hact_file[],
            tab_file[],
            prs_file[],
            sym_file[],
            def_file[],
            dcl_file[],
            file_prefix[],
            prefix[],
            suffix[],
            parm[],
            msg_line[];

extern FILE *syslis,
            *sysgrm,
            *sysact,
            *syshact,
            *systab,
            *syssym,
            *sysprs,
            *sysdcl,
            *sysprs,
            *sysdef;


/******************************************************/
/*  The variables below are global counters.          */
/******************************************************/
extern long num_items,
            num_states,
            max_la_state;

extern int num_symbols,
           symno_size,  /* NUM_SYMBOLS + 1 */
           num_names,
           num_terminals,
           num_non_terminals,
           num_rules,
           num_conflict_elements,
           num_single_productions,
           gotodom_size;

/******************************************************/
/*  The variables below are used for options setting. */
/******************************************************/
extern BOOLEAN list_bit,
               slr_bit,
               verbose_bit,
               first_bit,
               follow_bit,
               action_bit,
               edit_bit,
               states_bit,
               xref_bit,
               nt_check_bit,
               conflicts_bit,
               read_reduce_bit,
               goto_default_bit,
               shift_default_bit,
               byte_bit,
               warnings_bit,
               single_productions_bit,
               error_maps_bit,
               debug_bit,
               deferred_bit,
               c_bit,
               cpp_bit,
               java_bit,
               jikes_bit, /* undocumented hack for special jikes behavior */
               scopes_bit;

extern int lalr_level,
           default_opt,
           trace_opt,
           table_opt,
           names_opt,
           increment,
           maximum_distance,
           minimum_distance,
           stack_size;

extern char escape,
            ormark,
            record_format;

extern char blockb[],
            blocke[],
            hblockb[],
            hblocke[],
            errmsg[],
            gettok[],
            smactn[],
            tkactn[];

/*********************************************************************/
/*   The variables below are used to hold information about special  */
/* grammar symbols.                                                  */
/*********************************************************************/
extern short accept_image,
             eoft_image,
             eolt_image,
             empty,
             error_image;

                       /* Miscellaneous counters. */

extern int num_first_sets,
           num_shift_maps,
           page_no;

extern long string_offset,
            string_size,
            num_shifts,
            num_shift_reduces,
            num_gotos,
            num_goto_reduces,
            num_reductions,
            num_sr_conflicts,
            num_rr_conflicts,
            num_entries;

extern char *string_table;

extern short *rhs_sym;

extern struct ruletab_type *rules;

/***********************************************************************/
/* CLOSURE is a mapping from non-terminal to a set (linked-list) of    */
/* non-terminals.  The set consists of non-terminals that are          */
/* automatically introduced via closure when the original non-terminal */
/* is introduced.                                                      */
/* CL_ITEMS is a mapping from each non-terminal to a set (linked list) */
/* of items which are the first item of the rules generated by the     */
/* non-terminal in question. ADEQUATE_ITEM is a mapping from each rule */
/* to the last (complete) item produced by that rule.                  */
/* ITEM_TABLE is used to map each item into a number. Given that       */
/* number one can retrieve the rule the item belongs to, the position  */
/* of the dot,  the symbol following the dot, and FIRST of the suffix  */
/* following the "dot symbol".                                         */
/***********************************************************************/
extern struct node **closure,
                   **clitems,
                   **adequate_item;

extern struct itemtab
{
    short symbol,
          rule_number,
          suffix_index,
          dot;
} *item_table;

/***********************************************************************/
/* SYMNO is an array that maps symbol numbers to actual symbols.       */
/***********************************************************************/
extern struct symno_type
{
    int ptr,
        name_index;
} *symno;

/***********************************************************************/
/* These variables hold the number of BOOLEAN_CELLS required to form a */
/* set of terminals, non-terminals and states, respectively.           */
/***********************************************************************/
extern int term_set_size,
           non_term_set_size,
           state_set_size;

/***********************************************************************/
/* NULL_NT is a boolean vector that indicates whether or not a given   */
/* non-terminal is nullable.                                           */
/***********************************************************************/
extern BOOLEAN *null_nt;

/***********************************************************************/
/* FOLLOW is a mapping from non-terminals to a set of terminals that   */
/* may appear immediately after the non-terminal.                      */
/***********************************************************************/
extern SET_PTR nt_first,
               first,
               follow;

/***********************************************************************/
/* NAME is an array containing names to be associated with symbols.    */
/* REDUCE is a mapping from each state to reduce actions in that state.*/
/* SHIFT is an array used to hold the complete set of all shift maps   */
/* needed to construct the state automaton. Though its size is         */
/* NUM_STATES, the actual number of elements used in it is indicated   */
/* by the integer NUM_SHIFT_MAPS. NUM_STATES elements were allocated,  */
/* because if the user requests that certain single productions be     */
/* removed, a Shift map containing actions involving such productions  */
/* cannot be shared.                                                   */
/***********************************************************************/
extern struct shift_header_type  *shift;

extern struct reduce_header_type *reduce;

extern short *gotodef,
             *shiftdf,
             *gd_index,
             *gd_range;

extern int *name;

/***********************************************************************/
/* STATSET is a mapping from state number to state information.        */
/* LASTATS is a similar mapping for look-ahead states.                 */
/* IN_STAT is a mapping from each state to the set of states that have */
/* a transition into the state in question.                            */
/***********************************************************************/
extern struct statset_type *statset;

extern struct lastats_type *lastats;
extern struct node **in_stat;

extern int num_scopes,
           scope_rhs_size,
           scope_state_size,
           num_error_rules;

extern struct scope_type
{
    short prefix,
          suffix,
          lhs_symbol,
          look_ahead,
          state_set;
} *scope;

extern short *scope_right_side,
             *scope_state;

/*******************************************************************/
/*******************************************************************/
/**                                                               **/
/**                        OUTPUT DECLARATIONS                    **/
/**                                                               **/
/*******************************************************************/
/*******************************************************************/
/* The following external variables are used only in processing    */
/* output.                                                         */
/*******************************************************************/
extern char *output_ptr,
            *output_buffer;

extern short *symbol_map,
             *ordered_state,
             *state_list;

extern int *next,
           *previous,
           *state_index;

extern long table_size,
            action_size,
            increment_size;

extern short last_non_terminal,
             last_terminal;

extern int accept_act,
           error_act,
           first_index,
           last_index,
           last_symbol,
           max_name_length;

extern SET_PTR naction_symbols,
               action_symbols;

extern BOOLEAN byte_terminal_range;

#endif /* COMMON_INCLUDED */
