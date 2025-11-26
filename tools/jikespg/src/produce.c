static char hostfile[] = __FILE__;

#include <string.h>
#include "common.h"
#include "header.h"

static short *stack,
             *index_of,
             *nt_list,
             *item_list,
             *item_of,
             *next_item,
             *scope_table;

static int scope_top = 0;

static struct node **direct_produces;

static struct scope_elmt
{
    short link,
          item,
          index;
} *scope_element;

static BOOLEAN *symbol_seen;

static SET_PTR produces,
               right_produces,
               left_produces;

static int top;

static void compute_produces(int symbol);
static void print_name_map(int symbol);
static void process_scopes(void);
static BOOLEAN is_scope(int item_no);
static BOOLEAN scope_check(int lhs_symbol, int target, int source);
static int insert_prefix(int item_no);
static BOOLEAN is_prefix_equal(int item_no, int item_no2);
static int insert_suffix(int item_no);
static BOOLEAN is_suffix_equal(int item_no1, int item_no2);
static void print_scopes(void);
static int get_shift_symbol(int lhs_symbol);

/****************************************************************************/
/*                              PRODUCE:                                    */
/****************************************************************************/
/* This procedure computes for each state the set of non-terminal symbols   */
/* that are required as candidates for secondary error recovery.  If the    */
/* option NAMES=OPTIMIZED is requested, the NAME map is optimized and SYMNO */
/* is updated accordingly.                                                  */
/****************************************************************************/
void produce(void)
{
    /*****************************************************************/
    /* TOP, STACK, and INDEX are used for the digraph algorithm      */
    /* in the routines COMPUTE_PRODUCES.                             */
    /*                                                               */
    /* The array PRODUCES is used to construct two maps:             */
    /*                                                               */
    /* 1) PRODUCES, a mapping from each non-terminal A to the set of */
    /* non-terminals C such that:                                    */
    /*                                                               */
    /*                   A  =>*  x C w                               */
    /*                                                               */
    /* 2) RIGHT_MOST_PRODUCES, a mapping from each non-terminal A to */
    /* the set of non-terminals C such that:                         */
    /*                                                               */
    /*                   C =>+ A x   and   x =>* %empty.             */
    /*                                                               */
    /* NOTE: This is really a reverse right-most produces mapping,   */
    /*       since given the above rule, we say that                 */
    /*       C right-most produces A.                                */
    /*                                                               */
    /*****************************************************************/

    int state_no,
        state,
        nt_root,
        item_no,
        item_root,
        rule_no,
        symbol,
        nt,
        i,
        n;

    short *names_map;

    struct node **goto_domain,
                *p,
                *q;

    BOOLEAN *name_used,
             end_node;

    SET_PTR set;

    stack = Allocate_short_array(num_symbols + 1);
    index_of = Allocate_short_array(num_symbols + 1);
    names_map = Allocate_short_array(num_names + 1);
    name_used = Allocate_boolean_array(num_names + 1);
    item_list = Allocate_short_array(num_items + 1);
    nt_list = Allocate_short_array(num_non_terminals + 1);
    nt_list -= (num_terminals + 1);
    set = (SET_PTR)
          calloc(1, non_term_set_size * sizeof(BOOLEAN_CELL));
    if (set == NULL)
        nospace(__FILE__, __LINE__);

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

    goto_domain = (struct node **)
                  calloc(num_states + 1, sizeof(struct node *));
    if (goto_domain == NULL)
        nospace(__FILE__, __LINE__);

/*********************************************************************/
/* Note that the space allocated for PRODUCES and DIRECT_PRODUCES    */
/* is automatically initialized to 0 by calloc. Logically, this sets */
/* all the sets in the PRODUCES map to the empty set and all the     */
/* pointers in DIRECT_PRODUCES are set to NULL.                      */
/*                                                                   */
/* Next, PRODUCES is initialized to compute RIGHT_MOST_PRODUCES.     */
/* Also, we count the number of error rules and verify that they are */
/* in the right format.                                              */
/*********************************************************************/
    item_root = NIL;
    for ALL_NON_TERMINALS(nt)
    {
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
                    (! IS_IN_NTSET(produces, symbol, nt - num_terminals)))
                {
                    NTSET_BIT_IN(produces, symbol, nt - num_terminals);
                    q = Allocate_node();
                    q -> value = nt;
                    q -> next = direct_produces[symbol];
                    direct_produces[symbol] = q;
                }
            }
            rule_no = item_table[item_no].rule_number;
            for (i = 0; i < RHS_SIZE(rule_no); i++)
            {
                if (item_table[item_no + i].symbol == error_image)
                    break;
            }
            item_no += i;
            symbol = item_table[item_no].symbol;
            if (symbol == error_image)
            {
                if (item_table[item_no + 1].symbol IS_A_NON_TERMINAL && i > 0)
                {
                   symbol = item_table[item_no + 2].symbol;
                   if (symbol == empty)
                       num_error_rules++;
                }
                if (warnings_bit && symbol != empty)
                {
                    item_list[item_no] = item_root;
                    item_root = item_no;
                }
            }
            symbol = eoft_image;
        }
    }

    /*********************************************************************/
    /* If WARNINGS_BIT is on and some error rules are in the wrong,      */
    /* format, report them.                                              */
    /*********************************************************************/
    if (warnings_bit && item_root != NIL)
    {
        PR_HEADING;
        if (item_list[item_root] == NIL)
            fprintf(syslis,
                    "*** This error rule is not in manual format:\n\n");
        else
            fprintf(syslis,
                    "*** These error rules are not in manual format:\n\n");
        for (item_no = item_root;
             item_no != NIL; item_no = item_list[item_no])
        {
            print_item(item_no);
        }
    }

