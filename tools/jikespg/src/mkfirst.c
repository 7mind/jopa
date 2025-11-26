static char hostfile[] = __FILE__;

#include <string.h>
#include "common.h"
#include "header.h"

#define LEN (PRINT_LINE_SIZE - 4)
#define NEXT_RULE_SIZE (num_rules + 1)
#define LAST_RHS_INDEX(rule_no) rules[rule_no + 1].rhs - 1

#define INIT_FIRST(nt) \
        { \
            register int k; \
            for (k = 0; k < term_set_size; k++)\
                 first[nt * term_set_size + k] = 0;\
        }

static BOOLEAN is_terminal_rhs(short *rhs_start,
                               BOOLEAN *produces_terminals, int rule_no);
static BOOLEAN is_nullable_rhs(short *rhs_start, int rule_no);
static void compute_first(int nt);
static void print_unreachables(void);
static void print_xref(void);
static void print_nt_first(void);
static void print_follow_map(void);
static short first_map(int root, int tail);
static void s_first(int root, int tail, int set);
static void compute_follow(int nt);
static void quick_sym(short array[], int l, int h);
static void check_non_terminals(void);
static void no_rules_produced(void);
static void nullables_computation(void);
static void compute_closure(int lhs_symbol);
static void compute_produces(int symbol);

static struct f_element_type
{
    short suffix_root,
          suffix_tail,
          link;
} *first_element;

static struct node **direct_produces;
static SET_PTR produces;

/****************************************************************************/
/* TOP, STACK, and INDEX_OF are used for the linear graph algorithm in      */
/* constructing the FIRST, FOLLOW and CLOSURE maps.                         */
/*                                                                          */
/* LHS_RULE and NEXT_RULE are used in constructing a map from non-terminals */
/* to the set of rules produced by the non-terminals.                       */
/*                                                                          */
/* FIRSTis an array used as a hash table to construct                       */
/* the mapping from sequence of symbols to their FIRST terminal             */
/* set.  A sequence is hashed into a location depending on the              */
/* first symbol in that sequence.                                           */
/*                                                                          */
/* FIRST_ITEM_OF is a map from each rule into the first item                */
/* that it generates.                                                       */
/*                                                                          */
/* The following pointers are used to construct a mapping from each symbol  */
/* in the grammar into the set of items denoted by that symbol. I.e.,       */
/*                                                                          */
/*     f(t) := { [A -> x .t y] | A -> x t y is a rule in the grammar }      */
/*                                                                          */
/* Since these sets are simply partitions of the set of items, they are kept*/
/* all in a sequential list in the array NEXT_ITEM.  The roots of the lists */
/* are placed in the arrats T_ITEMS and NT_ITEMS.                           */
/****************************************************************************/
static short *stack,
             *index_of,

             *lhs_rule,
             *next_rule,

             *first_table,
             *first_item_of,

             *next_item,
             *nt_items,
             *nt_list;

static int top;

