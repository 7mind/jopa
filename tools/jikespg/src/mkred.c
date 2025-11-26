static char hostfile[] = __FILE__;

#include "common.h"
#include "reduce.h"
#include "header.h"

/***********************************************************************/
/* STACK_ROOT is used in la_traverse to construct a stack of symbols.  */
/* The boolean vector SINGLE_COMPLETE_ITEM identifies states whose     */
/* kernel consists of a single final item and other conditions allows  */
/* us to compute default reductions for such states.                   */
/* The vector LA_BASE is used in COMPUTE_READ and TRACE_LALR_PATH to   */
/* identify states whose read sets can be completely computed from     */
/* their kernel items.                                                 */
/***********************************************************************/
static struct node *stack_root = NULL;
static BOOLEAN *single_complete_item;
static int *la_base;

/*****************************************************************************/
/*                                 LPGACCESS:                                */
/*****************************************************************************/
/* Given a STATE_NO and an ITEM_NO, ACCESS computes the set of states where  */
/* the rule from which ITEM_NO is derived was introduced through closure.    */
/*****************************************************************************/
struct node *lpgaccess(int state_no, int item_no)
{
    int i;

    BOOLEAN end_node;

    struct node *access_root,
                *head,
                *tail,
                *q,
                *p,
                *s;

/****************************************************************/
/* Build a list pointed to by ACCESS_ROOT originally consisting */
/* only of STATE_NO.                                            */
/****************************************************************/
    access_root = Allocate_node();
    access_root -> value = state_no;
    access_root -> next = NULL;

    for (i = item_table[item_no].dot; i > 0; i--)/*distance to travel is DOT */
    {
        head = access_root;  /* Save old ACCESS_ROOT */
        access_root = NULL;  /* Initialize ACCESS_ROOT for new list */
        for (p = head; p != NULL; tail = p, p = p -> next)
        {
            /***********************************************************/
            /* Compute set of states with transition into p->value.    */
            /***********************************************************/
            for (end_node = ((s = in_stat[p -> value]) == NULL);
                 ! end_node;
                 end_node = (s == in_stat[p -> value]))
            {
                s = s -> next;

                q = Allocate_node();
                q -> value = s -> value;
                q -> next = access_root;
                access_root = q;
            }
        }

        free_nodes(head, tail);    /* free previous list */
     }

     return(access_root);
}


/*****************************************************************************/
/*                              TRACE_LALR_PATH:                             */
/*****************************************************************************/
/* Given an item of the form: [x .A y], where x and y are arbitrary strings, */
/* and A is a non-terminal, we pretrace the path(s) in the automaton  that   */
/* will be followed in computing the look-ahead set for that item in         */
/* STATE_NO.  A number is assigned to all pairs (S, B), where S is a state,  */
/* and B is a non-terminal, involved in the paths. GOTO_INDX points to the   */
/* GOTO_ELEMENT of (STATE_NO, A).                                            */
/*****************************************************************************/
static void trace_lalr_path(int state_no, int goto_indx)
{
    int i,
        symbol,
        item,
        state;

    struct goto_header_type go_to;

    BOOLEAN contains_empty;

    struct node *p,
                *r,
                *t,
                *w;

/*********************************************************************/
/*  If STATE is a state number we first check to see if its base     */
/* look-ahead set is a special one that does not contain EMPTY and   */
/* has already been assigned a slot that can be reused.              */
/* ((LA_BASE[STATE] != OMEGA) signals this condition.)               */
/* NOTE that look-ahead follow sets are shared only when the maximum */
/* look-ahead level allowed is 1 and single productions will not be  */
/* removed. If either (or both) of these conditions is true, we need */
/* to have a unique slot assigned to each pair [S, A] (where S is a  */
/* state, and A is a non-terminal) in the automaton.                 */
/*********************************************************************/
    go_to = statset[state_no].go_to;
    state = GOTO_ACTION(go_to, goto_indx);
    if (state > 0)
    {
        if (la_base[state] != OMEGA &&
            lalr_level == 1 && (! single_productions_bit))
        {
            GOTO_LAPTR(go_to, goto_indx) = la_base[state];
            return;
        }
        r = statset[state].kernel_items;
    }
    else
        r = adequate_item[-state];

/***********************************************************************/
/* At this point, R points to a list of items which are the successors */
/* of the items needed to initialize the Look-ahead follow sets.  If   */
/* anyone of these items contains EMPTY, we trace the Digraph for other*/
/* look-ahead follow sets that may be needed, and signal this fact     */
/* using the variable CONTAINS_EMPTY.                                  */
/***********************************************************************/
    la_top++;   /* allocate new slot */
    INT_CHECK(la_top);
    GOTO_LAPTR(go_to, goto_indx) = la_top;
    contains_empty = FALSE;

    for (; r != NULL;  r = r -> next)
    {
        item = r -> value - 1;
        if (IS_IN_SET(first, item_table[item].suffix_index, empty))
        {
            contains_empty = TRUE;
            symbol = rules[item_table[item].rule_number].lhs;
            w = lpgaccess(state_no, item);
            for (t = w; t != NULL; p = t, t = t -> next)
            {
                struct goto_header_type go_to;

                go_to = statset[t -> value].go_to;

                for (i = 1; GOTO_SYMBOL(go_to, i) != symbol; i++)
                    ;

                if (GOTO_LAPTR(go_to, i) == OMEGA)
                    trace_lalr_path(t -> value, i);
            }

            free_nodes(w, p);
        }
    }

/********************************************************************/
/* If the look-ahead follow set involved does not contain EMPTY, we */
/* mark the state involved (STATE) so that other look-ahead follow  */
/* sets which may need this set may reuse the same one.             */
/* NOTE that if CONTAINS_EMPTY is false, then STATE has to denote a */
/* state number (positive value) and not a rule number (negative).  */
/********************************************************************/
    if (! contains_empty)
        la_base[state] = GOTO_LAPTR(go_to, goto_indx);

    return;
}