/********************************************************************/
/* Complete the construction of the RIGHT_MOST_PRODUCES map for     */
/* non-terminals using the digraph algorithm.                       */
/* We make sure that each non-terminal A is not present in its own  */
/* PRODUCES set since we are interested in the non-reflexive        */
/* (positive) transitive closure.                                   */
/********************************************************************/
    for ALL_SYMBOLS(i)
        index_of[i] = OMEGA;

    top = 0;
    for ALL_NON_TERMINALS(nt)
    {
        if (index_of[nt] == OMEGA)
            compute_produces(nt);
        NTRESET_BIT_IN(produces, nt, nt - num_terminals);
    }

/********************************************************************/
/* Construct the minimum subset of the domain of the GOTO map       */
/* needed for automatic secondary level error recovery.   For each  */
/* state, we start out with the set of all nonterminals on which    */
/* there is a transition in that state, and pare it down to a       */
/* subset S, by removing all nonterminals B in S such that there    */
/* is a goto-reduce action on B by a single production.  If the     */
/* READ-REDUCE option is not turned on, then, we check whether or   */
/* not the goto action on B is to an LR(0) reduce state.Once we have*/
/* our subset S, we further reduce its size as follows.  For each   */
/* nonterminal A in S such that there exists another nonterminal    */
/* B in S, where B ^= A,  A ->+ Bx  and  x =>* %empty, we remove A  */
/* from S.                                                          */
/* At the end of this process, the nonterminal elements whose       */
/* NT_LIST values are still OMEGA are precisely the nonterminal     */
/* symbols that are never used as candidates.                       */
/********************************************************************/
    for ALL_NON_TERMINALS(i)
        nt_list[i] = OMEGA;
    nt_list[accept_image] = NIL;

    for ALL_STATES(state_no)
    {
        struct goto_header_type go_to;

        go_to = statset[state_no].go_to;
        nt_root = NIL;
        INIT_NTSET(set);

        for (i = 1; i <= go_to.size; i++)
        {
            symbol = GOTO_SYMBOL(go_to, i);
            state = GOTO_ACTION(go_to, i);
            if (state < 0)
                rule_no = -state;
            else
            {
                q = statset[state].kernel_items;
                item_no = q -> value;
                if (q -> next != NULL)
                    rule_no = 0;
                else
                    rule_no = item_table[item_no].rule_number;
            }
            if (rule_no == 0 || RHS_SIZE(rule_no) != 1)
            {
                nt_list[symbol] = nt_root;
                nt_root = symbol;
                NTSET_UNION(set, 0, produces, symbol);
            }
        }

        goto_domain[state_no] = NULL;
        for (symbol = nt_root; symbol != NIL; symbol = nt_list[symbol])
        {
            if (! IS_ELEMENT(set, symbol - num_terminals))
            {
                q = Allocate_node();
                q -> value = symbol;
                q -> next = goto_domain[state_no];
                goto_domain[state_no] = q;
                gotodom_size++;
            }
        }
    }

    /********************************************************************/
    /* Allocate and construct the permanent goto domain structure:      */
    /*   GD_INDEX and GD_RANGE.                                         */
    /********************************************************************/
    n = 0;
    gd_index = Allocate_short_array(num_states + 2);
    gd_range = Allocate_short_array(gotodom_size + 1);

    for ALL_STATES(state_no)
    {
        gd_index[state_no] = n + 1;
        for (p = goto_domain[state_no]; p != NULL; q = p, p = p -> next)
            gd_range[++n] = p -> value;

        if (goto_domain[state_no] != NULL)
            free_nodes(goto_domain[state_no], q);
    }
    gd_index[num_states + 1] = n + 1;


/******************************************************************/
/* Remove names assigned to nonterminals that are never used as   */
/* error candidates.                                              */
/******************************************************************/
    if (names_opt == OPTIMIZE_PHRASES)
    {
        /*****************************************************************/
        /* In addition to nonterminals that are never used as candidates,*/
        /* if a nullable nonterminal was assigned a name by default      */
        /* (nonterminals that were "named" by default are identified     */
        /* with negative indices), that name is also removed.            */
        /*****************************************************************/
        for ALL_NON_TERMINALS(symbol)
        {
            if (nt_list[symbol] == OMEGA)
                symno[symbol].name_index = symno[accept_image].name_index;
            else if (symno[symbol].name_index < 0)
            {
                if (null_nt[symbol])
                    symno[symbol].name_index = symno[accept_image].name_index;
                else
                    symno[symbol].name_index = - symno[symbol].name_index;
            }
        }


        /*******************************************************************/
        /* Adjust name map to remove unused elements and update SYMNO map. */
        /*******************************************************************/
        for (i = 1; i <= num_names; i++)
            name_used[i] = FALSE;

        for ALL_SYMBOLS(symbol)
            name_used[symno[symbol].name_index] = TRUE;

        n = 0;
        for (i = 1; i <= num_names; i++)
        {
            if (name_used[i])
            {
                name[++n] = name[i];
                names_map[i] = n;
            }
        }
        num_names = n;

        for ALL_SYMBOLS(symbol)
            symno[symbol].name_index = names_map[symno[symbol].name_index];
    }

    /********************************************************************/
    /* If the option LIST_BIT is ON, print the name map.                */
    /********************************************************************/
    if (list_bit)
    {
        PR_HEADING;
        fprintf(syslis, "\nName map:\n");
        output_line_no += 2;

        for ALL_SYMBOLS(symbol)
        {
            if (symno[symbol].name_index != symno[accept_image].name_index)
                print_name_map(symbol);
        }
        for ALL_SYMBOLS(symbol)
        {
            if (symbol != accept_image &&
                symno[symbol].name_index == symno[accept_image].name_index)
                print_name_map(symbol);
        }
    }

    if (scopes_bit)
        process_scopes();

    ffree(stack);
    ffree(index_of);
    ffree(names_map);
    ffree(name_used);
    nt_list += (num_terminals + 1);
    ffree(nt_list);
    ffree(set);
    produces += ((num_terminals + 1) * non_term_set_size);
    ffree(produces);
    direct_produces += (num_terminals + 1);
    ffree(direct_produces);
    ffree(goto_domain);

    return;
}