/*****************************************************************************/
/*                               MKFIRST:                                    */
/*****************************************************************************/
/*    MKFIRST constructs the FIRST and FOLLOW maps, the CLOSURE map,         */
/* ADEQUATE_ITEM and ITEM_TABLE maps and all other basic maps.               */
/*****************************************************************************/
void mkfirst(void)
{
    int symbol,
        nt,
        item_no,
        first_of_empty,
        rule_no,
        i;

    BOOLEAN end_node;

    term_set_size = num_terminals / SIZEOF_BC
                  + (num_terminals % SIZEOF_BC ? 1 : 0);
    non_term_set_size = num_non_terminals / SIZEOF_BC
                      + (num_non_terminals % SIZEOF_BC ? 1 : 0);

    /* allocate various arrays */

    lhs_rule = Allocate_short_array(num_non_terminals);
    lhs_rule -= (num_terminals + 1);

    next_rule = Allocate_short_array(NEXT_RULE_SIZE);
    first_item_of = Allocate_short_array(NEXT_RULE_SIZE);
    stack = Allocate_short_array(num_non_terminals + 1);

    index_of = Allocate_short_array(num_non_terminals);
    index_of -= (num_terminals + 1);

    /*********************************************************************/
    /* NT_FIRST is used to construct a mapping from non-terminals to the */
    /* set of terminals taht may appear first in a string derived from   */
    /* the non-terminal.                                                 */
    /*********************************************************************/
    nt_first = (SET_PTR)
               calloc(num_non_terminals,
                      term_set_size * sizeof(BOOLEAN_CELL));
    if (nt_first == NULL)
        nospace(__FILE__, __LINE__);

    nt_first -= ((num_terminals + 1) * term_set_size);

    next_item = Allocate_short_array(num_items + 1);

    nt_items = Allocate_short_array(num_non_terminals);
    nt_items -= (num_terminals + 1);

    nt_list = Allocate_short_array(num_non_terminals);
    nt_list -= (num_terminals + 1);

    first_element = (struct f_element_type *)
                    calloc(num_items + 1, sizeof(struct f_element_type));
    if (first_element == NULL)
        nospace(__FILE__, __LINE__);

    item_table = (struct itemtab *)
                 calloc(num_items + 1, sizeof(struct itemtab));
    if (item_table == NULL)
        nospace(__FILE__, __LINE__);

    for ALL_NON_TERMINALS(i) /* Initialize LHS_RULE to NIL */
        lhs_rule[i] = NIL;

    /**************************************************************/
    /* In this loop, we construct the LHS_RULE map which maps     */
    /* each non-terminal symbol into the set of rules it produces */
    /**************************************************************/
    for ALL_RULES(rule_no)
    {
        symbol = rules[rule_no].lhs;
        if (lhs_rule[symbol] == NIL)
            next_rule[rule_no] = rule_no;
        else
        {
            next_rule[rule_no] = next_rule[lhs_rule[symbol]];
            next_rule[lhs_rule[symbol]]  = rule_no;
        }
        lhs_rule[symbol] = rule_no;
    }

    /*************************************************************/
    /* Check if there are any non-terminals that do not produce  */
    /* any rules.                                                */
    /*************************************************************/

    no_rules_produced();

    /*************************************************************/
    /* Construct the CLOSURE map of non-terminals.               */
    /*************************************************************/
    closure = (struct node **)
              calloc(num_non_terminals, sizeof(struct node *));
    if (closure == NULL)
        nospace(__FILE__, __LINE__);
    closure -= (num_terminals + 1);

    for ALL_NON_TERMINALS(i)
        index_of[i] = OMEGA;

    top = 0;
    for ALL_NON_TERMINALS(nt)
    {
        if (index_of[nt] == OMEGA)
            compute_closure(nt);
    }

    /*************************************************************/
    /* Construct the NULL_NT map for non-terminals.              */
    /* A non-terminal B is said to be nullable if either:        */
    /*    B -> %empty  or  B -> B1 B2 B3 ... Bk  where Bi is     */
    /*                         nullable for 1 <= i <= k          */
    /*************************************************************/
    null_nt = Allocate_boolean_array(num_non_terminals);
    null_nt -= (num_terminals + 1);

    nullables_computation();

    /*************************************************************/
    /* Construct the FIRST map for non-terminals and also a list */
    /* of non-terminals whose first set is empty.                */
    /*************************************************************/
    for ALL_NON_TERMINALS(i) /* Initialize INDEX_OF to OMEGA */
        index_of[i] = OMEGA;
    top = 0;
    for ALL_NON_TERMINALS(nt)
    {
        if (index_of[nt] == OMEGA)
            compute_first(nt);
    }

    /*************************************************************/
    /*  Since every input source will be followed by the EOFT    */
    /*  symbol, FIRST[accept_image] cannot contain empty but     */
    /*  instead must contain the EOFT symbol.                    */
    /*************************************************************/
    if (null_nt[accept_image])
    {
        null_nt[accept_image] = FALSE;
        RESET_BIT_IN(nt_first, accept_image, empty);
        SET_BIT_IN(nt_first, accept_image, eoft_image);
    }

    /***************************************************************/
    /* Check whether there are any non-terminals that do not       */
    /* generate any terminal strings. If so, signal error and stop.*/
    /***************************************************************/

    check_non_terminals();

    /***************************************************************/
    /* Construct the ITEM_TABLE, FIRST_ITEM_OF, and NT_ITEMS maps. */
    /***************************************************************/

    first_table = Allocate_short_array(num_symbols + 1);

    for ALL_SYMBOLS(i) /* Initialize FIRST_TABLE to NIL */
        first_table[i] = NIL;

    top = 1;
    first_of_empty = top;
    first_element[first_of_empty].suffix_root = 1;
    first_element[first_of_empty].suffix_tail = 0;

    for ALL_NON_TERMINALS(i) /* Initialize NT_ITEMS to NIL */
        nt_items[i] = NIL;

    item_no = 0;
    item_table[item_no].rule_number = 0;
    item_table[item_no].symbol = empty;
    item_table[item_no].dot = 0;
    item_table[item_no].suffix_index = NIL;

    for ALL_RULES(rule_no)
    {
        int j,
            k;

        first_item_of[rule_no] = item_no + 1;
        j = 0;
        k = LAST_RHS_INDEX(rule_no);
        for ENTIRE_RHS(i, rule_no)
        {
            item_no++;
            symbol = rhs_sym[i];
            item_table[item_no].rule_number = rule_no;
            item_table[item_no].symbol = symbol;
            item_table[item_no].dot = j;

            if (lalr_level > 1 ||
                symbol IS_A_NON_TERMINAL ||
                symbol == error_image)
            {
                if (i == k)
                    item_table[item_no].suffix_index = first_of_empty;
                else
                    item_table[item_no].suffix_index = first_map(i + 1, k);
            }
            else
                item_table[item_no].suffix_index = NIL;

            if (symbol IS_A_NON_TERMINAL)
            {
                next_item[item_no] = nt_items[symbol];
                nt_items[symbol] = item_no;
            }
            j++;
        }

        item_table[++item_no].rule_number = rule_no;
        item_table[item_no].symbol = empty;
        item_table[item_no].dot = j;
        item_table[item_no].suffix_index = NIL;
    }


    /***************************************************************/
    /* We now compute the first set for all suffixes that were     */
    /* inserted in the FIRST_TABLE map. There are TOP such suffixes*/
    /* Extra space is also allocated to compute the first set for  */
    /* suffixes whose left-hand side is the ACCEPT non-terminal.   */
    /* The first set for these suffixes are the sets needed to     */
    /* construct the FOLLOW map and compute look-ahead sets.  They */
    /* are placed in the FIRST table in the range 1..NUM_FIRST_SETS*/
    /* The first element in the FIRST table contains the first sets*/
    /* for the empty sequence.                                     */
    /***************************************************************/
    num_first_sets = top;

    for (end_node = ((rule_no = lhs_rule[accept_image]) == NIL);
         ! end_node;
         end_node = (rule_no == lhs_rule[accept_image]))
    {
        rule_no = next_rule[rule_no];
        num_first_sets++;
    }

    first = (SET_PTR)
            calloc(num_first_sets + 1, term_set_size * sizeof(BOOLEAN_CELL));
    if (first == NULL)
        nospace(__FILE__, __LINE__);
    for (i = 1; i <= top; i++)
    {
        s_first(first_element[i].suffix_root,
                first_element[i].suffix_tail, i);
    }

    rule_no = lhs_rule[accept_image];
    for (i = top + 1; i <= num_first_sets; i++)
    {
        rule_no = next_rule[rule_no];
        item_no = first_item_of[rule_no];
        item_table[item_no].suffix_index = i;
        INIT_FIRST(i);
        SET_BIT_IN(first, i, eoft_image);
    }

    /***************************************************************/
    /* If the READ/REDUCE option is on, we precalculate the kernel */
    /* of the final states which simply consists of the last item  */
    /* in  the corresponding rule.  Rules with the ACCEPT          */
    /* non-terminal as their left-hand side are not considered so  */
    /* as to let the Accpet action remain as a Reduce action       */
    /* instead of a Goto/Reduce action.                            */
    /***************************************************************/
    adequate_item = (struct node **)
                    calloc(num_rules + 1, sizeof(struct node *));
    if (adequate_item == NULL)
        nospace(__FILE__, __LINE__);

    if (read_reduce_bit)
    {
        for ALL_RULES(rule_no)
        {
            int j;

            j = RHS_SIZE(rule_no);
            if (rules[rule_no].lhs != accept_image && j > 0)
            {
                struct node *p;

                item_no = first_item_of[rule_no] + j;
                p = Allocate_node();
                p -> value = item_no;
                p -> next = NULL;
                adequate_item[rule_no] = p;
            }
            else
                adequate_item[rule_no] = NULL;
        }
    }


    /***************************************************************/
    /* Construct the CLITEMS map. Each element of CLITEMS points   */
    /* to a circular linked list of items.                         */
    /***************************************************************/
    clitems = (struct node **)
              calloc(num_non_terminals, sizeof(struct node *));
    if (clitems == NULL)
        nospace(__FILE__, __LINE__);
    clitems -= (num_terminals + 1);

    for ALL_NON_TERMINALS(nt)
    {
        clitems[nt] = NULL;

        for (end_node = ((rule_no = lhs_rule[nt]) == NIL);
             ! end_node;
             end_node = (rule_no == lhs_rule[nt]))
        {
            struct node *p;

            rule_no = next_rule[rule_no];
            p = Allocate_node();
            p -> value = first_item_of[rule_no];
            if (clitems[nt] == NULL)
                p -> next = p;
            else
            {
                p -> next = clitems[nt] -> next;
                clitems[nt] -> next = p;
            }
            clitems[nt] = p;
        }
    }

    /***************************************************************/
    /* If LALR_LEVEL > 1, we need to calculate RMPSELF, a set that */
    /* identifies the nonterminals that can right-most produce     */
    /* themselves. In order to compute RMPSELF, the map PRODUCES   */
    /* must be constructed which identifies for each nonterminal   */
    /* the set of nonterminals that it can right-most produce.     */
    /***************************************************************/
    if (lalr_level > 1)
    {
        produces = (SET_PTR)
                   calloc(num_non_terminals,
                          non_term_set_size * sizeof(BOOLEAN_CELL));
        if (produces == NULL)
            nospace(__FILE__, __LINE__);
        produces -= ((num_terminals + 1) * non_term_set_size);

        direct_produces = (struct node **)
                          calloc(num_non_terminals, sizeof(struct node *));
        if (direct_produces == NULL)
            nospace(__FILE__, __LINE__);
        direct_produces -= (num_terminals + 1);

        for ALL_NON_TERMINALS(nt)
        {
            struct node *p,
                        *q;

            for (end_node = ((p = clitems[nt]) == NULL);
                 ! end_node; end_node = (p == clitems[nt]))
            {
                p = p -> next;
                item_no = p -> value;
                symbol = item_table[item_no].symbol;
                if (symbol IS_A_NON_TERMINAL)
                {
                    i = item_table[item_no].suffix_index;
                    if (IS_IN_SET(first, i, empty) &&
                        (! IS_IN_NTSET(produces, nt,
                                       symbol - num_terminals)))
                    {
                        NTSET_BIT_IN(produces, nt,
                                     symbol - num_terminals);
                        q = Allocate_node();
                        q -> value = symbol;
                        q -> next = direct_produces[nt];
                        direct_produces[nt] = q;
                    }
                }
            }
        }

        /************************************************************/
        /* Complete the construction of the RIGHT_MOST_PRODUCES map */
        /* for non-terminals using the digraph algorithm.           */
        /************************************************************/
        for ALL_NON_TERMINALS(nt)
            index_of[nt] = OMEGA;

        top = 0;
        for ALL_NON_TERMINALS(nt)
        {
            if (index_of[nt] == OMEGA)
                compute_produces(nt);
        }

        init_rmpself(produces);

        produces += ((num_terminals + 1) * non_term_set_size);
        ffree(produces);
        direct_produces += (num_terminals + 1);
        ffree(direct_produces);
    }

    /***************************************************************/
    /* Construct the FOLLOW map if                                 */
    /*   1) an SLR table is requested                              */
    /*   2) if we have to print the FOLLOW map                     */
    /*   3) Error-maps are requested                               */
    /*   4) There are more than one starting symbol.               */
    /***************************************************************/
    if (slr_bit || follow_bit || error_maps_bit ||
        next_rule[lhs_rule[accept_image]] != lhs_rule[accept_image])
    {
        follow = (SET_PTR)
                 calloc(num_non_terminals,
                        term_set_size * sizeof(BOOLEAN_CELL));
        if (follow == NULL)
            nospace(__FILE__, __LINE__);
        follow -= ((num_terminals + 1) * term_set_size);

        SET_BIT_IN(follow, accept_image, eoft_image);

        for ALL_NON_TERMINALS(i) /* Initialize INDEX_OF to OMEGA */
            index_of[i] = OMEGA;

        index_of[accept_image] = INFINITY;  /* mark computed */
        top = 0;

        for ALL_NON_TERMINALS(nt)
        {
            if (index_of[nt] == OMEGA) /* not yet computed ? */
                compute_follow(nt);
        }

    /***************************************************************/
    /*  Initialize FIRST for suffixes that can follow each starting*/
    /* non-terminal ( except the main symbol) with the FOLLOW set */
    /* of the non-terminal in question.                            */
    /***************************************************************/
        rule_no = lhs_rule[accept_image];
        if (next_rule[rule_no] != rule_no)
        {
            rule_no = next_rule[rule_no];   /* first rule */
            top = item_table[first_item_of[rule_no]].suffix_index;
            for (i = top + 1; i <= num_first_sets; i++)
            {
                rule_no = next_rule[rule_no];
                item_no = first_item_of[rule_no];
                symbol = item_table[item_no].symbol;
                if (symbol IS_A_NON_TERMINAL)
                {
                   ASSIGN_SET(first, i, follow, symbol);
                }
            }
        }
    }

    /***************************************************************/
    /* If WARNINGS option is turned on, the unreachable symbols in */
    /* the grammar are printed.                                    */
    /***************************************************************/
    if (warnings_bit)
        print_unreachables();

    /***************************************************************/
    /* If a Cross_Reference listing is requested, it is generated  */
    /* here.                                                       */
    /***************************************************************/
    if (xref_bit)
        print_xref();

    /***************************************************************/
    /* If a listing of the FIRST map is requested, it is generated */
    /* here.                                                       */
    /***************************************************************/
    if (first_bit)
        print_nt_first();

    /****************************************************************/
    /* If a listing of the FOLLOW map is requested, it is generated */
    /* here.                                                        */
    /***************************************************************/
    if (follow_bit)
        print_follow_map();

    /***************************************************************/
    /* Free allocated arrays.                                      */
    /***************************************************************/
    nt_first += ((num_terminals + 1) * term_set_size);
    ffree(nt_first);
    nt_list += (num_terminals + 1);
    ffree(nt_list);
    ffree(first_table);
    ffree(first_element);
    nt_items += (num_terminals + 1);
    ffree(nt_items);
    ffree(next_item);
    ffree(stack);
    index_of += (num_terminals + 1);
    ffree(index_of);
    lhs_rule += (num_terminals + 1);
    ffree(lhs_rule);
    ffree(next_rule);
    ffree(first_item_of);

    return;
}