/*****************************************************************************/
/*                               COMPUTE_READ:                               */
/*****************************************************************************/
/* COMPUTE_READ computes the number of intermediate look-ahead sets that     */
/* will be needed (in LA_TOP), allocates space for the sets(LA_SET), and    */
/* initializes them.                                                         */
/*  By intermediate look-ahead set, we mean the set of terminals that may    */
/* follow a non-terminal in a given state.                                   */
/*  These sets are initialized to the set of terminals that can immediately  */
/* follow the non-terminal in the state to which it can shift (READ set).    */
/*****************************************************************************/
static void compute_read(void)
{
    int state_no,
        item_no,
        rule_no,
        lhs_symbol,
        state,
        i,
        j,
        la_ptr;

    struct node *p,
                *q,
                *s,
                *v;

    if (lalr_level > 1 || single_productions_bit)
    {
        read_set = (SET_PTR)
                   calloc(num_states + 1,
                          sizeof(BOOLEAN_CELL) * term_set_size);
        if (read_set == NULL)
            nospace(__FILE__, __LINE__);
    }

/************************************************************************/
/*  We traverse all the states and for all complete items that requires */
/* a look-ahead set, we retrace the state digraph (with the help of the */
/* routine TRACE_LALR_PATH) and assign a unique number to all look-ahead*/
/* follow sets that it needs. A look-ahead follow set is a set of       */
/* terminal symbols associated with a pair [S, A], where S is a state,  */
/* and A is a non-terminal:                                             */
/*                                                                      */
/* [S, A] --> Follow-set                                                */
/* Follow-set = {t | t is a terminal that can be shifted on after       */
/*                      execution of a goto action on A in state S}.    */
/*                                                                      */
/* Each follow set is initialized with the set of terminals that can be */
/* shifted on in state S2, where GOTO(S, A) = S2. After initialization  */
/* a follow set F that does not contain the special terminal symbol     */
/* EMPTY is marked with the help of the array LA_BASE, and if the       */
/* highest level of look-ahead allowed is 1, then only one such set is  */
/* allocated, and shared for all pairs (S, B) whose follow set is F.    */
/************************************************************************/
    la_top = 0;
    la_base = (int *) calloc(num_states + 1, sizeof(int));
    if (la_base == NULL)
        nospace(__FILE__, __LINE__);

    for ALL_STATES(i)
        la_base[i] = OMEGA;

    for ALL_STATES(state_no)
    {
        for (p = ((lalr_level <= 1 && single_complete_item[state_no])
                  ? NULL
                  : statset[state_no].complete_items);
             p != NULL; p = p -> next)
        {
            item_no = p -> value;
            rule_no = item_table[item_no].rule_number;
            lhs_symbol = rules[rule_no].lhs;
            if (lhs_symbol != accept_image)
            {
                v = lpgaccess(state_no, item_no);
                for (s = v; s != NULL; q = s, s = s -> next)
                {
                    struct goto_header_type go_to;

                    go_to = statset[s -> value].go_to;
                    for (i = 1; GOTO_SYMBOL(go_to, i) != lhs_symbol; i++)
                        ;

                    if (GOTO_LAPTR(go_to, i) == OMEGA)
                        trace_lalr_path(s -> value, i);
                }

                free_nodes(v, q);
            }
        }

    /***********************************************************************/
    /*  If the look-ahead level is greater than 1 or single productions    */
    /* actions are to be removed when possible, then we have to compute    */
    /* a Follow-set for all pairs [S, A] in the state automaton. Therefore,*/
    /* we also have to consider Shift-reduce actions as reductions, and    */
    /* trace back to their roots as well.                                  */
    /* Note that this is not necessary for Goto-reduce actions. Since      */
    /* they terminate with a non-terminal, and that non-terminal is        */
    /* followed by the empty string, and we know that it must produce a    */
    /* rule that either ends up in a reduction, a shift-reduce, or another */
    /* goto-reduce. It will therefore be taken care of automatically by    */
    /* transitive closure.                                                 */
    /***********************************************************************/
        if (lalr_level > 1 || single_productions_bit)
        {
            struct shift_header_type sh;
            struct goto_header_type  go_to;

            sh = shift[statset[state_no].shift_number];
            for (j = 1; j <= sh.size; j++)
            {
                if (SHIFT_ACTION(sh, j) < 0)
                {
                    rule_no = - SHIFT_ACTION(sh, j);
                    lhs_symbol = rules[rule_no].lhs;
                    item_no = adequate_item[rule_no] -> value - 1;
                    v = lpgaccess(state_no, item_no);
                    for (s = v; s != NULL; q = s, s = s -> next)
                    {
                        go_to = statset[s -> value].go_to;
                        for (i = 1; GOTO_SYMBOL(go_to, i) != lhs_symbol; i++)
                            ;
                        if (GOTO_LAPTR(go_to, i) == OMEGA)
                            trace_lalr_path(s -> value, i);
                    }

                    free_nodes(v, q);
                }
            }

        /*******************************************************************/
        /* We also need to compute the set of terminal symbols that can be */
        /* read in a state entered via a terminal transition.              */
        /*******************************************************************/
            if (lalr_level > 1 && state_no != 1)
            {
                q = statset[state_no].kernel_items;
                item_no = q -> value - 1;
                if (item_table[item_no].symbol IS_A_TERMINAL)
                {
                    ASSIGN_SET(read_set, state_no, first,
                               item_table[item_no].suffix_index);
                    for (q = q -> next; q != NULL; q = q -> next)
                    {
                        item_no = q -> value - 1;
                        SET_UNION(read_set, state_no, first,
                                  item_table[item_no].suffix_index);
                    }
                }
            }
        }
    }


/*********************************************************************/
/*   We now allocate space for LA_INDEX and LA_SET, and initialize   */
/* all its elements as indicated in reduce.h. The array LA_BASE is   */
/* used to keep track of Follow sets that have been initialized. If  */
/* another set needs to be initialized with a value that has been    */
/* already computed, LA_BASE is used to retrieve the value.          */
/*********************************************************************/
    for ALL_STATES(i)
        la_base[i] = OMEGA;

    la_index = Allocate_short_array(la_top + 1);

    la_set = (SET_PTR)
             calloc(la_top + 1, term_set_size * sizeof(BOOLEAN_CELL));
    if (la_set == NULL)
        nospace(__FILE__, __LINE__);

    for ALL_STATES(state_no)
    {
        struct goto_header_type go_to;

        go_to = statset[state_no].go_to;
        for (i = 1; i <= go_to.size; i++)
        {
            la_ptr = GOTO_LAPTR(go_to, i);
            if (la_ptr != OMEGA)   /* Follow Look-ahead needed */
            {
                state = GOTO_ACTION(go_to, i);
                if (state > 0)
                {
                    if (la_base[state] != OMEGA)  /* already computed */
                    {
                        la_index[la_ptr] = la_index[la_base[state]];
                        ASSIGN_SET(la_set, la_ptr,
                                   la_set, la_base[state]);
                        q = NULL;
                    }
                    else
                    {
                        la_base[state] = la_ptr;
                        q = statset[state].kernel_items;
                    }
                }
                else
                    q = adequate_item[-state];
                if (q != NULL)
                {
                    item_no = q -> value - 1;
                    /* initialize with first item */
                    ASSIGN_SET(la_set, la_ptr,
                               first, item_table[item_no].suffix_index);
                    for (q = q -> next; q != NULL; q = q -> next)
                    {
                        item_no = q -> value - 1;
                        SET_UNION(la_set, la_ptr, first,
                                  item_table[item_no].suffix_index);
                    }
                    if (IS_IN_SET(la_set, la_ptr, empty))
                        la_index[la_ptr] = OMEGA;
                    else
                        la_index[la_ptr] = INFINITY;
                    if (lalr_level > 1 || single_productions_bit)
                    {
                        if(state > 0)
                        {
                            ASSIGN_SET(read_set, state, la_set, la_ptr);
                        }
                    }
                }
            }
        }
    }

    ffree(la_base);

    return;
}