/******************************************************************/
/*                     COMPUTE_PRODUCES:                          */
/******************************************************************/
/* This procedure is used to compute the transitive closure of    */
/* the PRODUCES, LEFT_PRODUCES and RIGHT_MOST_PRODUCES maps.      */
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



/******************************************************************/
/*                       PRINT_NAME_MAP:                          */
/******************************************************************/
/* This procedure prints the name associated with a given symbol. */
/* The same format that was used in the procedure DISPLAY_INPUT   */
/* to print aliases is used to print name mappings.               */
/******************************************************************/
static void print_name_map(int symbol)
{
    int len;

    char line[PRINT_LINE_SIZE],
         tok[SYMBOL_SIZE + 1];

    restore_symbol(tok, RETRIEVE_STRING(symbol));

    len = PRINT_LINE_SIZE - 5;
    print_large_token(line, tok, "", len);
    strcat(line, " ::= ");
    restore_symbol(tok, RETRIEVE_NAME(symno[symbol].name_index));
    if (strlen(line) + strlen(tok) > PRINT_LINE_SIZE - 1)
    {
        fprintf(syslis, "\n%s", line);
        ENDPAGE_CHECK;
        len = PRINT_LINE_SIZE - 4;
        print_large_token(line, tok, "    ", len);
    }
    else
       strcat(line, tok);

    fprintf(syslis, "\n%s", line);
    ENDPAGE_CHECK;

    return;
}