/*****************************************************************************/
/*                           NO_RULES_PRODUCED:                              */
/*****************************************************************************/
static void no_rules_produced(void)
{
    char line[PRINT_LINE_SIZE + 1],
         tok[SYMBOL_SIZE + 1];

    int nt_root,
        nt_last,
        symbol;

    /*************************************************************/
    /* Build a list of all non-terminals that do not produce any */
    /* rules.                                                    */
    /*************************************************************/
    nt_root = NIL;
    for ALL_NON_TERMINALS(symbol)
    {
        if (lhs_rule[symbol] == NIL)
        {
            if (nt_root == NIL)
                nt_root = symbol;
            else
                nt_list[nt_last] = symbol;
            nt_last = symbol;
        }
    }

    /*************************************************************/
    /* If the list of non-terminals that do not produce any rules*/
    /* is not empty, signal error and stop.                      */
    /*************************************************************/

    if (nt_root != NIL)
    {
        PR_HEADING;
        nt_list[nt_last] = NIL;
        if (nt_list[nt_root] == NIL)
        {
            PRNTERR("The following Non-terminal does not produce any rules: ");
        }
        else
        {
            PRNTERR("The following Non-terminals do not produce any rules: ");
        }
        strcpy(line, "        ");

        for (symbol = nt_root; symbol != NIL; symbol = nt_list[symbol])
        {
            restore_symbol(tok, RETRIEVE_STRING(symbol));
            if (strlen(line) + strlen(tok) > PRINT_LINE_SIZE)
            {
                PRNT(line);
                print_large_token(line, tok, "    ", LEN);
            }
            else
                strcat(line, tok);
            strcat(line,BLANK);
        }
        PRNT(line);
        exit(12);
    }
}