/*****************************************************************************/
/*                                LA_TRAVERSE:                               */
/*****************************************************************************/
/* LA_TRAVERSE takes two major arguments: STATE_NO, and an index (GOTO_INDX) */
/* that points to the GOTO_ELEMENT array in STATE_NO for the non-terminal    */
/* left hand side of an item for which look-ahead is to be computed. The     */
/* look-ahead of an item of the form [x. A y] in state STATE_NO is the set   */
/* of terminals that can appear immediately after A in the context summarized*/
/* by STATE_NO. When a look-ahead set is computed, the result is placed in   */
/* an allocation of LA_ELEMENT pointed to by the LA_PTR field of the         */
/* GOTO_ELEMENT array.                                                       */
/*                                                                           */
/* The same digraph algorithm used in MKFIRST is used for this computation.  */
/*                                                                           */
/*****************************************************************************/
void la_traverse(int state_no, int goto_indx, int *stack_top)
{
    int indx;

    int symbol,
        item,
        state,
        i,
        la_ptr;

    struct goto_header_type go_to;

    struct node *r,
                *s,
                *t,
                *w;

    go_to = statset[state_no].go_to;
    la_ptr =  GOTO_LAPTR(go_to, goto_indx);

    s = Allocate_node();    /* Push LA_PTR down the stack */
    s -> value = la_ptr;
    s -> next = stack_root;
    stack_root = s;

    indx = ++(*stack_top); /* one element was pushed into the stack */
    la_index[la_ptr] = indx;

/**********************************************************************/
/* Compute STATE, action to perform on Goto symbol in question. If    */
/* STATE is positive, it denotes a state to which to shift. If it is  */
/* negative, it is a rule on which to perform a Goto-Reduce.          */
/**********************************************************************/
    state = GOTO_ACTION(go_to, goto_indx);
    if (state > 0)     /* Not a Goto-Reduce action */
        r = statset[state].kernel_items;
    else
        r = adequate_item[-state];

    for (; r != NULL; r = r -> next)
    {  /* loop over items [A -> x LHS_SYMBOL . y] */
        item = r -> value - 1;
        if IS_IN_SET(first, item_table[item].suffix_index, empty)
        {
            symbol = rules[item_table[item].rule_number].lhs;
            w = lpgaccess(state_no, item); /* states where RULE was  */
                                     /* introduced through closure   */
            for (t = w; t != NULL; s = t, t = t -> next)
            {
                struct goto_header_type go_to;

                /**********************************************************/
                /* Search for GOTO action in access-state after reducing  */
                /* RULE to its left hand side (SYMBOL). Q points to the   */
                /* GOTO_ELEMENT in question.                              */
                /**********************************************************/
                go_to = statset[t -> value].go_to;
                for (i = 1; GOTO_SYMBOL(go_to, i) != symbol; i++)
                    ;
                if (la_index[GOTO_LAPTR(go_to, i)] == OMEGA)
                    la_traverse(t -> value, i, stack_top);
                SET_UNION(la_set, la_ptr,
                          la_set, GOTO_LAPTR(go_to, i));
                la_index[la_ptr] = MIN(la_index[la_ptr],
                                       la_index[GOTO_LAPTR(go_to, i)]);
            }

            free_nodes(w, s);
        }
    }

    if (la_index[la_ptr] == indx) /* Top of a SCC */
    {
        s = stack_root;

        for (i = stack_root -> value; i != la_ptr;
             stack_root = stack_root -> next, i = stack_root -> value)
        {
            ASSIGN_SET(la_set, i, la_set, la_ptr);
            la_index[i] = INFINITY;
            (*stack_top)--; /* one element was popped from the stack; */
        }
        la_index[la_ptr] = INFINITY;
        r = stack_root; /* mark last element that is popped and ... */
        stack_root = stack_root -> next; /* ... pop it! */
        (*stack_top)--; /* one element was popped from the stack; */

        free_nodes(s, r);
    }

    return;
}