/******************************************************************/
/*                         PROCESS_SCOPES:                        */
/******************************************************************/
/* Compute set of "scopes" and use it to construct SCOPE map.     */
/******************************************************************/
static void process_scopes(void)
{
    short *prefix_index,
          *suffix_index,
          *state_index;

    struct node **states_of;

    int num_state_sets = 0,
        i,
        j,
        k,
        n;

    short max_prefix_length = 0,
          dot_symbol,
          nt,
          symbol,
          item_root,
          item_no,
          rule_no,
          state_no,
          nt_root;

    BOOLEAN end_node;

    struct node *p,
                *q;

    prefix_index = Allocate_short_array(num_items + 1);
    suffix_index = Allocate_short_array(num_items + 1);
    item_of = Allocate_short_array(num_non_terminals);
    item_of -= (num_terminals + 1);
    next_item = Allocate_short_array(num_items + 1);

    symbol_seen = Allocate_boolean_array(num_non_terminals);
    symbol_seen -= (num_terminals + 1);
    states_of = (struct node **)
                calloc(num_non_terminals, sizeof(struct node *));
    states_of -= (num_terminals + 1);
    state_index = Allocate_short_array(num_non_terminals);
    state_index -= (num_terminals + 1);
    scope_element = (struct scope_elmt *)
                    calloc(num_items + 1, sizeof(struct scope_elmt));

/********************************************************************/
/* Initially, PRODUCES was used to compute the right-most-produces  */
/* map.  We save that map map and make it reflexive.  Recall that   */
/* RIGHT_PRODUCES is a mapping from each nonterminal B into the set */
/* of nonterminals A such that:                                     */
/*                                                                  */
/*    A =>rm* B                                                     */
/*                                                                  */
/* Next, reallocate PRODUCES and initialize it in order to          */
/* construct the LEFT_PRODUCES map. Initially, CALLOC sets PRODUCES */
/* to the empty map.                                                */
/* LEFT_PRODUCES is a mapping  from each nonterminal A into the set */
/* of nonterminals B such that:                                     */
/*                                                                  */
/*    A =>lm* B x                                                   */
/*                                                                  */
/* for some arbitrary string x.                                     */
/*                                                                  */
/* Since A ->* A for all A,  we insert A in PRODUCES(A)  (but not   */
/* in the linked list).                                             */
/*                                                                  */
/********************************************************************/
    right_produces = produces;
    produces = (SET_PTR)
               calloc(num_non_terminals,
                      non_term_set_size * sizeof(BOOLEAN_CELL));
    if (produces == NULL)
        nospace(__FILE__, __LINE__);
    produces -= ((num_terminals + 1) * non_term_set_size);

    for ALL_NON_TERMINALS(nt)
    {
        NTSET_BIT_IN(right_produces, nt, nt - num_terminals);
        NTSET_BIT_IN(produces, nt, nt - num_terminals);

        direct_produces[nt] = NULL;

        for (end_node = ((p = clitems[nt]) == NULL);
             ! end_node; end_node = (p == clitems[nt]))
        {
            p = p -> next;
            for (item_no = p -> value;
                 item_table[item_no].symbol IS_A_NON_TERMINAL;
                 item_no++)
            {
                symbol = item_table[item_no].symbol;
                if (! IS_IN_NTSET(produces, nt, symbol - num_terminals))
                {
                    NTSET_BIT_IN(produces, nt, symbol - num_terminals);
                    q = Allocate_node();
                    q -> value = symbol;
                    q -> next = direct_produces[nt];
                    direct_produces[nt] = q;
                }
                if (! null_nt[symbol])
                    break;
            }
        }
    }

    /****************************************************************/
    /* Complete the construction of the LEFT_produces map for       */
    /* non_terminals using the digraph algorithm.                   */
    /****************************************************************/
    for ALL_NON_TERMINALS(nt)
        index_of[nt] = OMEGA;

    top = 0;
    for ALL_NON_TERMINALS(nt)
    {
        if (index_of[nt] == OMEGA)
            compute_produces(nt);
    }
    left_produces = produces;

/********************************************************************/
/* Allocate and initialize the PRODUCES array to construct the      */
/* PRODUCES map.  After allocation, CALLOC sets all sets to empty.  */
/* Since A ->* A for all A,  we insert A in PRODUCES(A)  (but not   */
/* in the linked list).                                             */
/********************************************************************/
    produces = (SET_PTR)
               calloc(num_non_terminals,
                      non_term_set_size * sizeof(BOOLEAN_CELL));
    if (produces == NULL)
        nospace(__FILE__, __LINE__);
    produces -= ((num_terminals + 1) * non_term_set_size);

    for ALL_NON_TERMINALS(nt)
    {
        NTSET_BIT_IN(produces, nt, nt - num_terminals);

        direct_produces[nt] = NULL;

        for (end_node = ((p = clitems[nt]) == NULL);
             ! end_node; end_node = (p == clitems[nt]))
        {
            p = p -> next;
            for (item_no = p -> value;
                 item_table[item_no].symbol != empty; item_no++)
            {
                symbol = item_table[item_no].symbol;
                if (symbol IS_A_NON_TERMINAL)
                {
                    if (! IS_IN_NTSET(produces, nt, symbol - num_terminals))
                    {
                        NTSET_BIT_IN(produces, nt, symbol - num_terminals);
                        q = Allocate_node();
                        q -> value = symbol;
                        q -> next = direct_produces[nt];
                        direct_produces[nt] = q;
                    }
                }
            }
        }
    }

    /****************************************************************/
    /* Complete the construction of the PRODUCES map for            */
    /* non_terminals using the digraph algorithm.                   */
    /*                                                              */
    /* Since $ACC =>* x A y for all nonterminal A in the grammar, a */
    /* single call to COMPUTE_PRODUCES does the trick.              */
    /****************************************************************/
    for ALL_NON_TERMINALS(nt)
        index_of[nt] = OMEGA;
    top = 0;
    compute_produces(accept_image);

/********************************************************************/
/* Construct a mapping from each non_terminal A into the set of     */
/* items of the form [B  ->  x . A y].                              */
/********************************************************************/
    for ALL_NON_TERMINALS(nt)
        item_of[nt] = NIL;

    for ALL_ITEMS(item_no)
    {
        dot_symbol = item_table[item_no].symbol;
        if (dot_symbol IS_A_NON_TERMINAL)
        {
            next_item[item_no] = item_of[dot_symbol];
            item_of[dot_symbol] = item_no;
        }
    }

/********************************************************************/
/* Construct a list of scoped items in ITEM_LIST.                   */
/* Scoped items are derived from rules of the form  A -> x B y such */
/* that B =>* w A z, %empty not in FIRST(y), and it is not the case */
/* that x = %empty and B ->* A v.                                   */
/* Scoped items may also be identified by the user, using %error    */
/* productions.                                                     */
/* As scoped items are added to the list, we keep track of the      */
/* longest prefix encountered.  This is subsequently used to        */
/* bucket sort the scoped items in descending order of the length    */
/* of their prefixes.                                               */
/********************************************************************/
    for ALL_ITEMS(item_no)
        item_list[item_no] = OMEGA;

    item_root = NIL;
    for ALL_ITEMS(item_no)
    {
        dot_symbol = item_table[item_no].symbol;
        if (dot_symbol == error_image)
        {
            if (item_table[item_no].dot != 0 &&
                (! IS_IN_SET(first,
                             item_table[item_no].suffix_index, empty)))
            {
                if (item_list[item_no] == OMEGA)
                {
                    item_list[item_no] = item_root;
                    item_root = item_no;
                    max_prefix_length = MAX(max_prefix_length,
                                            item_table[item_no].dot);
                }
            }
        }
        else if (dot_symbol IS_A_NON_TERMINAL)
        {
            symbol = rules[item_table[item_no].rule_number].lhs;
            if (! IS_IN_SET(first, item_table[item_no].suffix_index,
                            empty) &&
                IS_IN_NTSET(produces, dot_symbol, symbol - num_terminals))
            {
                if (is_scope(item_no))
                {
                    for (i = item_no + 1; ;i++)
                    {
                        symbol = item_table[i].symbol;
                        if (symbol IS_A_TERMINAL)
                            break;
                        if (! null_nt[symbol])
                            break;
                    }
                    if (symbol IS_A_NON_TERMINAL)
                    {
                        for ALL_NON_TERMINALS(nt)
                            symbol_seen[nt] = FALSE;
                        symbol = get_shift_symbol(symbol);
                    }

                    if (symbol != empty && item_list[i] == OMEGA)
                    {
                        item_list[i] = item_root;
                        item_root = i;
                        max_prefix_length = MAX(max_prefix_length,
                                                item_table[i].dot);
                    }
                }
            }
        }
    }

/*********************************************************************/
/* In this loop, the prefix and suffix string for each scope in      */
/* entered into a table.  We also use the SYMBOL_SEEN array to       */
/* identify the set of left-hand side symbols associated with the    */
/* scopes.                                                           */
/*********************************************************************/
    scope_table = Allocate_short_array(SCOPE_SIZE);
    for (i = 0; i < SCOPE_SIZE; i++)
        scope_table[i] = NIL;

    for ALL_NON_TERMINALS(nt)
        symbol_seen[nt] = FALSE;

    for (item_no = item_root; item_no != NIL; item_no = item_list[item_no])
    {
        rule_no = item_table[item_no].rule_number;
        symbol = rules[rule_no].lhs;
        num_scopes = num_scopes + 1;

        symbol_seen[symbol] = TRUE;

        prefix_index[item_no] = insert_prefix(item_no);
        suffix_index[item_no] = insert_suffix(item_no);
    }
    ffree(scope_table);

/*********************************************************************/
/* We now construct a mapping from each nonterminal symbol that is   */
/* the left-hand side of a rule containing scopes into the set of    */
/* states that has a transition on the nonterminal in question.      */
/*********************************************************************/
    nt_root = NIL;
    for ALL_NON_TERMINALS(nt)
        states_of[nt] = NULL;

    for ALL_STATES(state_no)
    {
        struct goto_header_type go_to;

        go_to = statset[state_no].go_to;
        for (i = 1; i <= go_to.size; i++)
        {
            symbol = GOTO_SYMBOL(go_to, i);
            if (symbol_seen[symbol])
            {
                if (states_of[symbol] == NULL)
                {
                    nt_list[symbol] = nt_root;
                    nt_root = symbol;
                    num_state_sets = num_state_sets + 1;
                }
                q = Allocate_node();
                q -> value = state_no;
                q -> next  = states_of[symbol];
                states_of[symbol] = q;
            }
        }
    }

    right_produces += ((num_terminals + 1) * non_term_set_size);
    ffree(right_produces);
    left_produces += ((num_terminals + 1) * non_term_set_size);
    ffree(left_produces);

/*********************************************************************/
/* Next, we used the optimal partition procedure to compress the     */
/* space used by the sets of states, allocate the SCOPE structure    */
/* and store the compressed sets of states in it.                    */
/* We also sort the list of items by the length of their prefixes in */
/* descending order.  This is done primarily as an optimization.     */
/* If a longer prefix matches prior to a shorter one, the parsing    */
/* will terminate quicker.                                           */
/*********************************************************************/
process_scope_states:
    {
        SET_PTR collection;

        short *element_size,
              *list,
              *start,
              *stack,
              *ordered_symbol,
              *state_list,
              *bucket;

        int state_root,
            state_no;

        state_set_size = num_states / SIZEOF_BC
                                    + (num_states % SIZEOF_BC ? 1 : 0);
        collection = (SET_PTR)
                     calloc(num_state_sets + 1,
                            state_set_size * sizeof(BOOLEAN_CELL));
        if (collection == NULL)
            nospace(__FILE__, __LINE__);

        element_size = Allocate_short_array(num_state_sets + 1);
        start = Allocate_short_array(num_state_sets + 2);
        stack = Allocate_short_array(num_state_sets + 1);
        ordered_symbol = Allocate_short_array(num_state_sets + 1);
        list = Allocate_short_array(num_state_sets + 1);
        state_list = Allocate_short_array(num_states + 1);
        bucket = Allocate_short_array(max_prefix_length + 1);

        for (symbol = nt_root, i = 1;
             symbol != NIL; symbol = nt_list[symbol], i++)
        {
            list[i] = i;
            ordered_symbol[i] = symbol;
            EMPTY_COLLECTION_SET(i);
            element_size[i] = 0;
            for (p = states_of[symbol]; p != NULL; p = p -> next)
            {
                element_size[i]++;
                SET_COLLECTION_BIT(i, p -> value);
            }
        }

        partset(collection, element_size, list, start, stack, num_state_sets);

        for (i = 1; i <= num_state_sets; i++)
        {
            symbol = ordered_symbol[i];
            state_index[symbol] = ABS(start[i]);
        }
        scope_state_size = start[num_state_sets + 1] - 1;

        scope = (struct scope_type *)
                calloc(num_scopes + 1, sizeof(struct scope_type));
        if (scope == NULL)
            nospace(__FILE__, __LINE__);
        scope_right_side = Allocate_short_array(scope_rhs_size + 1);
        scope_state = Allocate_short_array(scope_state_size + 1);

        k = 0;
        for (i = 0; i <= (int) num_states; i++)
            state_list[i] = OMEGA;

        for (i = 1; i <= num_state_sets; i++)
        {
            if (start[i] > 0)
            {
                state_root = 0;
                state_list[state_root] = NIL;

                for (end_node = ((j = i) == NIL);
                     ! end_node; end_node = (j == i))
                {
                    j = stack[j];
                    symbol = ordered_symbol[j];
                    for (p = states_of[symbol]; p != NULL; p = p -> next)
                    {
                        state_no = p -> value;
                        if (state_list[state_no] == OMEGA)
                        {
                            state_list[state_no] = state_root;
                            state_root = state_no;
                        }
                    }
                }

                for (state_no = state_root;
                     state_no != NIL; state_no = state_root)
                {
                    state_root = state_list[state_no];
                    state_list[state_no] = OMEGA;
                    k++;
                    scope_state[k] = state_no;
                }
            }
        }

        for (symbol = nt_root; symbol != NIL; symbol = nt_list[symbol])
        {
            for (p = states_of[symbol]; p != NULL; q = p, p = p -> next)
                ;
            free_nodes(states_of[symbol], q);
        }

/***********************************************************************/
/* Use the BUCKET array as a base to partition the scoped items        */
/* based on the length of their prefixes.  The list of items in each   */
/* bucket is kept in the NEXT_ITEM array sorted in descending order    */
/* of the length of the right-hand side of the item.                   */
/* Items are kept sorted in that fashion because when two items have   */
/* the same prefix, we want the one with the shortest suffix to be     */
/* chosen. In other words, if we have two scoped items, say:           */
/*                                                                     */
/*    A ::= x . y       and      B ::= x . z     where |y| < |z|       */
/*                                                                     */
/* and both of them are applicable in a given context with similar     */
/* result, then we always want A ::= x . y to be used.                 */
/***********************************************************************/
        for (i = 1; i <= max_prefix_length; i++)
            bucket[i] = NIL;

        for (item_no = item_root;
             item_no != NIL; item_no = item_list[item_no])
        {
            int tail;

            k = item_table[item_no].dot;
            for (i = bucket[k]; i != NIL; tail = i, i = next_item[i])
            {
                if (RHS_SIZE(item_table[item_no].rule_number) >=
                    RHS_SIZE(item_table[i].rule_number))
                   break;
            }

            next_item[item_no] = i;
            if (i == bucket[k])
                 bucket[k] = item_no;       /* insert at the beginning */
            else next_item[tail] = item_no; /* insert in middle or end */
        }

/*********************************************************************/
/* Reconstruct list of scoped items in sorted order. Since we want   */
/* the items in descending order, we start with the smallest bucket  */
/* proceeding to the largest one and insert the items from each      */
/* bucket in LIFO order in ITEM_LIST.                                */
/*********************************************************************/
        item_root = NIL;
        for (k = 1; k <= max_prefix_length; k++)
        {
            for (item_no = bucket[k];
                 item_no != NIL; item_no = next_item[item_no])
            {
                item_list[item_no] = item_root;
                item_root = item_no;
            }
        }

        ffree(collection);
        ffree(element_size);
        ffree(start);
        ffree(stack);
        ffree(ordered_symbol);
        ffree(state_list);
        ffree(list);
        ffree(bucket);
    } /* End PROCESS_SCOPE_STATES */

/*********************************************************************/
/* Next, we initialize the remaining fields of the SCOPE structure.  */
/*********************************************************************/
    item_no = item_root;
    for (i = 1; item_no != NIL; i++)
    {
        scope[i].prefix = prefix_index[item_no];
        scope[i].suffix = suffix_index[item_no];
        rule_no = item_table[item_no].rule_number;
        scope[i].lhs_symbol = rules[rule_no].lhs;
        symbol = rhs_sym[rules[rule_no].rhs + item_table[item_no].dot];
        if (symbol IS_A_TERMINAL)
            scope[i].look_ahead = symbol;
        else
        {
            for ALL_NON_TERMINALS(j)
                symbol_seen[j] = FALSE;
            scope[i].look_ahead = get_shift_symbol(symbol);
        }
        scope[i].state_set = state_index[scope[i].lhs_symbol];
        item_no = item_list[item_no];
    }

    for (j = 1; j <= scope_top; j++)
    {
        if (scope_element[j].item < 0)
        {
            item_no = -scope_element[j].item;
            rule_no = item_table[item_no].rule_number;
            n = scope_element[j].index;
            for (k = rules[rule_no].rhs+item_table[item_no].dot - 1;
                 k >= rules[rule_no].rhs; /* symbols before dot*/
                 k--)
                scope_right_side[n++] = rhs_sym[k];
        }
        else
        {
            item_no = scope_element[j].item;
            rule_no = item_table[item_no].rule_number;
            n = scope_element[j].index;
            for (k = rules[rule_no].rhs + item_table[item_no].dot;
                 k < rules[rule_no + 1].rhs; /* symbols after dot */
                 k++)
            {
                symbol = rhs_sym[k];
                if (symbol IS_A_NON_TERMINAL)
                {
                    if (! null_nt[symbol])
                        scope_right_side[n++] = rhs_sym[k];
                }
                else if (symbol != error_image)
                    scope_right_side[n++] = rhs_sym[k];
            }
        }
        scope_right_side[n] = 0;
    }

    if (list_bit)
        print_scopes();

    ffree(prefix_index);
    ffree(suffix_index);
    item_of += (num_terminals + 1);
    ffree(item_of);
    ffree(next_item);
    symbol_seen += (num_terminals + 1);
    ffree(symbol_seen);
    states_of += (num_terminals + 1);
    ffree(states_of);
    state_index += (num_terminals + 1);
    ffree(state_index);
    ffree(scope_element);

    return;
}