/*****************************************************************************/
/*                            COMPUTE_CLOSURE:                               */
/*****************************************************************************/
/*  This function computes the closure of a non-terminal LHS_SYMBOL passed   */
/* to it as an argument using the digraph algorithm.                         */
/*  The closure of a non-terminal A is the set of all non-terminals Bi that  */
/* can directly or indirectly start a string generated by A.                 */
/* I.e., A *::= Bi X where X is an arbitrary string.                         */
/*****************************************************************************/
static void compute_closure(int lhs_symbol)
{
    int indx;
    short *nont_list;

    int symbol,
        rule_no,
        nt_root,
        i;

    struct node *p,
                *q;

    BOOLEAN end_node;

    nont_list = Allocate_short_array(num_non_terminals);
    nont_list -= (num_terminals + 1); /* Temporary direct        */
                                      /* access set for closure. */
    stack[++top] = lhs_symbol;
    indx = top;
    index_of[lhs_symbol] = indx;

    for ALL_NON_TERMINALS(i)
        nont_list[i] = OMEGA;

    nont_list[lhs_symbol] = NIL;
    nt_root = lhs_symbol;

    closure[lhs_symbol] = NULL; /* Permanent closure set. Linked list */

    for (end_node = ((rule_no = lhs_rule[lhs_symbol]) == NIL);
         ! end_node;          /* Iterate over all rules of LHS_SYMBOL */
         end_node = (rule_no == lhs_rule[lhs_symbol]))
    {
        rule_no = next_rule[rule_no];
        symbol = (RHS_SIZE(rule_no) == 0 ? empty
                                         : rhs_sym[rules[rule_no].rhs]);

        if (symbol IS_A_NON_TERMINAL)
        {
            if (nont_list[symbol] == OMEGA)
            {
                if (index_of[symbol] == OMEGA) /* if first time seen */
                    compute_closure(symbol);

                index_of[lhs_symbol] = MIN(index_of[lhs_symbol],
                                           index_of[symbol]);
                nont_list[symbol] = nt_root;
                nt_root = symbol;

                /* add closure[symbol] to closure of LHS_SYMBOL.  */

                for (end_node = ((q = closure[symbol]) == NULL);
                     ! end_node;
                     end_node = (q == closure[symbol]))
                {
                    q = q -> next;
                    if (nont_list[q -> value] == OMEGA)
                    {
                        nont_list[q -> value] = nt_root;
                        nt_root = q -> value;
                    }
                }
            }
        }
    }

    for (; nt_root != lhs_symbol; nt_root = nont_list[nt_root])
    {
        p = Allocate_node();
        p -> value = nt_root;
        if (closure[lhs_symbol] == NULL)
            p -> next = p;
        else
        {
            p -> next = closure[lhs_symbol] -> next;
            closure[lhs_symbol] -> next = p;
        }
        closure[lhs_symbol] = p;
    }

    if (index_of[lhs_symbol] == indx)
    {
        for (symbol = stack[top]; symbol != lhs_symbol; symbol = stack[--top])
        {
            q = closure[symbol];
            if (q != NULL)
            {
                p = q -> next;
                free_nodes(p, q); /* free nodes used by CLOSURE[SYMBOL] */
                closure[symbol] = NULL;
            }

            p = Allocate_node();
            p -> value = lhs_symbol;
            p -> next = p;
            closure[symbol] = p;

            for (end_node = ((q = closure[lhs_symbol]) == NULL);
                 ! end_node;
                 end_node = (q == closure[lhs_symbol]))
            {
                q = q -> next;
                if (q -> value != symbol)
                {
                    p = Allocate_node();
                    p -> value = q -> value;
                    p -> next = closure[symbol] -> next;
                    closure[symbol] -> next = p;
                    closure[symbol] = p;
                }
            }

            index_of[symbol] = INFINITY;
        }

        index_of[lhs_symbol] = INFINITY;
        top--;
    }

    nont_list += (num_terminals + 1);
    ffree(nont_list);

    return;
}


/*****************************************************************************/
/*                           NULLABLES_COMPUTATION:                          */
/*****************************************************************************/
/*   This procedure computes the set of non-terminal symbols that can        */
/* generate the empty string.  Such non-terminals are said to be nullable.   */
/*                                                                           */
/* A non-terminal "A" can generate empty if the grammar in question contains */
/* a rule:                                                                   */
/*          A ::= B1 B2 ... Bn     n >= 0,  1 <= i <= n                      */
/* and Bi, for all i, is a nullable non-terminal.                            */
/*****************************************************************************/
static void nullables_computation(void)
{
    short *rhs_start;

    int rule_no,
        nt;

    BOOLEAN changed = TRUE,
            end_node;

    rhs_start = Allocate_short_array(NEXT_RULE_SIZE);

    /******************************************************************/
    /* First, mark all non-terminals as non-nullable.  Then initialize*/
    /* RHS_START. RHS_START is a mapping from each rule in the grammar*/
    /* into the next symbol in its right-hand side that has not yet   */
    /* proven to be nullable.                                         */
    /******************************************************************/
    for ALL_NON_TERMINALS(nt)
        null_nt[nt] = FALSE;

    for ALL_RULES(rule_no)
        rhs_start[rule_no] = rules[rule_no].rhs;

    /******************************************************************/
    /* We now iterate over the rules and try to advance the RHS_START */
    /* pointer thru each right-hand side as far as we can.  If one or */
    /* more non-terminals are found to be nullable, they are marked   */
    /* as such and the process is repeated.                           */
    /*                                                                */
    /* If we go through all the rules and no new non-terminal is found*/
    /* to be nullable then we stop and return.                        */
    /*                                                                */
    /* Note that for each iteration, only rules associated with       */
    /* non-terminals that are non-nullable are considered.  Further,  */
    /* as soon as a non-terminal is found to be nullable, the         */
    /* remaining rules associated with it are not considered.  I.e.,  */
    /* we quit the inner loop.                                        */
    /******************************************************************/
    while(changed)
    {
        changed = FALSE;

        for ALL_NON_TERMINALS(nt)
        {
            for (end_node = ((rule_no = lhs_rule[nt]) == NIL);
                 ! null_nt[nt] && ! end_node;
                 end_node = (rule_no == lhs_rule[nt]))
            {
                rule_no = next_rule[rule_no];
                if (is_nullable_rhs(rhs_start,rule_no))
                {
                    changed = TRUE;
                    null_nt[nt] = TRUE;
                }
            }
        }
    }

    ffree(rhs_start);

    return;
}