/*****************************************************************************/
/*                               COMPUTE_LA:                                 */
/*****************************************************************************/
/* COMPUTE_LA takes as argument a state number (STATE_NO), an item number    */
/* (ITEM_NO), and a set (LOOK_AHEAD).  It computes the look-ahead set of     */
/* terminals for the given item in the given state and places the answer in  */
/* the set LOOK_AHEAD.                                                       */
/*****************************************************************************/
void compute_la(int state_no, int item_no, SET_PTR look_ahead)
{
    int lhs_symbol,
        i;

    struct node *r,
                *s,
                *v;

    stack_root = NULL;
    lhs_symbol = rules[item_table[item_no].rule_number].lhs;
    if (lhs_symbol == accept_image)
    {
        ASSIGN_SET(look_ahead, 0,
                   first, item_table[item_no-1].suffix_index);
        return;
    }

    INIT_SET(look_ahead);  /* initialize set */

    v = lpgaccess(state_no, item_no);
    for (s = v; s != NULL; r = s, s = s -> next)
    {
        struct  goto_header_type go_to;

        /*****************************************************************/
        /* Search for GOTO action in Access-State after reducing rule to */
        /* its left hand side(LHS_SYMBOL). Q points to the state.        */
        /*****************************************************************/
        go_to = statset[s -> value].go_to;
        for (i = 1; GOTO_SYMBOL(go_to, i) != lhs_symbol; i++)
            ;

        /***********************************************************/
        /* If look-ahead after left hand side is not yet computed, */
        /* LA_TRAVERSE the graph to compute it.                    */
        /***********************************************************/
        if (la_index[GOTO_LAPTR(go_to, i)] == OMEGA)
        {
            int stack_top = 0;
            la_traverse(s -> value, i, &stack_top);
        }
        SET_UNION(look_ahead, 0, la_set, GOTO_LAPTR(go_to, i));
    }

    RESET_BIT(look_ahead, empty); /* empty not valid look-ahead */
    free_nodes(v, r);

    return;
}