/*********************************************************************/
/*                              IS_SCOPE:                            */
/*********************************************************************/
/* This procedure checks whether or not an item of the form:         */
/* [A  ->  w B x  where  B ->* y A z  is a valid scope.              */
/*                                                                   */
/* Such an item is a valid scope if the following conditions hold:   */
/*                                                                   */
/* 1) it is not the case that x =>* %empty                           */
/* 2) either it is not the case that w =>* %empty or it is not the   */
/*    case that B =>lm* A.                                           */
/* 3) it is not the case that whenever A is introduced through       */
/*    closure, it is introduced by a nonterminal C where C =>rm* A   */
/*    and C =>rm+ B.                                                 */
/*********************************************************************/
static BOOLEAN is_scope(int item_no)
{
    int i,
        nt,
        lhs_symbol,
        target,
        symbol;

    for (i = item_no - item_table[item_no].dot; i < item_no; i++)
    {
        symbol = item_table[i].symbol;
        if (symbol IS_A_TERMINAL)
            return(TRUE);
        if (! null_nt[symbol])
            return(TRUE);
    }

    lhs_symbol = rules[item_table[item_no].rule_number].lhs;
    target = item_table[item_no].symbol;
    if (IS_IN_NTSET(left_produces, target, lhs_symbol - num_terminals))
        return(FALSE);

    if (item_table[item_no].dot > 0)
        return(TRUE);

    for ALL_NON_TERMINALS(nt)
        symbol_seen[nt] = FALSE;

    return(scope_check(lhs_symbol, target, lhs_symbol));
}