/*****************************************************************************/
/*                            IS_NULLABLE_RHS:                               */
/*****************************************************************************/
/*   This procedure tries to advance the RHS_START pointer.  If the current  */
/* symbol identified by the RHS_START element is a terminal it returns FALSE */
/* to indicate that it cannot go any further.  If it encounters a  non-null- */
/* lable non-terminal, it also returns FALSE. Otherwise, the whole right-hand*/
/* side is consumed, and it returns the value TRUE.                          */
/*****************************************************************************/
static BOOLEAN is_nullable_rhs(short *rhs_start, int rule_no)
{
    int symbol;

    for(rhs_start[rule_no] = rhs_start[rule_no];
        rhs_start[rule_no] <= rules[rule_no + 1].rhs - 1;
        rhs_start[rule_no]++)
    {
        symbol = rhs_sym[rhs_start[rule_no]];
        if (symbol IS_A_TERMINAL)
            return(FALSE);
        else if (! null_nt[symbol]) /* symbol is a non-terminal */
            return(FALSE);
    }

    return(TRUE);
}


/*****************************************************************************/
/*                             COMPUTE_FIRST:                                */
/*****************************************************************************/
/* This subroutine computes FIRST(NT) for some non-terminal NT using the     */
/* digraph algorithm.                                                        */
/* FIRST(NT) is the set of all terminals Ti that may start a string generated*/
/* by NT. That is, NT *::= Ti X where X is an arbitrary string.              */
/*****************************************************************************/
static void compute_first(int nt)
{
    int indx;

    BOOLEAN end_node,
            blocked;

    int i,
        symbol,
        rule_no;

    SET_PTR temp_set;

    temp_set = (SET_PTR)
               calloc(1, term_set_size * sizeof(BOOLEAN_CELL));
    if (temp_set == NULL)
        nospace(__FILE__, __LINE__);

    stack[++top] = nt;
    indx = top;
    index_of[nt] = indx;

    /**************************************************************/
    /* Iterate over all rules generated by non-terminal NT...     */
    /* In this application of the transitive closure algorithm,   */
    /*                                                            */
    /*  G(A) := { t | A ::= t X for a terminal t and a string X } */
    /*                                                            */
    /* The relation R is defined as follows:                      */
    /*                                                            */
    /*    R(A, B) iff A ::= B1 B2 ... Bk B X                      */
    /*                                                            */
    /* where Bi is nullable for 1 <= i <= k                       */
    /**************************************************************/

    for (end_node = ((rule_no = lhs_rule[nt]) == NIL);
         ! end_node;    /* Iterate over all rules produced by NT */
         end_node = (rule_no == lhs_rule[nt]))
    {
        rule_no = next_rule[rule_no];
        blocked = FALSE;

        for ENTIRE_RHS(i, rule_no)
        {
            symbol = rhs_sym[i];
            if (symbol IS_A_NON_TERMINAL)
            {
                if (index_of[symbol] == OMEGA)
                    compute_first(symbol);
                index_of[nt] = MIN( index_of[nt], index_of[symbol]);

                ASSIGN_SET(temp_set, 0, nt_first, symbol);
                RESET_BIT(temp_set, empty);
                SET_UNION(nt_first, nt, temp_set, 0);
                blocked = NOT(null_nt[symbol]);
            }
            else
            {
                SET_BIT_IN(nt_first, nt, symbol);
                blocked = TRUE;
            }

            if (blocked)
                break;
        }

        if (! blocked)
        {
            SET_BIT_IN(nt_first, nt, empty);
        }
    }

    if (index_of[nt] == indx)
    {
        for (symbol = stack[top]; symbol != nt; symbol = stack[--top])
        {
            ASSIGN_SET(nt_first, symbol, nt_first, nt);
            index_of[symbol] = INFINITY;
        }
        index_of[nt] = INFINITY;
        top--;
    }
    ffree(temp_set);
    return;
}


/*****************************************************************************/
/*                            CHECK_NON_TERMINALS:                           */
/*****************************************************************************/
/* This procedure checks whether or not any non-terminal symbols can fail to */
/* generate a string of terminals.                                           */
/*                                                                           */
/* A non-terminal "A" can generate a terminal string if the grammar in       */
/* question contains a rule of the form:                                     */
/*                                                                           */
/*         A ::= X1 X2 ... Xn           n >= 0,  1 <= i <= n                 */
/*                                                                           */
/* and Xi, for all i, is a terminal or a non-terminal that can generate a    */
/* string of terminals.                                                      */
/* This routine is structurally identical to COMPUTE_NULLABLES.              */
/*****************************************************************************/
static void check_non_terminals(void)
{
    char line[PRINT_LINE_SIZE + 1],
         tok[SYMBOL_SIZE + 1];

    short *rhs_start;

    int rule_no,
        nt_root,
        nt_last,
        symbol,
        nt;

    BOOLEAN changed = TRUE,
            end_node,
            *produces_terminals;

    rhs_start = Allocate_short_array(NEXT_RULE_SIZE);
    produces_terminals = Allocate_boolean_array(num_non_terminals);
    produces_terminals -= (num_terminals + 1);

    /******************************************************************/
    /* First, mark all non-terminals as not producing terminals. Then */
    /* initialize RHS_START. RHS_START is a mapping from each rule in */
    /* the grammar into the next symbol in its right-hand side that   */
    /* has not yet proven to be a symbol that generates terminals.    */
    /******************************************************************/
    for ALL_NON_TERMINALS(nt)
        produces_terminals[nt] = FALSE;

    produces_terminals[accept_image] = TRUE;

    for ALL_RULES(rule_no)
        rhs_start[rule_no] = rules[rule_no].rhs;

    /******************************************************************/
    /* We now iterate over the rules and try to advance the RHS_START */
    /* pointer to each right-hand side as far as we can.  If one or   */
    /* more non-terminals are found to be "all right", they are       */
    /* marked as such and the process is repeated.                    */
    /*                                                                */
    /* If we go through all the rules and no new non-terminal is      */
    /* found to be "all right" then we stop and return.               */
    /*                                                                */
    /* Note that on each iteration, only rules associated with        */
    /* non-terminals that are not "all right" are considered. Further,*/
    /* as soon as a non-terminal is found to be "all right", the      */
    /* remaining rules associated with it are not considered. I.e.,   */
    /* we quit the inner loop.                                        */
    /******************************************************************/
    while(changed)
    {
        changed = FALSE;

        for ALL_NON_TERMINALS(nt)
        {
            for (end_node = ((rule_no = lhs_rule[nt]) == NIL);
                 (! produces_terminals[nt]) && (! end_node);
                 end_node = (rule_no == lhs_rule[nt]))
            {
                rule_no = next_rule[rule_no];
                if (is_terminal_rhs(rhs_start, produces_terminals, rule_no))
                {
                    changed = TRUE;
                    produces_terminals[nt] = TRUE;
                }
            }
        }
    }

    /*************************************************************/
    /* Construct a list of all non-terminals that do not generate*/
    /* terminal strings.                                         */
    /*************************************************************/
    nt_root = NIL;
    for ALL_NON_TERMINALS(nt)
    {
        if (! produces_terminals[nt])
        {
            if (nt_root == NIL)
                nt_root = nt;
            else
                nt_list[nt_last] = nt;
            nt_last = nt;
        }
    }

    /*************************************************************/
    /* If there are non-terminal symbols that do not generate    */
    /* terminal strings, print them out and stop the program.    */
    /*************************************************************/
    if (nt_root != NIL)
    {
        nt_list[nt_last] = NIL;  /* mark end of list */
        PR_HEADING;
        strcpy(line, "*** ERROR: The following Non-terminal");
        if (nt_list[nt_root] == NIL)
            strcat(line, " does not generate any terminal strings: ");
        else
        {
            strcat(line, "s do not generate any terminal strings: ");
            PRNT(line);
            strcpy(line, "        "); /* 8 spaces */
        }

        for (symbol = nt_root; symbol != NIL; symbol = nt_list[symbol])
        {
            restore_symbol(tok, RETRIEVE_STRING(symbol));
            if (strlen(line) + strlen(tok) > PRINT_LINE_SIZE-1)
            {
                PRNT(line);
                print_large_token(line, tok, "    ", LEN);
            }
            else
                strcat(line, tok);
            strcat(line, BLANK);
        }
        PRNT(line);
        exit(12);
    }
    produces_terminals += (num_terminals + 1);
    ffree(produces_terminals);
    ffree(rhs_start);
}