/**********************************************************************/
/*                            BUILD_IN_STAT:                          */
/**********************************************************************/
/* We construct the IN_STAT map which is the inverse of the transition*/
/* map formed by GOTO and SHIFT maps.                                 */
/* This map is implemented as a table of pointers that can be indexed */
/* by the states to a circular list of integers representing other    */
/* states that contain transitions to the state in question.          */
/**********************************************************************/
static void build_in_stat(void)
{
    struct shift_header_type sh;
    struct goto_header_type  go_to;
    struct node *q;
    int state_no,
        i,
        n;

    for ALL_STATES(state_no)
    {
        n = statset[state_no].shift_number;
        sh = shift[n];
        for (i = 1; i <= sh.size; ++i)
        {
            n = SHIFT_ACTION(sh, i);
            if (n > 0 && n <= num_states) /* A shift action? */
            {
                q = Allocate_node();
                q -> value = state_no;
                if (in_stat[n] == NULL)
                    q -> next = q;
                else
                {
                    q -> next = in_stat[n] -> next;
                    in_stat[n] -> next = q;
                }
                in_stat[n] = q;
            }
        }

        go_to = statset[state_no].go_to;
        for (i = 1; i <= go_to.size; i++)
        {
            n = GOTO_ACTION(go_to, i);
            if (n > 0) /* A goto action */
            {
                q = Allocate_node();
                q -> value = state_no;
                if (in_stat[n] == NULL)
                    q -> next = q;
                else
                {
                    q -> next = in_stat[n] -> next;
                    in_stat[n] -> next = q;
                }
                in_stat[n] = q;
            }
        }
    }

    return;
}