/*********************************************************************/
/*                             SCOPE_CHECK:                          */
/*********************************************************************/
/* Given a nonterminal LHS_SYMBOL and a nonterminal TARGET where,    */
/*                                                                   */
/*                     LHS_SYMBOL ::= TARGET x                       */
/*                                                                   */
/* find out if whenever LHS_SYMBOL is introduced through closure, it */
/* is introduced by a nonterminal SOURCE such that                   */
/*                                                                   */
/*                     SOURCE ->rm* LHS_SYMBOL                       */
/*                                                                   */
/*                               and                                 */
/*                                                                   */
/*                     SOURCE ->rm+ TARGET                           */
/*                                                                   */
/*********************************************************************/
static BOOLEAN scope_check(int lhs_symbol, int target, int source)
{
    int item_no,
        rule_no,
        symbol;

    symbol_seen[source] = TRUE;

    if (IS_IN_NTSET(right_produces, target, source - num_terminals) &&
        IS_IN_NTSET(right_produces, lhs_symbol, source - num_terminals))
        return(FALSE);

    for (item_no = item_of[source];
         item_no != NIL; item_no = next_item[item_no])
    {
        if (item_table[item_no].dot != 0)
            return(TRUE);

        rule_no = item_table[item_no].rule_number;
        symbol = rules[rule_no].lhs;
        if (! symbol_seen[symbol])        /* not yet processed */
        {
            if (scope_check(lhs_symbol, target, symbol))
                return(TRUE);
        }
    }

    return(FALSE);
}