/*****************************************************************************/
/*                          IS_TERMINAL_RHS:                                 */
/*****************************************************************************/
/*   This procedure tries to advance the RHS_START pointer.  If the current  */
/* symbol identified by the RHS_START element is a bad non-terminal it       */
/* returns FALSE.  Otherwise, the whole right-hand side is traversed, and it */
/* returns the value TRUE.                                                   */
/*****************************************************************************/
static BOOLEAN is_terminal_rhs(short *rhs_start,
                               BOOLEAN *produces_terminals, int rule_no)
{
    int symbol;

    for(rhs_start[rule_no] = rhs_start[rule_no];
        rhs_start[rule_no] <= rules[rule_no + 1].rhs - 1;
        rhs_start[rule_no]++)
    {
        symbol = rhs_sym[rhs_start[rule_no]];
        if (symbol IS_A_NON_TERMINAL)
        {
            if (! produces_terminals[symbol])
                return(FALSE);
        }
    }

    return(TRUE);
}


/*****************************************************************************/
/*                             FIRST_MAP:                                    */
/*****************************************************************************/
/*  FIRST_MAP takes as arguments two pointers, ROOT and TAIL, to a sequence  */
/* of symbols in RHS which it inserts in FIRST_TABLE.  The vector FIRST_TABLE*/
/* is used as the base for a hashed table where collisions are resolved by   */
/* links.  Elements added to this hash table are allocated from the vector   */
/* FIRST_ELEMENT, with the variable TOP always indicating the position of the*/
/* last element in FIRST_ELEMENT that was allocated.                         */
/* NOTE: The suffix indentified by ROOT and TAIL is presumed not to be empty.*/
/*       That is, ROOT <= TAIL !!!                                           */
/*****************************************************************************/
static short first_map(int root, int tail)
{
    int i,
        j,
        k;

    for (i = first_table[rhs_sym[root]]; i != NIL; i = first_element[i].link)
    {
        for (j = root + 1,
             k = first_element[i].suffix_root + 1;
             (j <= tail && k <= first_element[i].suffix_tail);
             j++, k++)
        {
            if (rhs_sym[j] != rhs_sym[k])
                break;
        }
        if (j > tail && k > first_element[i].suffix_tail)
            return((short) i);
    }
    top++;
    first_element[top].suffix_root = root;
    first_element[top].suffix_tail = tail;
    first_element[top].link = first_table[rhs_sym[root]];
    first_table[rhs_sym[root]] = top;

    return(top);
}


/*****************************************************************************/
/*                              S_FIRST:                                     */
/*****************************************************************************/
/* S_FIRST takes as argument, two pointers: ROOT and TAIL to a sequence of   */
/* symbols in the vector RHS, and INDEX which is the index of a first set.   */
/* It computes the set of all terminals that can appear as the first symbol  */
/* in the sequence and places the result in the FIRST set indexable by INDEX.*/
/*****************************************************************************/
static void s_first(int root, int tail, int index)
{
    int i,
        symbol;

    symbol = (root > tail ? empty : rhs_sym[root]);

    if (symbol IS_A_TERMINAL)
    {
        INIT_FIRST(index);
        SET_BIT_IN(first, index, symbol); /* add it to set */
    }
    else
    {
        ASSIGN_SET(first, index, nt_first, symbol);
    }

    for (i = root + 1; i <= tail && IS_IN_SET(first, index, empty); i++)
    {
        symbol = rhs_sym[i];
        RESET_BIT_IN(first, index, empty);    /* remove EMPTY */
        if (symbol IS_A_TERMINAL)
        {
            SET_BIT_IN(first, index, symbol);  /* add it to set */
        }
        else
        {
            SET_UNION(first, index, nt_first, symbol);
        }
    }

    return;
}


/******************************************************************/
/*                     COMPUTE_PRODUCES:                          */
/******************************************************************/
/* For a given symbol, complete the computation of                */
/* PRODUCES[symbol].                                              */
/******************************************************************/
static void compute_produces(int symbol)
{
    int indx;

    int new_symbol;

    struct node *p,
                *q;

    stack[++top] = symbol;
    indx = top;
    index_of[symbol] = indx;

    for (p = direct_produces[symbol]; p != NULL; q = p, p = p -> next)
    {
        new_symbol = p -> value;
        if (index_of[new_symbol] == OMEGA)  /* first time seen? */
            compute_produces(new_symbol);
        index_of[symbol] = MIN(index_of[symbol], index_of[new_symbol]);
        NTSET_UNION(produces, symbol, produces, new_symbol);
    }
    if (direct_produces[symbol] != NULL)
        free_nodes(direct_produces[symbol], q);

    if (index_of[symbol] == indx)  /*symbol is SCC root */
    {
        for (new_symbol = stack[top];
             new_symbol != symbol; new_symbol = stack[--top])
        {
            ASSIGN_NTSET(produces, new_symbol, produces, symbol);
            index_of[new_symbol] = INFINITY;
        }

        index_of[symbol] = INFINITY;
        top--;
    }

    return;
}


/*****************************************************************************/
/*                          COMPUTE_FOLLOW:                                  */
/*****************************************************************************/
/* COMPUTE_FOLLOW computes FOLLOW[nt] for some non-terminal NT using the     */
/* digraph algorithm.  FOLLOW[NT] is the set of all terminals Ti that        */
/* may immediately follow a string X generated by NT. I.e., if NT *::= X     */
/* then X Ti is a valid substring of a class of strings that may be          */
/* recognized by the language.                                               */
/*****************************************************************************/
static void compute_follow(int nt)
{
    int indx;

    int rule_no,
        lhs_symbol,
        item_no;

    SET_PTR temp_set;

    temp_set = (SET_PTR)
               calloc(1, term_set_size * sizeof(BOOLEAN_CELL));
    if (temp_set == NULL)
        nospace(__FILE__, __LINE__);

    /**************************************************************/
    /* FOLLOW[NT] was initialized to 0 for all non-terminals.     */
    /**************************************************************/

    stack[++top] = nt;
    indx = top;
    index_of[nt] = indx;

    for (item_no = nt_items[nt]; item_no != NIL; item_no = next_item[item_no])
    { /* iterate over all items of NT */
        ASSIGN_SET(temp_set, 0, first, item_table[item_no].suffix_index);
        if (IS_ELEMENT(temp_set, empty))
        {
            RESET_BIT(temp_set, empty);
            rule_no = item_table[item_no].rule_number;
            lhs_symbol = rules[rule_no].lhs;
            if (index_of[lhs_symbol] == OMEGA)
                compute_follow(lhs_symbol);
            SET_UNION( follow, nt, follow, lhs_symbol);
            index_of[nt] = MIN( index_of[nt], index_of[lhs_symbol]);
        }
        SET_UNION(follow, nt, temp_set, 0);
    }

    if (index_of[nt] == indx)
    {
        for (lhs_symbol = stack[top];
             lhs_symbol != nt; lhs_symbol = stack[--top])
        {
            ASSIGN_SET(follow, lhs_symbol, follow, nt);
            index_of[lhs_symbol] = INFINITY;
        }
        index_of[nt] = INFINITY;
        top--;
    }
    ffree(temp_set);
    return;
}