/*****************************************************************************/
/*                                MKRDCTS:                                   */
/*****************************************************************************/
/* MKRDCTS constructs the REDUCE map and detects conflicts in the grammar.   */
/* When constructing an LALR parser, the subroutine COMPUTE_LA is invoked to */
/* compute the lalr look-ahead sets.  For an SLR parser, the FOLLOW map      */
/* computed earlier in the procedure MKFIRST is used.                        */
/*                                                                           */
/* For a complete description of the lookahead algorithm used in this        */
/* program, see Charles, PhD thesis, NYU 1991.                               */
/*****************************************************************************/
void mkrdcts(void)
{
    short *symbol_list,
          *rule_count;

    int symbol_root,
        reduce_size,
        state_no,
        item_no,
        rule_no,
        symbol,
        default_rule,
        i,
        n;

    struct node **action;

    struct node *p,
                *q,
                *r,
                *item_ptr;

    BOOLEAN *no_shift_on_error_sym;
    SET_PTR look_ahead;

/**********************************************************************/
/* Set up a pool of temporary space. If LALR(k), k > 1 is requested,  */
/* INIT_LALRK_PROCESS sets up the necessary environment for the       */
/* computation of multiple lookahead.                                 */
/**********************************************************************/
    reset_temporary_space();

    init_lalrk_process();

/**********************************************************************/
/* IN_STAT is used to construct a reverse transition map. See         */
/* BUILD_IN_STAT for more detail.                                     */
/*                                                                    */
/* RULE_COUNT is an array used to count the number of reductions on   */
/* particular rules within a given state.                             */
/*                                                                    */
/* NO_SHIFT_ON_ERROR_SYM is a vector used to identify states that     */
/* contain shift actions on the %ERROR symbol.  Such states are marked*/
/* only when DEFAULT_OPT is 5.                                        */
/*                                                                    */
/* SYMBOL_LIST is used to construct temporary lists of terminals on   */
/* which reductions are defined.                                      */
/*                                                                    */
/* When default actions are requested, the vector SINGLE_COMPLETE_ITEM*/
/* is used to identify states that contain exactly one final item.    */
/* NOTE that when the READ_REDUCE options is turned on, the LR(0)     */
/* automaton constructed contains no such state.                      */
/*                                                                    */
/* ACTION is an array that is used as the base for a mapping from     */
/* each terminal symbol into a list of actions that can be executed   */
/* on that symbol in a given state.                                   */
/*                                                                    */
/* LOOK_AHEAD is used to compute lookahead sets.                      */
/**********************************************************************/
    in_stat = (struct node **)
              calloc(num_states + 1, sizeof(struct node *));
    if (in_stat == NULL)
        nospace(__FILE__, __LINE__);

    rule_count = Allocate_short_array(num_rules + 1);
    no_shift_on_error_sym = Allocate_boolean_array(num_states + 1);
    symbol_list = Allocate_short_array(num_terminals + 1);
    single_complete_item = Allocate_boolean_array(num_states + 1);

    action = (struct node **)
             calloc(num_terminals + 1, sizeof(struct node *));
    if (action == NULL)
        nospace(__FILE__, __LINE__);

    look_ahead = (SET_PTR)
                 calloc(1, term_set_size * sizeof(BOOLEAN_CELL));
    if (look_ahead == NULL)
        nospace(__FILE__, __LINE__);

    /****************************************************************/
    /* If we will be removing single productions, we need to keep   */
    /* track of all (state, symbol) pairs on which a conflict is    */
    /* detected. The structure conflict_symbols is used as a base   */
    /* to construct that map. See ADD_CONFLICT_SYMBOL in resolve.c. */
    /* NOTE that this allocation automatically initialized all      */
    /* elements of the conflict_symbols array to NULL.              */
    /****************************************************************/
    if (single_productions_bit)
    {
        conflict_symbols = (struct node **)
                           calloc(num_states + 1, sizeof(struct node *));
        if (conflict_symbols == NULL)
            nospace(__FILE__, __LINE__);
    }

    /**********************************************************************/
    /* First, construct the IN_STAT map. Next, iterate over the states to */
    /* construct two boolean vectors.  One indicates whether there is a   */
    /* shift action on the ERROR symbol when the DEFAULT_OPT is 5.  The   */
    /* other indicates whether it is all right to take default action in  */
    /* states containing exactly one final item.                          */
    /*                                                                    */
    /* We also check whether the grammar is LR(0). I.e., whether it needs */
    /* any look-ahead at all.                                             */
    /**********************************************************************/
    build_in_stat();

    for ALL_STATES(state_no)
    {
        struct shift_header_type sh;

        no_shift_on_error_sym[state_no] = TRUE;

        if (default_opt == 5)
        {
            n = statset[state_no].shift_number;
            sh = shift[n];
            for (i = 1; i <= sh.size; ++i)
            {
                if (SHIFT_SYMBOL(sh, i) == error_image)
                    no_shift_on_error_sym[state_no] = FALSE;
            }
        }

    /**********************************************************************/
    /*   Compute whether this state is a final state.  I.e., a state that */
    /* contains only a single complete item. If so, mark it as a default  */
    /* state. Note that if the READ-REDUCE option is used, the automaton  */
    /* will not contain such states. Also, states are marked only when    */
    /* default actions are requested.                                     */
    /**********************************************************************/
        item_ptr = statset[state_no].kernel_items;
        item_no = item_ptr -> value;
        single_complete_item[state_no] =
               ((! read_reduce_bit) &&
                (! single_productions_bit) &&
                (table_opt != OPTIMIZE_TIME) &&
                (table_opt != OPTIMIZE_SPACE) &&
                (default_opt > 0) &&
                (item_ptr  -> next == NULL) &&
                (item_table[item_no].symbol == empty));

    /**********************************************************************/
    /* If a state has a complete item, and more than one kernel item      */
    /* which is different from the complete item, then this state         */
    /* requires look-ahead for the complete item.                         */
    /**********************************************************************/
        if (highest_level == 0)
        {
            r = statset[state_no].complete_items;
            if (r != NULL)
            {
                if (item_ptr -> next != NULL ||
                    item_ptr -> value != r -> value)
                    highest_level = 1;
            }
        }
    }

    /****************************************************************/
    /* We call COMPUTE_READ to perform the following tasks:         */
    /* 1) Count how many elements are needed in LA_ELEMENT: LA_TOP  */
    /* 2) Allocate space for and initialize LA_SET and LA_INDEX     */
    /****************************************************************/
    if (! slr_bit)
        compute_read();

    /****************************************************************/
    /* Allocate space for REDUCE which will be used to map each     */
    /* into its reduce map. We also initialize RULE_COUNT which     */
    /* will be used to count the number of reduce actions on each   */
    /* rule with in a given state.                                  */
    /****************************************************************/
    reduce = (struct reduce_header_type *)
             calloc(num_states + 1, sizeof(struct reduce_header_type));
    if (reduce == NULL)
        nospace(__FILE__, __LINE__);

    for ALL_RULES(i)
        rule_count[i] = 0;

    /****************************************************************/
    /* We are now ready to construct the reduce map. First, we      */
    /* initialize MAX_LA_STATE to NUM_STATES. If no lookahead       */
    /* state is added (the grammar is LALR(1)) this value will not  */
    /* change. Otherwise, MAX_LA_STATE is incremented by 1 for each */
    /* lookahead state added.                                       */
    /****************************************************************/
    max_la_state = num_states;

    /****************************************************************/
    /* We iterate over the states, compute the lookahead sets,      */
    /* resolve conflicts (if multiple lookahead is requested) and/or*/
    /* report the conflicts if requested...                         */
    /****************************************************************/
    for ALL_STATES(state_no)
    {
        struct reduce_header_type red;

        default_rule = OMEGA;
        symbol_root = NIL;

        item_ptr = statset[state_no].complete_items;
        if (item_ptr != NULL)
        {
    /**********************************************************************/
    /* Check if it is possible to take default reduction. The DEFAULT_OPT */
    /* parameter indicates what kind of default options are requested.    */
    /* The various values it can have are:                                */
    /*                                                                    */
    /*    a)   0 => no default reduction.                                 */
    /*    b)   1 => default reduction only on adequate states. I.e.,      */
    /*              states with only one complete item in their kernel.   */
    /*    c)   2 => Default on all states that contain exactly one        */
    /*              complete item not derived from an empty rule.         */
    /*    d)   3 => Default on all states that contain exactly one        */
    /*              complete item including items from empty rules.       */
    /*    e)   4 => Default reduction on all states that contain exactly  */
    /*              one item. If a state contains more than one item we   */
    /*              take Default on the item that generated the most      */
    /*              reductions. If there is a tie, one is selected at     */
    /*              random.                                               */
    /*    f)   5 => Same as 4 except that no default actions are computed */
    /*              for states that contain a shift action on the ERROR   */
    /*              symbol.                                               */
    /*                                                                    */
    /*  In the code below, we first check for category 3.  If it is not   */
    /* satisfied, then we check for the others. Note that in any case,    */
    /* default reductions are never taken on the ACCEPT rule.             */
    /*                                                                    */
    /**********************************************************************/
            item_no = item_ptr -> value;
            rule_no = item_table[item_no].rule_number;
            symbol = rules[rule_no].lhs;
            if (single_complete_item[state_no] && symbol != accept_image)
            {
                default_rule = rule_no;
                item_ptr = NULL; /* No need to check for conflicts */
            }

        /******************************************************************/
        /* Iterate over all complete items in the state, build action     */
        /* map, and check for conflicts.                                  */
        /******************************************************************/
            for (; item_ptr != NULL; item_ptr = item_ptr -> next)
            { /* for all complete items */
                item_no = item_ptr -> value;
                rule_no = item_table[item_no].rule_number;

                if (slr_bit)  /* SLR table? use Follow */
                {
                    ASSIGN_SET(look_ahead, 0, follow, rules[rule_no].lhs);
                }
                else
                    compute_la(state_no, item_no, look_ahead);

                for ALL_TERMINALS(symbol) /* for all symbols in la set */
                {
                    if (IS_ELEMENT(look_ahead, symbol))
                    {
                        p = Allocate_node();
                        p -> value = item_no;

                        if (action[symbol] == NULL)
                        {
                            symbol_list[symbol] = symbol_root;
                            symbol_root = symbol;
                        }
                        else
                        {
                            /**********************************************/
                            /* Always place the rule with the largest     */
                            /* right-hand side first in the list.         */
                            /**********************************************/
                            n = item_table[action[symbol]
                                                -> value].rule_number;
                            if (RHS_SIZE(n) >= RHS_SIZE(rule_no))
                            {
                                p -> value = action[symbol] -> value;
                                action[symbol] -> value = item_no;
                            }
                        }

                        p -> next = action[symbol];
                        action[symbol] = p;
                    }
                }
            }

        /******************************************************************/
        /* At this stage, we have constructed the ACTION map for STATE_NO.*/
        /* ACTION is a map from each symbol into a set of final items.    */
        /* The rules associated with these items are the rules by which   */
        /* to reduce when the lookahead is the symbol in question.        */
        /* SYMBOL_LIST/SYMBOL_ROOT is a list of the non-empty elements of */
        /* ACTION. If the number of elements in a set ACTION(t), for some */
        /* terminal t, is greater than one or it is not empty and STATE_NO*/
        /* contains a shift action on t then STATE_NO has one or more     */
        /* conflict(s). The procedure RESOLVE_CONFLICTS takes care of     */
        /* resolving the conflicts appropriately and returns an ACTION    */
        /* map where each element has either 0 (if the conflicts were     */
        /* shift-reduce conflicts, the shift is given precedence) or 1    */
        /* element (if the conflicts were reduce-reduce conflicts, only   */
        /* the first element in the ACTION(t) list is returned).          */
        /******************************************************************/
            if (symbol_root != NIL)
            {
                resolve_conflicts(state_no, action,
                                  symbol_list, symbol_root);

                for (symbol = symbol_root;
                     symbol != NIL; symbol = symbol_list[symbol])
                {
                    if (action[symbol] != NULL)
                    {
                        item_no = action[symbol] -> value;
                        rule_count[item_table[item_no].rule_number]++;
                    }
                }
            }
        }

    /*********************************************************************/
    /* We are now ready to compute the size of the reduce map for        */
    /* STATE_NO (reduce_size) and the default rule.                      */
    /* If the state being processed contains only a single complete item */
    /* then the DEFAULT_RULE was previously computed and the list of     */
    /* symbols is empty.                                                 */
    /* NOTE: a REDUCE_ELEMENT will be allocated for all states, even     */
    /* those that have no reductions at all. This will facilitate the    */
    /* Table Compression routines, for they can assume that such an      */
    /* object exists, and can be used for Default values.                */
    /*********************************************************************/
        reduce_size = 0;
        if (symbol_root != NIL)
        {
        /******************************************************************/
        /* Compute REDUCE_SIZE, the number of reductions in the state and */
        /* DEFAULT_RULE: the rule with the highest number of reductions   */
        /* to it.                                                         */
        /******************************************************************/
            n = 0;
            for (q = statset[state_no].complete_items;
                 q != NULL; q = q -> next)
            {
                item_no = q -> value;
                rule_no = item_table[item_no].rule_number;
                symbol = rules[rule_no].lhs;
                reduce_size += rule_count[rule_no];
                if ((rule_count[rule_no] > n) &&
                    (no_shift_on_error_sym[state_no]) &&
                    (symbol != accept_image))
                {
                    n = rule_count[rule_no];
                    default_rule = rule_no;
                }
            }

            /*********************************************************/
            /*   If the removal of single productions is requested   */
            /* and/or parsing tables will not be output, figure out  */
            /* if the level of the default option requested permits  */
            /* default actions, and compute how many reduce actions  */
            /* can be eliminated as a result.                        */
            /*********************************************************/
            if (default_opt == 0)
                default_rule = OMEGA;
            else if (table_opt != OPTIMIZE_TIME &&
                     table_opt != OPTIMIZE_SPACE &&
                     ! single_productions_bit)
            {
                q = statset[state_no].complete_items;
                if (q -> next == NULL)
                {
                    item_no = q -> value;
                    rule_no = item_table[item_no].rule_number;
                    if (default_opt > 2 ||  /* No empty rule defined */
                        (default_opt == 2 && RHS_SIZE(rule_no) != 0))
                        reduce_size -= n;
                    else
                        default_rule = OMEGA;
                }
                else if (default_opt > 3)
                    reduce_size -= n;
            }
            num_reductions += reduce_size;
        }

        /**************************************************************/
        /*   NOTE that the default fields are set for all states,     */
        /* whether or not DEFAULT actions are requested. This is      */
        /* all right since one can always check whether (DEFAULT > 0) */
        /* before using these fields.                                 */
        /**************************************************************/
        red = Allocate_reduce_map(reduce_size);
        reduce[state_no] = red;

        REDUCE_SYMBOL(red, 0)  = DEFAULT_SYMBOL;
        REDUCE_RULE_NO(red, 0) = default_rule;
        for (symbol = symbol_root;
             symbol != NIL; symbol = symbol_list[symbol])
        {
            if (action[symbol] != NULL)
            {
                rule_no = item_table[action[symbol] -> value].rule_number;
                if (rule_no != default_rule ||
                    table_opt == OPTIMIZE_SPACE ||
                    table_opt == OPTIMIZE_TIME  ||
                    single_productions_bit)
                {
                    REDUCE_SYMBOL(red, reduce_size)  = symbol;
                    REDUCE_RULE_NO(red, reduce_size) = rule_no;
                    reduce_size--;
                }

                free_nodes(action[symbol], action[symbol]);
                action[symbol] = NULL;
            }
        }

        /************************************************************/
        /* Reset RULE_COUNT elements used in this state.            */
        /************************************************************/
        for (q = statset[state_no].complete_items;
             q != NULL; q = q -> next)
        {
            rule_no = item_table[q -> value].rule_number;
            rule_count[rule_no] = 0;
        }
    } /* end for ALL_STATES*/

    printf("\n");
    fprintf(syslis,"\n\n");

/****************************************************************/
/* If the automaton required multiple lookahead, construct the  */
/* permanent lookahead states.                                  */
/****************************************************************/
    if (max_la_state > num_states)
        create_lastats();

/****************************************************************/
/* We are now finished with the LALR(k) construction of the     */
/* automaton. Clear all temporary space that was used in that   */
/* process and calculate the maximum lookahead level that was   */
/* needed.                                                      */
/****************************************************************/
    exit_lalrk_process();
    free_conflict_space();

    lalr_level = highest_level;

/****************************************************************/
/* If the removal of single productions is requested, do that.  */
/****************************************************************/
    if (single_productions_bit)
        remove_single_productions();

/****************************************************************/
/* If either more than one lookahead was needed or the removal  */
/* of single productions was requested, the automaton was       */
/* transformed with the addition of new states and new          */
/* transitions. In such a case, we reconstruct the IN_STAT map. */
/****************************************************************/
    if (lalr_level > 1 || single_productions_bit)
    {
        for ALL_STATES(state_no)
        { /* First, clear out the previous map */
            if (in_stat[state_no] != NULL)
            {
                q = in_stat[state_no] -> next; /* point to root */
                free_nodes(q, in_stat[state_no]);
                in_stat[state_no] = NULL;
            }
        }

        build_in_stat(); /* rebuild in_stat map */
    }

/******************************************************************/
/* Print informational messages and free all temporary space that */
/* was used to compute lookahead information.                     */
/******************************************************************/
    if (not_lrk)
    {
        printf("This grammar is not LR(K).\n");
        fprintf(syslis,"This grammar is not LR(K).\n");
    }
    else if (num_rr_conflicts > 0 || num_sr_conflicts > 0)
    {
        if (! slr_bit)
        {
            if (highest_level != INFINITY)
            {
                printf("This grammar is not LALR(%d).\n",
                       highest_level);
                fprintf(syslis,
                        "This grammar is not LALR(%d).\n",
                        highest_level);
            }
            else
            {
                printf("This grammar is not LALR(K).\n");
                fprintf(syslis,"This grammar is not LALR(K).\n");
            }
        }
        else
        {
            printf("This grammar is not SLR(1).\n");
            fprintf(syslis,"This grammar is not SLR(1).\n");
        }
    }
    else
    {
        if (highest_level == 0)
        {
            printf("This grammar is LR(0).\n");
            fprintf(syslis,"This grammar is LR(0).\n");
        }
        else if (slr_bit)
        {
            printf("This grammar is SLR(1).\n");
            fprintf(syslis,"This grammar is SLR(1).\n");
        }
        else
        {
            printf("This grammar is LALR(%d).\n", highest_level);
            fprintf(syslis,
                    "This grammar is LALR(%d).\n", highest_level);
        }
    }

    ffree(rule_count);
    ffree(no_shift_on_error_sym);
    ffree(symbol_list);
    ffree(single_complete_item);
    ffree(action);
    ffree(look_ahead);
    if (conflict_symbols != NULL)
        ffree(conflict_symbols);
    if (read_set != NULL)
        ffree(read_set);
    if (la_index != NULL)
        ffree(la_index);
    if (la_set != NULL)
        ffree(la_set);

    return;
} /* end mkrdcts */