/*********************************************************************/
/*                       INSERT_PREFIX:                              */
/*********************************************************************/
/* This procedure takes as argument an item and inserts the string   */
/* prefix of the item preceeding the "dot" into the scope table, if  */
/* that string is not already there.  In any case, the index  number */
/* associated with the prefix in question is returned.               */
/* NOTE that since both prefixes and suffixes are entered in the     */
/* table, the prefix of a given item, ITEM_NO, is encoded as         */
/* -ITEM_NO, whereas the suffix of that item is encoded as +ITEM_NO. */
/*********************************************************************/
static int insert_prefix(int item_no)
{
    int i,
        j,
        rule_no;

    unsigned long hash_address = 0;

    rule_no = item_table[item_no].rule_number;
    for (i = rules[rule_no].rhs;    /* symbols before dot */
         i < rules[rule_no].rhs + item_table[item_no].dot; i++)
        hash_address += rhs_sym[i];

    i = hash_address % SCOPE_SIZE;

    for (j = scope_table[i]; j != NIL; j = scope_element[j].link)
    {
        if (is_prefix_equal(scope_element[j].item, item_no))
            return(scope_element[j].index);
    }
    scope_top++;
    scope_element[scope_top].item = -item_no;
    scope_element[scope_top].index = scope_rhs_size + 1;
    scope_element[scope_top].link = scope_table[i];
    scope_table[i] = scope_top;

    scope_rhs_size += (item_table[item_no].dot + 1);

    return(scope_element[scope_top].index);
}


/******************************************************************/
/*                      IS_PREFIX_EQUAL:                          */
/******************************************************************/
/* This boolean function takes two items as arguments and checks  */
/* whether or not they have the same prefix.                      */
/******************************************************************/
static BOOLEAN is_prefix_equal(int item_no, int item_no2)
{
    int item_no1,
        start,
        dot,
        i,
        j;

    if (item_no > 0)    /* a suffix */
        return(FALSE);

    item_no1 = -item_no;
    if (item_table[item_no1].dot != item_table[item_no2].dot)
        return(FALSE);

    j = rules[item_table[item_no1].rule_number].rhs;
    start = rules[item_table[item_no2].rule_number].rhs;
    dot = start + item_table[item_no2].dot - 1;
    for (i = start; i <= dot; i++)    /* symbols before dot */
    {
        if (rhs_sym[i] != rhs_sym[j])
            return(FALSE);
        j++;
    }

    return(TRUE);
}


/*********************************************************************/
/*                            INSERT_SUFFIX:                         */
/*********************************************************************/
/* This procedure is analoguous to INSERT_PREFIX.  It takes as       */
/* argument an item, and inserts the suffix string following the dot */
/* in the item into the scope table, if it is not already there.     */
/* In any case, it returns the index associated with the suffix.     */
/* When inserting a suffix into the table, all nullable nonterminals */
/* in the suffix are disregarded.                                    */
/*********************************************************************/
static int insert_suffix(int item_no)
{
    int i,
        j,
        rule_no,
        num_elements = 0;

    unsigned long hash_address = 0;

    rule_no = item_table[item_no].rule_number;
    for (i = rules[rule_no].rhs + item_table[item_no].dot;
         i < rules[rule_no + 1].rhs; /* symbols after dot */
         i++)
    {
        if (rhs_sym[i] IS_A_NON_TERMINAL)
        {
            if (! null_nt[rhs_sym[i]])
            {
                hash_address += rhs_sym[i];
                num_elements++;
            }
        }
        else if (rhs_sym[i] != error_image)
        {
            hash_address += rhs_sym[i];
            num_elements++;
        }
    }

    i = hash_address % SCOPE_SIZE;

    for (j = scope_table[i]; j != NIL; j = scope_element[j].link)
    {
        if (is_suffix_equal(scope_element[j].item, item_no))
            return(scope_element[j].index);
    }
    scope_top++;
    scope_element[scope_top].item = item_no;
    scope_element[scope_top].index = scope_rhs_size + 1;
    scope_element[scope_top].link = scope_table[i];
    scope_table[i] = scope_top;

    scope_rhs_size += (num_elements + 1);

    return(scope_element[scope_top].index);
}