/*****************************************************************************/
/*                           PRINT_UNREACHABLES:                             */
/*****************************************************************************/
static void print_unreachables(void)
{
    short *symbol_list;

    int nt,
        t_root,
        nt_root,
        rule_no,
        symbol,
        i;

    BOOLEAN end_node;

    char line[PRINT_LINE_SIZE + 1],
         tok[SYMBOL_SIZE + 1];

    /***************************************************************/
    /* SYMBOL_LIST is used for two purposes:                       */
    /*  1) to mark symbols that are reachable from the Accepting   */
    /*        non-terminal.                                        */
    /*  2) to construct lists of symbols that are not reachable.   */
    /***************************************************************/

    symbol_list = Allocate_short_array(num_symbols + 1);
    for ALL_SYMBOLS(i)
        symbol_list[i] = OMEGA;
    symbol_list[eoft_image] = NIL;
    symbol_list[empty] = NIL;
    if (error_maps_bit)
        symbol_list[error_image] = NIL;

    /*********************************************************************/
    /* Initialize a list consisting only of the Accept non-terminal.     */
    /* This list is a work pile of non-terminals to process as follows:  */
    /* Each non-terminal in the front of the list is removed in turn and */
    /* 1) All terminal symbols in one of its right-hand sides are        */
    /*     marked reachable.                                             */
    /* 2) All non-terminals in one of its right-hand sides are placed    */
    /*     in the the work pile of it had not been processed previously  */
    /*********************************************************************/
    nt_root = accept_image;
    symbol_list[nt_root] = NIL;

    for (nt = nt_root; nt != NIL; nt = nt_root)
    {
        nt_root = symbol_list[nt];

        for (end_node = ((rule_no = lhs_rule[nt]) == NIL);
             ! end_node;
             end_node = (rule_no == lhs_rule[nt]))
        {
            rule_no = next_rule[rule_no];
            for ENTIRE_RHS(i, rule_no)
            {
                symbol = rhs_sym[i];
                if (symbol IS_A_TERMINAL)
                    symbol_list[symbol] = NIL;
                else if (symbol_list[symbol] == OMEGA)
                {
                    symbol_list[symbol] = nt_root;
                    nt_root = symbol;
                }
            }
        }
    }

    /***************************************************************/
    /* We now iterate (backwards to keep things in order) over the */
    /* terminal symbols, and place each unreachable terminal in a  */
    /* list. If the list is not empty, we signal that these symbols*/
    /* are unused.                                                 */
    /***************************************************************/
    t_root = NIL;
    for ALL_TERMINALS_BACKWARDS(symbol)
    {
        if (symbol_list[symbol] == OMEGA)
        {
            symbol_list[symbol] = t_root;
            t_root = symbol;
        }
    }

    if (t_root != NIL)
    {
        PR_HEADING;
        if (symbol_list[t_root] != NIL)
        {
            PRNT("*** The following Terminals are useless: ");
            fprintf(syslis, "\n\n");
            strcpy(line, "        "); /* 8 spaces */
        }
        else
            strcpy(line, "*** The following Terminal is useless: ");

        for (symbol = t_root; symbol != NIL; symbol = symbol_list[symbol])
        {
            restore_symbol(tok, RETRIEVE_STRING(symbol));
            if (strlen(line) + strlen(tok) > PRINT_LINE_SIZE)
            {
                PRNT(line);
                print_large_token(line, tok, "    ", LEN);
            }
            else
            {
                strcat(line, tok);
                strcat(line, BLANK);
             }
             strcat(line, BLANK);
        }
        PRNT(line);
    }


    /***************************************************************/
    /* We now iterate (backward to keep things in order) over the  */
    /* non-terminals, and place each unreachable non-terminal in a */
    /* list.  If the list is not empty, we signal that these       */
    /* symbols are unused.                                         */
    /***************************************************************/
    nt_root = NIL;
    for ALL_NON_TERMINALS_BACKWARDS(symbol)
    {
        if (symbol_list[symbol] == OMEGA)
        {
            symbol_list[symbol] = nt_root;
            nt_root = symbol;
        }
    }

    if (nt_root != NIL)
    {
        PR_HEADING;
        if (symbol_list[nt_root] != NIL)
        {
            PRNT("*** The following Non-Terminals are useless: ");
            fprintf(syslis, "\n\n");
            strcpy(line, "        "); /* 8 spaces */
        }
        else
            strcpy(line, "*** The following Non-Terminal is useless: ");

        for (symbol = nt_root; symbol != NIL; symbol = symbol_list[symbol])
        {
            restore_symbol(tok, RETRIEVE_STRING(symbol));
            if (strlen(line) + strlen(tok) > PRINT_LINE_SIZE)
            {
                PRNT(line);
                print_large_token(line, tok, "    ", LEN);
            }
            else
                strcat(line, tok);
            strcat(line, BLANK);
        }
        PRNT(line);
    }

    ffree(symbol_list);

    return;
}


/*****************************************************************************/
/*                               PRINT_XREF:                                 */
/*****************************************************************************/
/* PRINT_XREF prints out the Cross-reference map. We build a map from each   */
/* terminal into the set of items whose Dot-symbol (symbol immediately       */
/* following the dot ) is the terminal in question.  Note that we iterate    */
/* backwards over the rules to keep the rules associated with the items      */
/* sorted, since they are inserted in STACK fashion in the lists:  Last-in,  */
/* First out.                                                                */
/*****************************************************************************/
static void print_xref(void)
{
    short *sort_sym,
          *t_items;

    int i,
        offset,
        rule_no,
        item_no,
        symbol;

    char line[PRINT_LINE_SIZE + 1],
         tok[SYMBOL_SIZE + 1];

    /*********************************************************************/
    /* SORT_SYM is used to sort the symbols for cross_reference listing. */
    /*********************************************************************/
    sort_sym = Allocate_short_array(num_symbols + 1);
    t_items = Allocate_short_array(num_terminals + 1);

    for ALL_TERMINALS(i)
        t_items[i] = NIL;

    for ALL_RULES_BACKWARDS(rule_no)
    {
        item_no = first_item_of[rule_no];
        for ENTIRE_RHS(i, rule_no)
        {
            symbol = rhs_sym[i];
            if (symbol IS_A_TERMINAL)
            {
                next_item[item_no] = t_items[symbol];
                t_items[symbol] = item_no;
            }
            item_no++;
        }
    }
    for ALL_SYMBOLS(i)
        sort_sym[i] = i;
    quick_sym(sort_sym, 1, num_symbols);

    PR_HEADING;
    fprintf(syslis, "\n\nCross-reference table:\n");
    output_line_no += 3;
    for ALL_SYMBOLS(i)
    {
        symbol = sort_sym[i];
        if (symbol != accept_image && symbol != eoft_image
                                   && symbol != empty)
        {
            fprintf(syslis, "\n");
            ENDPAGE_CHECK;
            restore_symbol(tok, RETRIEVE_STRING(symbol));
            print_large_token(line, tok, "", PRINT_LINE_SIZE-7);
            strcat(line, "  ==>> ");
            offset = strlen(line) - 1;
            if (symbol IS_A_NON_TERMINAL)
            {
                BOOLEAN end_node;

                for (end_node = ((rule_no = lhs_rule[symbol]) == NIL);
                     ! end_node;
                     end_node = (rule_no == lhs_rule[symbol]))
                {
                    rule_no = next_rule[rule_no];
                    sprintf(tok, "%d", rule_no);
                    if (strlen(tok) + strlen(line) > PRINT_LINE_SIZE)
                    {
                        int j;

                        fprintf(syslis, "\n%s", line);
                        ENDPAGE_CHECK;
                        strcpy(line, BLANK);
                        for (j = 1; j <= offset; j++)
                             strcat(line, BLANK);
                    }
                    strcat(line, tok);
                    strcat(line, BLANK);
                }
                fprintf(syslis, "\n%s", line);
                ENDPAGE_CHECK;
                item_no = nt_items[symbol];
            }
            else
            {
                for (item_no = t_items[symbol];
                     item_no != NIL; item_no = next_item[item_no])
                {
                    rule_no = item_table[item_no].rule_number;
                    sprintf(tok, "%d", rule_no);
                    if (strlen(tok) + strlen(line) > PRINT_LINE_SIZE)
                    {
                        int j;

                        fprintf(syslis, "\n%s", line);
                        ENDPAGE_CHECK;
                        strcpy(line, BLANK);
                        for (j = 1; j <= offset; j++)
                             strcat(line, BLANK);
                    }
                    strcat(line, tok);
                    strcat(line, BLANK);
                }
                fprintf(syslis, "\n%s",line);
                ENDPAGE_CHECK;
            }
        }
    }
    fprintf(syslis, "\n\n");
    output_line_no +=2;
    ffree(t_items);
    ffree(sort_sym);

    return;
}


/*****************************************************************************/
/*                              QUICK_SYM:                                   */
/*****************************************************************************/
/* QUICK_SYM takes as arguments an array of pointers whose elements point to */
/* nodes and two integer arguments: L, H. L and H indicate respectively the  */
/* lower and upper bound of a section in the array.                          */
/*****************************************************************************/
static void quick_sym(short array[], int l, int h)
{
    /**************************************************************/
    /* Since no more than 2**15-1 symbols are allowed, the stack  */
    /* not grow past 14.                                          */
    /**************************************************************/

    int   lostack[14],
          histack[14],
          lower,
          upper,
          top,
          i,
          j;

    short temp,
          pivot;

    top = 1;
    lostack[top] = l;
    histack[top] = h;

    while(top != 0)
    {
        lower = lostack[top];
        upper = histack[top--];

        while(upper > lower)
        {
            /********************************************************/
            /* Split the array section indicated by LOWER and UPPER */
            /* using ARRAY[LOWER] as the pivot.                     */
            /********************************************************/
            i = lower;
            pivot = array[lower];
            for (j = lower + 1; j <= upper; j++)
            {
                if (strcmp(RETRIEVE_STRING(array[j]),
                           RETRIEVE_STRING(pivot)) < 0)
                {
                    temp = array[++i];
                    array[i] = array[j];
                    array[j] = temp;
                }
            }

            array[lower] = array[i];
            array[i] = pivot;

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

    return;
}


/*****************************************************************************/
/*                             PRINT_NT_FIRST:                               */
/*****************************************************************************/
/* PRINT_NT_FIRST prints the first set for each non-terminal.                */
/*****************************************************************************/
static void print_nt_first(void)
{
    int nt,
        t;

    char line[PRINT_LINE_SIZE + 1],
         tok[SYMBOL_SIZE + 1];

    PR_HEADING;
    fprintf(syslis, "\nFirst map for non-terminals:\n\n");
    output_line_no += 3;

    for ALL_NON_TERMINALS(nt)
    {
        restore_symbol(tok, RETRIEVE_STRING(nt));
        print_large_token(line, tok, "", PRINT_LINE_SIZE - 7);
        strcat(line, "  ==>> ");
        for ALL_TERMINALS(t)
        {
            if (IS_IN_SET(nt_first, nt, t))
            {
                restore_symbol(tok, RETRIEVE_STRING(t));
                if (strlen(line) + strlen(tok) > PRINT_LINE_SIZE - 1)
                {
                    fprintf(syslis, "\n%s", line);
                    ENDPAGE_CHECK;
                    print_large_token(line, tok, "    ", LEN);
                }
                else
                    strcat(line, tok);
                strcat(line, BLANK);
            }
        }
        fprintf(syslis, "\n%s\n", line);
        output_line_no++;
        ENDPAGE_CHECK;
    }

    return;
}


/*****************************************************************************/
/*                           PRINT_FOLLOW_MAP:                               */
/*****************************************************************************/
/* PRINT_FOLLOW_MAP prints the follow map.                                   */
/*****************************************************************************/
static void print_follow_map(void)
{
    int nt,
        t;

    char line[PRINT_LINE_SIZE + 1],
         tok[SYMBOL_SIZE + 1];

    PR_HEADING;
    fprintf(syslis, "\nFollow Map:\n\n");
    output_line_no += 3;

    for ALL_NON_TERMINALS(nt)
    {
        restore_symbol(tok, RETRIEVE_STRING(nt));
        print_large_token(line, tok, "", PRINT_LINE_SIZE-7);
        strcat(line, "  ==>> ");
        for ALL_TERMINALS(t)
        {
            if (IS_IN_SET(follow, nt, t))
            {
                restore_symbol(tok, RETRIEVE_STRING(t));
                if (strlen(line) + strlen(tok) > PRINT_LINE_SIZE-2)
                {
                    fprintf(syslis, "\n%s", line);
                    ENDPAGE_CHECK;
                    print_large_token(line, tok, "    ", LEN);
                }
                else
                    strcat(line, tok);
                strcat(line, BLANK);
            }
        }
        fprintf(syslis, "\n%s\n", line);
        output_line_no++;
        ENDPAGE_CHECK;
    }
    return;
}