/******************************************************************/
/*                        IS_SUFFIX_EQUAL:                        */
/******************************************************************/
/* This boolean function takes two items as arguments and checks  */
/* whether or not they have the same suffix.                      */
/******************************************************************/
static BOOLEAN is_suffix_equal(int item_no1, int item_no2)
{
    int rule_no,
        dot1,
        dot2,
        i,
        j;

    if (item_no1 < 0)      /* a prefix */
        return(FALSE);

    rule_no = item_table[item_no1].rule_number;
    i = rules[rule_no].rhs + item_table[item_no1].dot;
    dot1 = rules[rule_no + 1].rhs - 1;

    rule_no = item_table[item_no2].rule_number;
    j = rules[rule_no].rhs + item_table[item_no2].dot;
    dot2 = rules[rule_no + 1].rhs - 1;

    while (i <= dot1 && j <= dot2) /* non-nullable syms before dot */
    {
        if (rhs_sym[i] IS_A_NON_TERMINAL)
        {
            if (null_nt[rhs_sym[i]])
            {
                i++;
                continue;
            }
        }
        else if (rhs_sym[i] == error_image)
        {
            i++;
            continue;
        }

        if (rhs_sym[j] IS_A_NON_TERMINAL)
        {
            if (null_nt[rhs_sym[j]])
            {
                j++;
                continue;
            }
        }
        else if (rhs_sym[j] == error_image)
        {
            j++;
            continue;
        }

        if (rhs_sym[i] != rhs_sym[j])
            return(FALSE);

        j++;
        i++;
    }

    for (; i <= dot1; i++)
    {
        if (rhs_sym[i] IS_A_NON_TERMINAL)
        {
            if (! null_nt[rhs_sym[i]])
                return(FALSE);
        }
        else if (rhs_sym[i] != error_image)
            return(FALSE);
    }

    for (; j <= dot2; j++)
    {
        if (rhs_sym[j] IS_A_NON_TERMINAL)
        {
            if (! null_nt[rhs_sym[j]])
                return(FALSE);
        }
        else if (rhs_sym[j] != error_image)
            return(FALSE);
    }

    return(TRUE);
}


/******************************************************************/
/*                         PRINT_SCOPES:                          */
/******************************************************************/
/* This procedure is similar to the global procedure PTITEM.      */
/******************************************************************/
static void print_scopes(void)
{
    int len,
        offset,
        i,
        k,
        symbol;

    char line[PRINT_LINE_SIZE + 1],
         tok[SYMBOL_SIZE + 1],
         tmp[PRINT_LINE_SIZE];

    PR_HEADING;
    fprintf(syslis, "\nScopes:\n");
    output_line_no += 2;

    for (k=1; k <= num_scopes; k++)
    {
        symbol = scope[k].lhs_symbol;
        restore_symbol(tok, RETRIEVE_STRING(symbol));
        len = PRINT_LINE_SIZE - 5;
        print_large_token(line, tok, "", len);
        strcat(line, " ::= ");
        i = (PRINT_LINE_SIZE / 2) - 1;
        offset = MIN(strlen(line) - 1, i);
        len = PRINT_LINE_SIZE - (offset + 4);

        /* locate end of list */
        for (i = scope[k].prefix; scope_right_side[i] != 0; i++)
            ;

        for (i = i - 1; i >= scope[k].prefix; i--) /* symbols before dot */
        {
            symbol = scope_right_side[i];
            restore_symbol(tok, RETRIEVE_STRING(symbol));
            if (strlen(line) + strlen(tok) > PRINT_LINE_SIZE - 4)
            {
                fprintf(syslis, "\n%s", line);
                ENDPAGE_CHECK;
                fill_in(tmp, offset, ' ');
                print_large_token(line, tok, tmp, len);
            }
            else
                strcat(line, tok);
            strcat(line, BLANK);
        }

/*********************************************************************/
/* We now add a dot "." to the output line, and print the remaining  */
/* symbols in the right hand side.                                   */
/*********************************************************************/
        strcat(line, " .");
        len = PRINT_LINE_SIZE - (offset + 1);

        for (i = scope[k].suffix;  scope_right_side[i] != 0; i++)
        {
            symbol = scope_right_side[i];
            restore_symbol(tok, RETRIEVE_STRING(symbol));
            if (strlen(line) + strlen(tok) > PRINT_LINE_SIZE - 1)
            {
                fprintf(syslis, "\n%s", line);
                ENDPAGE_CHECK;
                fill_in(tmp, offset, ' ');
                print_large_token(line, tok, tmp, len);
            }
            else
                strcat(line, tok);
            strcat(line, BLANK);
        }
        fprintf(syslis, "\n%s", line);
        ENDPAGE_CHECK;
    }

    return;
}


/*********************************************************************/
/*                           GET_SHIFT_SYMBOL:                       */
/*********************************************************************/
/* This procedure takes as parameter a nonterminal, LHS_SYMBOL, and  */
/* determines whether or not there is a terminal symbol t such that  */
/* LHS_SYMBOL can rightmost produce a string tX.  If so, t is        */
/* returned, otherwise EMPTY is returned.                            */
/*********************************************************************/
static int get_shift_symbol(int lhs_symbol)
{
    int item_no,
        rule_no,
        symbol;

    BOOLEAN end_node;

    struct node *p;

    if (! symbol_seen[lhs_symbol])
    {
        symbol_seen[lhs_symbol] = TRUE;

        for (end_node = ((p = clitems[lhs_symbol]) == NULL);
             ! end_node; end_node = (p == clitems[lhs_symbol]))
        {
            p = p -> next;
            item_no = p -> value;
            rule_no = item_table[item_no].rule_number;
            if (RHS_SIZE(rule_no) > 0)
            {
                symbol = rhs_sym[rules[rule_no].rhs];
                if (symbol IS_A_TERMINAL)
                    return(symbol);
                else
                {
                    symbol = get_shift_symbol(symbol);
                    if (symbol != empty)
                        return(symbol);
                }
            }
        }
    }

    return(empty);
}
