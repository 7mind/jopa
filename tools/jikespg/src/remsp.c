static char hostfile[] = __FILE__;

#include "common.h"
#include "reduce.h"
#include "header.h"

#define IS_SP_RHS(symbol)   (sp_rules[symbol] != NIL)
#define IS_SP_RULE(rule_no) (rule_list[rule_no] != OMEGA)

static int max_sp_state;

static struct action_element
{
    struct action_element *next;
    short symbol,
          action;
} **new_action;
static struct action_element *action_element_pool = NULL;

static struct update_action_element
{
    struct update_action_element *next;
    short symbol,
          action,
          state;
} **update_action;

static struct sp_state_element
{
    struct sp_state_element *next,
                            *link;
    struct action_element   *action_root;
    struct node             *rule_root,
                            *complete_items;
    short                    state_number,
                             rule_count,
                             action_count;
} **sp_table;
static struct sp_state_element *sp_state_root;

static short *sp_rules,
             *stack,
             *index_of,
             *next_rule,
             *rule_list,

             **sp_action;

static BOOLEAN *is_conflict_symbol;

static SET_PTR look_ahead;

static int top,
           symbol_root,
           rule_root;

/**********************************************************************/
/*                        ALLOCATE_ACTION_ELEMENT:                    */
/**********************************************************************/
/* This function first tries to recycle an action_element node from a */
/* free list. If the list is empty a new node is allocated from       */
/* temporary storage.                                                 */
/**********************************************************************/
static struct action_element* allocate_action_element(void)
{
    struct action_element *p;

    p = action_element_pool;
    if (p != NULL)
        action_element_pool = p -> next;
    else
    {
        p = (struct action_element *)
            talloc(sizeof(struct action_element));
        if (p == (struct action_element *) NULL)
            nospace(__FILE__, __LINE__);
    }

    return p;
}


/**********************************************************************/
/*                          FREE_ACTION_ELEMENTS:                     */
/**********************************************************************/
/* This routine returns a list of action_element structures to the    */
/* free list.                                                         */
/**********************************************************************/
static void free_action_elements(struct action_element *head,
                                 struct action_element *tail)
{
    tail -> next = action_element_pool;
    action_element_pool = head;

    return;
}


/**********************************************************************/
/*                             COMPUTE_SP_MAP:                        */
/**********************************************************************/
/* Compute_sp_map is an instantiation of the digraph algorithm. It    */
/* is invoked repeatedly by remove_single_productions to:             */
/*                                                                    */
/*   1) Partially order the right-hand side of all the single         */
/*      productions (SP) into a list [A1, A2, A3, ..., An]            */
/*      such that if Ai -> Aj then i < j.                             */
/*                                                                    */
/*   2) As a side effect, it uses the ordering above to order all     */
/*      the SP rules.                                                 */
/*                                                                    */
/**********************************************************************/
static void compute_sp_map(int symbol)
{
    int indx;
    int i,
        lhs_symbol,
        rule_no;

    stack[++top] = symbol;
    indx = top;
    index_of[symbol] = indx;

/**********************************************************************/
/* In this instantiation of the digraph algorithm, two symbols (A, B) */
/* are related if  A -> B  is a SP and A is the right-hand side of    */
/* some other SP rule.                                                */
/**********************************************************************/
    for (rule_no = sp_rules[symbol];
         rule_no != NIL; rule_no = next_rule[rule_no])
    {
        lhs_symbol = rules[rule_no].lhs;
        if (IS_SP_RHS(lhs_symbol))
        {
            if (index_of[lhs_symbol] == OMEGA)
                compute_sp_map(lhs_symbol);

            index_of[symbol] = MIN(index_of[symbol],
                                   index_of[lhs_symbol]);
        }
    }

/**********************************************************************/
/* If the index of symbol is the same index it started with then      */
/* symbol if the root of a SCC...                                     */
/**********************************************************************/
    if (index_of[symbol] == indx)
    {
        /**************************************************************/
        /* If symbol is on top of the stack then it is the only       */
        /* symbol in its SCC (thus it is not part of a cycle).        */
        /* Note that since remove_single_productions is only invoked  */
        /* when the input grammar is conflict-free, the graph of      */
        /* the single productions will never contain any cycle.       */
        /* Thus, this test will always succeed and all single         */
        /* productions associated with the symbol being processed     */
        /* are added to the list of SP rules here...                  */
        /**************************************************************/
        if (stack[top] == symbol)
        {
            for (rule_no = sp_rules[symbol];
                 rule_no != NIL; rule_no = next_rule[rule_no])
            {
                if (rule_root == NIL)
                    rule_list[rule_no] = rule_no;
                else
                {
                    rule_list[rule_no] = rule_list[rule_root];
                    rule_list[rule_root] = rule_no;
                }
                rule_root = rule_no;
            }
        }

        /**************************************************************/
        /* As all SCC contains exactly one symbol (as explained above)*/
        /* this loop will always execute exactly once.                */
        /**************************************************************/
        do
        {
            i = stack[top--];
            index_of[i] = INFINITY;
        } while(i != symbol);
    }

    return;
}


/**********************************************************************/
/*                         COMPUTE_SP_ACTION:                         */
/**********************************************************************/
/* When the parser enters STATE_NO and it is processing SYMBOL, its   */
/* next move is ACTION. Given these 3 parameters, compute_sp_action   */
/* computes the set of reduce actions that may be executed after      */
/* SYMBOL is shifted in STATE_NO.                                     */
/*                                                                    */
/* NOTE that this algorithm works only for the LALR(1) case. When the */
/* transition on SYMBOL is a lookahead-shift, indicating that the     */
/* parser requires extra lookahead on a particular symbol, the set of */
/* reduce actions for that symbol is calculated as the empty set.     */
/**********************************************************************/
static void compute_sp_action(short state_no, short symbol, short action)
{
    struct goto_header_type go_to;

    struct node *item_ptr;
    int item_no,
        rule_no,
        lhs_symbol,
        i,
        k;

    BOOLEAN end_node;

    struct node *p;

    go_to = statset[state_no].go_to;

    if (sp_action[symbol] == NULL)
        sp_action[symbol] = Allocate_short_array(num_terminals + 1);

    for ALL_TERMINALS(i) /* initialize sp_action to the empty map */
        sp_action[symbol][i] = OMEGA;

/**********************************************************************/
/* Note that before this routine is invoked, the global vector        */
/* index_of identifies the index of each symbol in the goto map of    */
/* state_no.                                                          */
/**********************************************************************/
    if (is_conflict_symbol[symbol])             /* do nothing */
        ;
    else if (action > 0) /* transition action (shift or goto) */
    {
        for (item_ptr = statset[action].complete_items;
             item_ptr != NULL; item_ptr = item_ptr -> next)
        {
            item_no = item_ptr -> value;
            rule_no = item_table[item_no].rule_number;
            lhs_symbol = rules[rule_no].lhs;
            if (RHS_SIZE(rule_no) == 1 && lhs_symbol != accept_image)
            {
                if (slr_bit)
                {
                    ASSIGN_SET(look_ahead, 0, follow, lhs_symbol);
                }
                else
                {
                    i = index_of[lhs_symbol];
                    k = GOTO_LAPTR(go_to, i);
                    if (la_index[k] == OMEGA)
                    {
                        int stack_top = 0;
                        la_traverse(state_no, i, &stack_top);
                    }
                    ASSIGN_SET(look_ahead, 0, la_set, k);
                }
                RESET_BIT(look_ahead, empty); /* empty not valid look-ahead */

                for ALL_TERMINALS(i)
                {
                    if (IS_ELEMENT(look_ahead, i))
                        sp_action[symbol][i] = rule_no;
                }
            }
        }

        /**************************************************************/
        /* Remove all lookahead symbols on which conflicts were       */
        /* detected from consideration.                               */
        /**************************************************************/
        for (end_node = ((p = conflict_symbols[action]) == NULL);
             ! end_node; end_node = (p == conflict_symbols[action]))
        {
            p = p -> next;
            sp_action[symbol][p -> value] = OMEGA;
        }
    }
    else /* read-reduce action */
    {
        rule_no = -action;
        if (RHS_SIZE(rule_no) == 1)
        {
            lhs_symbol = rules[rule_no].lhs;

            if (slr_bit)
            {
                ASSIGN_SET(look_ahead, 0, follow, lhs_symbol);
            }
            else
            {
                i = index_of[lhs_symbol];
                k = GOTO_LAPTR(go_to, i);
                if (la_index[k] == OMEGA)
                {
                    int stack_top = 0;
                    la_traverse(state_no, i, &stack_top);
                }
                ASSIGN_SET(look_ahead, 0, la_set, k);
            }
            RESET_BIT(look_ahead, empty); /* empty not valid look-ahead */

            for ALL_TERMINALS(i)
            {
                if (IS_ELEMENT(look_ahead, i))
                    sp_action[symbol][i] = rule_no;
            }
        }
    }

    return;
}


/**********************************************************************/
/*                           SP_DEFAULT_ACTION:                       */
/**********************************************************************/
/* Sp_default_action takes as parameter a state, state_no and a rule, */
/* rule_no that may be reduce when the parser enters state_no.        */
/* Sp_default_action tries to determine the highest rule that may be  */
/* reached via a sequence of SP reductions.                           */
/**********************************************************************/
static short sp_default_action(short state_no, short rule_no)
{
    struct reduce_header_type red;
    struct goto_header_type go_to;

    int lhs_symbol,
        action,
        i;

    go_to = statset[state_no].go_to;

/**********************************************************************/
/* While the rule we have at hand is a single production, ...         */
/**********************************************************************/
    while (IS_SP_RULE(rule_no))
    {
        lhs_symbol = rules[rule_no].lhs;
        for (i = 1; GOTO_SYMBOL(go_to, i) != lhs_symbol; i++)
            ;

        action = GOTO_ACTION(go_to, i);
        if (action < 0) /* goto-reduce action? */
        {
            action = -action;
            if (RHS_SIZE(action) != 1)
                break;

            rule_no = action;
        }
        else
        {
            int best_rule = OMEGA;

        /**************************************************************/
        /* Enter the state action and look for preferably a SP rule   */
        /* or some rule with right-hand size 1.                       */
        /**************************************************************/
            red = reduce[action];
            for (i = 1; i <= red.size; i++)
            {
                action = REDUCE_RULE_NO(red, i);
                if (IS_SP_RULE(action))
                {
                    best_rule = action;
                    break;
                }
                if (RHS_SIZE(action) == 1)
                    best_rule = action;
            }
            if (best_rule == OMEGA)
                break;

            rule_no = best_rule;
        }
    }

    return rule_no;
}

/**********************************************************************/
/*                              SP_NT_ACTION:                         */
/**********************************************************************/
/* This routine takes as parameter a state, state_no, a nonterminal,  */
/* lhs_symbol (that is the right-hand side of a SP or a rule with     */
/* right-hand size 1, but not identified as a SP) on which there is   */
/* a transition in state_no and a lookahead symbol la_symbol that may */
/* be processed after taking the transition. It returns the reduce    */
/* action that follows the transition if an action on la_symbol is    */
/* found, otherwise it returns the most suitable default action.      */
/**********************************************************************/
static short sp_nt_action(short state_no, short lhs_symbol, short la_symbol)
{
    struct reduce_header_type red;
    struct goto_header_type go_to;

    int action,
        rule_no,
        i;

    go_to = statset[state_no].go_to;
    for (i = 1; GOTO_SYMBOL(go_to, i) != lhs_symbol; i++)
        ;

    action = GOTO_ACTION(go_to, i);
    if (action < 0)
        action = -action;
    else
    {
        red = reduce[action];
        action = OMEGA;
        for (i = 1; i <= red.size; i++)
        {
            rule_no = REDUCE_RULE_NO(red, i);
            if (REDUCE_SYMBOL(red, i) == la_symbol)
            {
                action = rule_no;
                break;
            }
            else if (action == OMEGA && IS_SP_RULE(rule_no))
                action = rule_no;
        }
    }

    return action;
}


/**********************************************************************/
/*                       GREATEST_COMMON_ANCESTOR:                    */
/**********************************************************************/
/* Let BASE_RULE be a rule  A -> X.  The item [A -> .X] is in STATE1  */
/* and STATE2.  After shifting on X (in STATE1 and STATE2), if the    */
/* lookahead is LA_SYMBOL then BASE_RULE is reduced. In STATE1, a     */
/* sequence of single-production reductions is executed ending with   */
/* a reduction of RULE1. In STATE2, a sequence of single-productions  */
/* is also executed ending with RULE2.                                */
/* The goal of this function is to find the greatest ancestor of      */
/* BASE_RULE that is also a descendant of both RULE1 and RULE2.       */
/**********************************************************************/
static short greatest_common_ancestor(short base_rule, short la_symbol,
                                      short state1, short rule1,
                                      short state2, short rule2)
{
    int lhs_symbol,
        act1,
        act2,
        rule_no;

    act1 = base_rule;
    act2 = base_rule;
    while (act1 == act2)
    {
        rule_no = act1;
        if (act1 == rule1 || act2 == rule2)
            break;

        lhs_symbol = rules[rule_no].lhs;

        act1 = sp_nt_action(state1, lhs_symbol, la_symbol);
        act2 = sp_nt_action(state2, lhs_symbol, la_symbol);
    }

    return rule_no;
}


/**********************************************************************/
/*                       COMPUTE_UPDATE_ACTION:                       */
/**********************************************************************/
/* In SOURCE_STATE there is a transition on SYMBOL into STATE_NO.     */
/* SYMBOL is the right-hand side of a SP rule and the global map      */
/* sp_action[SYMBOL] yields a set of update reduce actions that may   */
/* follow the transition on SYMBOL into STATE_NO.                     */
/**********************************************************************/
static void compute_update_actions(short source_state,
                                   short state_no, short symbol)
{
    int i,
        rule_no;

    struct reduce_header_type red;
    struct update_action_element *p;

    red = reduce[state_no];

    for (i = 1; i <= red.size; i++)
    {
        if (IS_SP_RULE(REDUCE_RULE_NO(red, i)))
        {
            rule_no = sp_action[symbol][REDUCE_SYMBOL(red, i)];
            if (rule_no == OMEGA)
                rule_no = sp_default_action(source_state,
                                            REDUCE_RULE_NO(red, i));

        /**************************************************************/
        /* Lookup the update map to see if a previous update was made */
        /* in STATE_NO on SYMBOL...                                   */
        /**************************************************************/
            for (p = update_action[state_no]; p != NULL; p = p -> next)
            {
                if (p -> symbol == REDUCE_SYMBOL(red, i))
                    break;
            }

        /**************************************************************/
        /* If no previous update action was defined on STATE_NO and   */
        /* SYMBOL, simply add it. Otherwise, chose as the greatest    */
        /* common ancestor of the initial reduce action and the two   */
        /* contending updates as the update action.                   */
        /**************************************************************/
            if (p == NULL)
            {
                p = (struct update_action_element *)
                    talloc(sizeof(struct update_action_element));
                if (p == (struct update_action_element *) NULL)
                    nospace(__FILE__, __LINE__);
                p -> next = update_action[state_no];
                update_action[state_no] = p;

                p -> symbol = REDUCE_SYMBOL(red, i);
                p -> action = rule_no;
                p -> state  = source_state;
            }
            else if ((rule_no != p -> action) &&
                     (p -> action != REDUCE_RULE_NO(red, i)))
            {
                p -> action = greatest_common_ancestor(REDUCE_RULE_NO(red, i),
                                                       REDUCE_SYMBOL(red, i),
                                                       source_state, rule_no,
                                                       p -> state,
                                                       p -> action);
            }
        }
    }

    return;
}


/**********************************************************************/
/*                             SP_STATE_MAP:                          */
/**********************************************************************/
/* Sp_state_map is invoked to create a new state using the reduce map */
/* sp_symbol[SYMBOL]. The new state will be entered via a transition  */
/* on SYMBOL which is the right-hand side of the SP rule of which     */
/* ITEM_NO is the final item.                                         */
/*                                                                    */
/* RULE_HEAD is the root of a list of rules in the global vector      */
/* next_rule.  This list of rules identifies the range of the reduce  */
/* map sp_symbol[SYMBOL]. The value SP_RULE_COUNT is the number of    */
/* rules in the list. The value SP_ACTION_COUNT is the number of      */
/* actions in the map sp_symbol[SYMBOL].                              */
/**********************************************************************/
static short sp_state_map(int rule_head, short item_no,
                          short sp_rule_count,
                          short sp_action_count, short symbol)
{
    struct sp_state_element *state;
    struct node             *p;

    int rule_no,
        i;

    BOOLEAN no_overwrite;

    unsigned long hash_address;

    /******************************************************************/
    /* These new SP states are defined by their reduce maps. Hash the */
    /* reduce map based on the set of rules in its range - simply add */
    /* them up and reduce modulo STATE_TABLE_SIZE.                    */
    /******************************************************************/
    hash_address = 0;
    for (rule_no = rule_head;
         rule_no != NIL; rule_no = next_rule[rule_no])
        hash_address += rule_no;

    hash_address %= STATE_TABLE_SIZE;

    /******************************************************************/
    /* Search the hash table for a compatible state. Two states S1    */
    /* and S2 are compatible if                                       */
    /*     1) the set of rules in their reduce map is identical.      */
    /*     2) for each terminal symbol t, either                      */
    /*            reduce[S1][t] == reduce[S2][t] or                   */
    /*            reduce[S1][t] == OMEGA         or                   */
    /*            reduce[S2][t] == OMEGA                              */
    /*                                                                */
    /******************************************************************/
    for (state = sp_table[hash_address];
         state != NULL; state = state -> link)
    {
        if (state -> rule_count == sp_rule_count) /* same # of rules? */
        {
            for (p = state -> rule_root; p != NULL; p = p -> next)
            {
                if (next_rule[p -> value] == OMEGA)   /* not in list? */
                    break;
            }

    /******************************************************************/
    /* If the set of rules are identical, we proceed to compare the   */
    /* actions for compatibility. The idea is to make sure that all   */
    /* actions in the hash table do not clash in the actions in the   */
    /* map sp_action[SYMBOL].                                         */
    /******************************************************************/
            if (p == NULL) /* all the rules match? */
            {
                struct action_element *actionp,
                                      *action_tail;

                for (actionp = state -> action_root;
                     actionp != NULL; actionp = actionp -> next)
                {
                    if (sp_action[symbol][actionp -> symbol] != OMEGA &&
                        sp_action[symbol][actionp -> symbol] !=
                                                     actionp -> action)
                         break;
                }

        /**************************************************************/
        /* If the two states are compatible merge them into the map   */
        /* sp_action[SYMBOL]. (Note that this effectively destroys    */
        /* the original map.) Also, keep track of whether or not an   */
        /* actual merging action was necessary with the boolean       */
        /* variable no_overwrite.                                     */
        /**************************************************************/
                if (actionp == NULL) /* compatible states */
                {
                    no_overwrite = TRUE;
                    for (actionp = state -> action_root;
                         actionp != NULL;
                         action_tail = actionp, actionp = actionp -> next)
                    {
                        if (sp_action[symbol][actionp -> symbol] == OMEGA)
                        {
                            sp_action[symbol][actionp -> symbol] =
                                                         actionp -> action;
                            no_overwrite = FALSE;
                        }
                    }

            /**********************************************************/
            /* If the item was not previously associated with this    */
            /* state, add it.                                         */
            /**********************************************************/
                    for (p = state -> complete_items;
                         p != NULL; p = p -> next)
                    {
                        if (p -> value == item_no)
                            break;
                    }
                    if (p == NULL)
                    {
                        p = Allocate_node();
                        p -> value = item_no;
                        p -> next = state -> complete_items;
                        state -> complete_items = p;
                    }

            /**********************************************************/
            /* If the two maps are identical (there was no merging),  */
            /* return the state number otherwise, free the old map    */
            /* and break out of the search loop.                      */
            /**********************************************************/
                    if (no_overwrite &&
                        state -> action_count == sp_action_count)
                        return state -> state_number;

                    free_action_elements(state -> action_root, action_tail);

                    break; /* for (state = sp_table[hash_address]; ... */
                }
            }
        }
    }

    /******************************************************************/
    /* If we did not find a compatible state, construct a new one.    */
    /* Add it to the list of state and add it to the hash table.      */
    /******************************************************************/
    if (state == NULL)
    {
        state = (struct sp_state_element *)
                talloc(sizeof(struct sp_state_element));
        if (state == NULL)
            nospace(__FILE__, __LINE__);

        state -> next = sp_state_root;
        sp_state_root = state;

        state -> link = sp_table[hash_address];
        sp_table[hash_address] = state;

        max_sp_state++;
        state -> state_number = max_sp_state;
        state -> rule_count = sp_rule_count;

        p = Allocate_node();
        p -> value = item_no;
        p -> next = NULL;
        state -> complete_items = p;

        state -> rule_root = NULL;
        for (rule_no = rule_head;
             rule_no != NIL; rule_no = next_rule[rule_no])
        {
            p = Allocate_node();
            p -> value = rule_no;
            p -> next = state -> rule_root;
            state -> rule_root = p;
        }
    }

    /******************************************************************/
    /* If the state is new or had its reduce map merged with another  */
    /* map, we update the reduce map here.                            */
    /******************************************************************/
    state -> action_count = sp_action_count;
    state -> action_root = NULL;
    for ALL_TERMINALS(i)
    {
        struct action_element *actionp;

        if (sp_action[symbol][i] != OMEGA)
        {
            actionp = allocate_action_element();
            actionp -> symbol = i;
            actionp -> action = sp_action[symbol][i];

            actionp -> next = state -> action_root;
            state -> action_root = actionp;
        }
    }

    return state -> state_number;
}


/*****************************************************************************/
/*                       REMOVE_SINGLE_PRODUCTIONS:                          */
/*****************************************************************************/
/* This program is invoked to remove as many single production actions as    */
/* possible for a conflict-free automaton.                                   */
/*****************************************************************************/
void remove_single_productions(void)
{
    struct goto_header_type go_to;
    struct shift_header_type sh;
    struct reduce_header_type red;

    struct sp_state_element *state;
    struct node             *p,
                            *rule_tail;

    int rule_head,
        sp_rule_count,
        sp_action_count,
        rule_no,
        state_no,
        symbol,
        lhs_symbol,
        action,
        item_no,
        i,
        j;

    BOOLEAN end_node;

    short *symbol_list,
          *shift_transition,
          *rule_count;

    short shift_table[SHIFT_TABLE_SIZE];

    struct new_shift_element
    {
        short link,
              shift_number;
    } *new_shift;

    /******************************************************************/
    /* Set up a a pool of temporary space.                            */
    /******************************************************************/
    reset_temporary_space();

    /******************************************************************/
    /* Allocate all other necessary temporary objects.                */
    /******************************************************************/
    sp_rules = Allocate_short_array(num_symbols + 1);
    stack = Allocate_short_array(num_symbols + 1);
    index_of = Allocate_short_array(num_symbols + 1);
    next_rule = Allocate_short_array(num_rules + 1);
    rule_list = Allocate_short_array(num_rules + 1);
    symbol_list = Allocate_short_array(num_symbols + 1);
    shift_transition = Allocate_short_array(num_symbols + 1);
    rule_count = Allocate_short_array(num_rules + 1);

    new_shift = (struct new_shift_element *)
                calloc(max_la_state + 1,
                       sizeof(struct new_shift_element));
    if (new_shift == NULL)
        nospace(__FILE__, __LINE__);

    look_ahead = (SET_PTR)
                 calloc(1, term_set_size * sizeof(BOOLEAN_CELL));
    if (look_ahead == NULL)
        nospace(__FILE__, __LINE__);

    sp_action = (short **)
                calloc(num_symbols + 1, sizeof(short *));
    if (sp_action == NULL)
        nospace(__FILE__, __LINE__);

    is_conflict_symbol = (BOOLEAN *)
                         calloc(num_symbols + 1, sizeof(BOOLEAN));
    if (is_conflict_symbol == NULL)
        nospace(__FILE__, __LINE__);

    sp_table = (struct sp_state_element **)
               calloc(STATE_TABLE_SIZE,
                      sizeof(struct sp_state_element *));
    if (sp_table == NULL)
        nospace(__FILE__, __LINE__);

    new_action = (struct action_element **)
                 calloc(num_states + 1, sizeof(struct action_element *));
    if (new_action == NULL)
        nospace(__FILE__, __LINE__);

    update_action = (struct update_action_element **)
                    calloc(num_states + 1,
                           sizeof(struct update_action_element *));
    if (update_action == NULL)
        nospace(__FILE__, __LINE__);

    /******************************************************************/
    /* Initialize all relevant sets and maps to the empty set.        */
    /******************************************************************/
    rule_root = NIL;
    symbol_root = NIL;

    for ALL_RULES(i)
        rule_list[i] = OMEGA;

    for ALL_SYMBOLS(i)
    {
        symbol_list[i] = OMEGA;
        sp_rules[i]    = NIL;
        sp_action[i]   = NULL;
    }

    /******************************************************************/
    /* Construct a set of all symbols used in the right-hand side of  */
    /* single production in symbol_list. The variable symbol_root     */
    /* points to the root of the list. Also, construct a mapping from */
    /* each symbol into the set of single productions of which it is  */
    /* the right-hand side. sp_rules is the base of that map and the  */
    /* relevant sets are stored in the vector next_rule.              */
    /******************************************************************/
    for ALL_RULES(rule_no)
    {
        if (rules[rule_no].sp)
        {
            i = rhs_sym[rules[rule_no].rhs];
            next_rule[rule_no] = sp_rules[i];
            sp_rules[i] = rule_no;
            if (symbol_list[i] == OMEGA)
            {
                symbol_list[i] = symbol_root;
                symbol_root = i;
            }
        }
    }

    /******************************************************************/
    /* Initialize the index_of vector and clear the stack (top).      */
    /* Next, iterate over the list of right-hand side symbols used in */
    /* single productions and invoke compute_sp_map to partially      */
    /* order these symbols (based on the ::= (or ->) relationship) as */
    /* well as their associated rules. (See compute_sp_map for detail)*/
    /* As the list of rules is constructed as a circular list to keep */
    /* it in proper order, it is turned into a linear list here.      */
    /******************************************************************/
    for (i = symbol_root; i != NIL; i = symbol_list[i])
        index_of[i] = OMEGA;
    top = 0;
    for (i = symbol_root; i != NIL; i = symbol_list[i])
    {
        if (index_of[i] == OMEGA)
            compute_sp_map(i);
    }

    if (rule_root != NIL) /* make rule_list non-circular */
    {
        rule_no = rule_root;
        rule_root = rule_list[rule_no];
        rule_list[rule_no] = NIL;
    }

    /******************************************************************/
    /* Clear out all the sets in sp_rules and using the new revised   */
    /* list of SP rules mark the new set of right-hand side symbols.  */
    /* Note this code is important for consistency in case we are     */
    /* removing single productions in an automaton containing         */
    /* conflicts. If an automaton does not contain any conflict, the  */
    /* new set of SP rules is always the same as the initial set.     */
    /******************************************************************/
    for (i = symbol_root; i != NIL; i = symbol_list[i])
        sp_rules[i] = NIL;

    top = 0;
    for (rule_no = rule_root;
         rule_no != NIL; rule_no = rule_list[rule_no])
    {
        top++;

        i = rhs_sym[rules[rule_no].rhs];
        sp_rules[i] = OMEGA;
    }

    /******************************************************************/
    /* Initialize the base of the hash table for the new SP states.   */
    /* Initialize the root pointer for the list of new states.        */
    /* Initialize max_sp_state to num_states. It will be incremented  */
    /* each time a new state is constructed.                          */
    /* Initialize the update_action table.                            */
    /* Initialize the is_conflict_symbol array for non_terminals.     */
    /* Since nonterminals are not used as lookahead symbols, they are */
    /* never involved in conflicts.                                   */
    /* Initialize the set/list (symbol_root, symbol_list) to the      */
    /* empty set/list.                                                */
    /******************************************************************/
    for (i = 0; i < STATE_TABLE_SIZE; i++)
        sp_table[i] = NULL;

    sp_state_root = NULL;
    max_sp_state = num_states;

    for ALL_STATES(state_no)
        update_action[state_no] = NULL;

    for ALL_NON_TERMINALS(i)
        is_conflict_symbol[i] = FALSE;

    symbol_root = NIL;
    for ALL_SYMBOLS(i)
        symbol_list[i] = OMEGA;

    /******************************************************************/
    /* Traverse all regular states and process the relevant ones.     */
    /******************************************************************/
    for ALL_STATES(state_no)
    {
        struct node *item_ptr;

        new_action[state_no] = NULL;

        go_to = statset[state_no].go_to;
        sh = shift[statset[state_no].shift_number];

        /**************************************************************/
        /* If the state has no goto actions, it is not considered, as */
        /* no single productions could have been introduced in it.    */
        /* Otherwise, we initialize index_of to the empty map and     */
        /* presume that symbol_list is initialized to the empty set.  */
        /**************************************************************/
        if (go_to.size > 0)
        {
            for ALL_SYMBOLS(i)
                index_of[i] = OMEGA;

            for ALL_TERMINALS(i)
                is_conflict_symbol[i] = FALSE;
            for (end_node = ((p = conflict_symbols[state_no]) == NULL);
                 ! end_node; end_node = (p == conflict_symbols[state_no]))
            {
                p = p -> next;
                is_conflict_symbol[p -> value] = TRUE;
            }

            /**********************************************************/
            /* First, use index_of to map each nonterminal symbol on  */
            /* which there is a transition in state_no into its index */
            /* in the goto map of state_no.                           */
            /* Note that this initialization must be executed first   */
            /* before we process the next loop, because index_of is   */
            /* a global variable that is used in the routine          */
            /* compute_sp_action.                                     */
            /**********************************************************/
            for (i = 1; i <= go_to.size; i++)
                index_of[GOTO_SYMBOL(go_to, i)] = i;

            /**********************************************************/
            /* Traverse first the goto map, then the shift map and    */
            /* for each symbol that is the right-hand side of a single*/
            /* production on which there is a transition, compute the */
            /* lookahead set that can follow this transition and add  */
            /* the symbol to the set of candidates (in symbol_list).  */
            /**********************************************************/
            for (i = 1; i <= go_to.size; i++)
            {
                symbol = GOTO_SYMBOL(go_to, i);
                if (IS_SP_RHS(symbol))
                {
                    compute_sp_action(state_no, symbol,
                                      GOTO_ACTION(go_to, i));
                    symbol_list[symbol] = symbol_root;
                    symbol_root = symbol;
                }
            }

            for (i = 1; i <= sh.size; i++)
            {
                symbol = SHIFT_SYMBOL(sh, i);
                index_of[symbol] = i;
                if (IS_SP_RHS(symbol))
                {
                    compute_sp_action(state_no, symbol,
                                      SHIFT_ACTION(sh, i));
                    symbol_list[symbol] = symbol_root;
                    symbol_root = symbol;
                }
            }

            /**********************************************************/
            /* We now traverse the set of single productions in order */
            /* and for each rule that was introduced through closure  */
            /* in the state (there is an action on both the left- and */
            /* right-hand side)...                                    */
            /**********************************************************/
            for (rule_no = rule_root;
                 rule_no != NIL; rule_no = rule_list[rule_no])
            {
                symbol = rhs_sym[rules[rule_no].rhs];
                if (symbol_list[symbol] != OMEGA)
                {
                    lhs_symbol = rules[rule_no].lhs;
                    if (index_of[lhs_symbol] != OMEGA)
                    {
                        if (symbol_list[lhs_symbol] == OMEGA)
                        {
                            compute_sp_action(state_no, lhs_symbol,
                                GOTO_ACTION(go_to, index_of[lhs_symbol]));
                            symbol_list[lhs_symbol] = symbol_root;
                            symbol_root = lhs_symbol;
                        }
                /******************************************************/
                /* Copy all reduce actions defined after the          */
                /* transition on the left-hand side into the          */
                /* corresponding action defined after the transition  */
                /* on the right-hand side. If an action is defined    */
                /* for the left-hand side -                           */
                /*                                                    */
                /*     sp_action[lhs_symbol][i] != OMEGA              */
                /*                                                    */
                /* - but not for the right-hand side -                */
                /*                                                    */
                /*     sp_action[symbol][i] == OMEGA                  */
                /*                                                    */
                /* it is an indication that after the transition on   */
                /* symbol, the action on i is a lookahead shift. In   */
                /* that case, no action is copied.                    */
                /******************************************************/
                        for ALL_TERMINALS(i)
                        {
                            if (sp_action[lhs_symbol][i] != OMEGA &&
                                sp_action[symbol][i] != OMEGA)
                                    sp_action[symbol][i] =
                                        sp_action[lhs_symbol][i];
                        }
                    }
                }
            }

            /**********************************************************/
            /* For each symbol that is the right-hand side of some SP */
            /* for which a reduce map is defined, we either construct */
            /* a new state if the transition is into a final state,   */
            /* or we update the relevant reduce action of the state   */
            /* into which the transition is made, otherwise.          */
            /*                                                        */
            /* When execution of this loop is terminated the set      */
            /* symbol_root/symbol_list is reinitialize to the empty   */
            /* set.                                                   */
            /**********************************************************/
            for (symbol = symbol_root;
                 symbol != NIL;
                 symbol_list[symbol] = OMEGA, symbol = symbol_root)
            {
                symbol_root = symbol_list[symbol];

                if (IS_SP_RHS(symbol))
                {
                    if (symbol IS_A_TERMINAL)
                         action = SHIFT_ACTION(sh, index_of[symbol]);
                    else action = GOTO_ACTION(go_to, index_of[symbol]);

                /******************************************************/
                /* If the transition is a lookahead shift, do nothing.*/
                /* If the action is a goto- or shift-reduce, compute  */
                /* the relevant rule and item involved.               */
                /* Otherwise, the action is a shift or a goto. If the */
                /* transition is into a final state then it is        */
                /* processed as the case of read-reduce above. If     */
                /* not, we invoke compute_update_actions to update    */
                /* the relevant actions.                              */
                /******************************************************/
                    if (action > num_states) /* lookahead shift */
                        rule_no = OMEGA;
                    else if (action < 0) /* read-reduce action */
                    {
                        rule_no = -action;
                        item_no = adequate_item[rule_no] -> value;
                    }
                    else                /* transition action  */
                    {
                        item_ptr = statset[action].kernel_items;
                        item_no = item_ptr -> value;
                        if ((item_ptr -> next == NULL) &&
                            (item_table[item_no].symbol == empty))
                             rule_no = item_table[item_no].rule_number;
                        else
                        {
                            compute_update_actions(state_no, action, symbol);
                            rule_no = OMEGA;
                        }
                    }

                /******************************************************/
                /* If we have a valid SP rule we first construct the  */
                /* set of rules in the range of the reduce map of the */
                /* right-hand side of the rule. If that set contains  */
                /* a single rule then the action on the right-hand    */
                /* side is redefined as the same action on the left-  */
                /* hand side of the rule in question. Otherwise, we   */
                /* create a new state for the final item of the SP    */
                /* rule consisting of the reduce map associated with  */
                /* the right-hand side of the SP rule and the new     */
                /* action on the right-hand side is a transition into */
                /* this new state.                                    */
                /******************************************************/
                    if (rule_no != OMEGA)
                    {
                        if (IS_SP_RULE(rule_no))
                        {
                            struct action_element *p;

                            sp_rule_count = 0;
                            sp_action_count = 0;
                            rule_head = NIL;

                            for ALL_RULES(i)
                                next_rule[i] = OMEGA;

                            for ALL_TERMINALS(i)
                            {
                                rule_no = sp_action[symbol][i];
                                if (rule_no != OMEGA)
                                {
                                    sp_action_count++;
                                    if (next_rule[rule_no] == OMEGA)
                                    {
                                        sp_rule_count++;
                                        next_rule[rule_no] = rule_head;
                                        rule_head = rule_no;
                                    }
                                }
                            }

                            if (sp_rule_count == 1 && IS_SP_RULE(rule_head))
                            {
                                lhs_symbol = rules[rule_head].lhs;
                                action = GOTO_ACTION(go_to,
                                                index_of[lhs_symbol]);
                            }
                            else
                            {
                                action = sp_state_map(rule_head,
                                                      item_no,
                                                      sp_rule_count,
                                                      sp_action_count,
                                                      symbol);
                            }

                            p = allocate_action_element();
                            p -> symbol = symbol;
                            p -> action = action;

                            p -> next = new_action[state_no];
                            new_action[state_no] = p;
                        }
                    }
                }
            }
        }
    } /* for ALL_STATES(state_no) */

    /******************************************************************/
    /* We are now ready to extend all global maps based on states and */
    /* permanently install the new states.                            */
    /******************************************************************/
    statset = (struct statset_type *)
              realloc(statset,
                      (max_sp_state + 1) * sizeof(struct statset_type));
    if (statset == NULL)
        nospace(__FILE__, __LINE__);

    reduce = (struct reduce_header_type *)
             realloc(reduce,
                     (max_sp_state + 1) * sizeof(struct reduce_header_type));
    if (reduce == NULL)
        nospace(__FILE__, __LINE__);

    if (gd_index != NULL) /* see routine PRODUCE */
    {
        gd_index = (short *)
                   realloc(gd_index, (max_sp_state + 2) * sizeof(short));
        if (gd_index == NULL)
            nospace(__FILE__, __LINE__);

        /**************************************************************/
        /* Each element gd_index[i] points to the starting location   */
        /* of a slice in another array. The last element of the slice */
        /* can be computed as (gd_index[i+1] - 1). After extending    */
        /* gd_index, we set each new element to point to the same     */
        /* index as its previous element, making it point to a null   */
        /* slice.                                                     */
        /**************************************************************/
        for (state_no = num_states + 2;
             state_no <= max_sp_state + 1; state_no++)
             gd_index[state_no] = gd_index[state_no - 1];
    }

    in_stat = (struct node **)
              realloc(in_stat,
                      (max_sp_state + 1) * sizeof(struct node *));
    if (in_stat == NULL)
        nospace(__FILE__, __LINE__);

    for (state_no = num_states + 1; state_no <= max_sp_state; state_no++)
         in_stat[state_no] = NULL;

    /******************************************************************/
    /* We now adjust all references to a lookahead state. The idea is */
    /* offset the number associated with each lookahead state by the  */
    /* number of new SP states that were added.                       */
    /******************************************************************/
    for (j = 1; j <= num_shift_maps; j++)
    {
        sh = shift[j];
        for (i = 1; i <= sh.size; i++)
        {
            if (SHIFT_ACTION(sh, i) > num_states)
                SHIFT_ACTION(sh, i) += (max_sp_state - num_states);
        }
    }

    for (state_no = num_states + 1; state_no <= max_la_state; state_no++)
    {
        if (lastats[state_no].in_state > num_states)
            lastats[state_no].in_state += (max_sp_state - num_states);
    }

    lastats -= (max_sp_state - num_states);
    max_la_state += (max_sp_state - num_states);
    SHORT_CHECK(max_la_state);

    /******************************************************************/
    /* We now permanently construct all the new SP states.            */
    /******************************************************************/
    for (state = sp_state_root; state != NULL; state = state -> next)
    {
        struct action_element *actionp;

        int default_rule,
            reduce_size;

        state_no = state -> state_number;

        /**************************************************************/
        /* These states are identified as special SP states since     */
        /* they have no kernel items. They also have no goto and      */
        /* shift actions.                                             */
        /**************************************************************/
        statset[state_no].kernel_items = NULL;
        statset[state_no].complete_items = state -> complete_items;
        statset[state_no].go_to.size = 0;
        statset[state_no].go_to.map = NULL;
        statset[state_no].shift_number = 0;

        /**************************************************************/
        /* Count the number of actions defined on each rule in the    */
        /* range of the reduce map.                                   */
        /**************************************************************/
        for (p = state -> rule_root; p != NULL; p = p -> next)
            rule_count[p -> value] = 0;

        for (actionp = state -> action_root;
             actionp != NULL; actionp = actionp -> next)
            rule_count[actionp -> action]++;

        /**************************************************************/
        /* Count the total number of reduce actions in the reduce map */
        /* and calculate the default.                                 */
        /**************************************************************/
        reduce_size = 0;
        sp_rule_count = 0;
        for (p = state -> rule_root; p != NULL; rule_tail = p, p = p -> next)
        {
            reduce_size += rule_count[p -> value];
            if (rule_count[p -> value] > sp_rule_count)
            {
                sp_rule_count = rule_count[p -> value];
                default_rule = p -> value;
            }
        }

        free_nodes(state -> rule_root, rule_tail);

        /**************************************************************/
        /* Construct a permanent reduce map for this SP state.        */
        /**************************************************************/
        num_reductions += reduce_size;

        red = Allocate_reduce_map(reduce_size);
        reduce[state_no] = red;
        REDUCE_SYMBOL(red, 0)  = DEFAULT_SYMBOL;
        REDUCE_RULE_NO(red, 0) = default_rule;

        for (actionp = state -> action_root;
             actionp != NULL; actionp = actionp -> next)
        {
            REDUCE_SYMBOL(red, reduce_size)  = actionp -> symbol;
            REDUCE_RULE_NO(red, reduce_size) = actionp -> action;
            reduce_size--;
        }
    }

    /******************************************************************/
    /* We are now ready to update some old actions and add new ones.  */
    /* This may require that we create new shift maps.  We            */
    /* initialize top to 0 so we can use it as an index to allocate   */
    /* elements from new_shift. We also initialize all the elements   */
    /* of shift_table to NIL. Shift_table will be used as the base of */
    /* a hash table for the new shift maps.                           */
    /******************************************************************/
    top = 0;
    for (i = 0; i <= SHIFT_TABLE_UBOUND; i++)
        shift_table[i] = NIL;

    /******************************************************************/
    /* At most, the shift array contains 1..num_states elements. As,  */
    /* each of these elements might be (theoretically) replaced by a  */
    /* new one, we need to double its size.                           */
    /******************************************************************/
    shift = (struct shift_header_type *)
            realloc(shift,
               2 * (num_states + 1) * sizeof(struct shift_header_type));
    if (shift == NULL)
        nospace(__FILE__, __LINE__);

    /******************************************************************/
    /* For each state with updates or new actions, take appropriate   */
    /* actions.                                                       */
    /******************************************************************/
    for ALL_STATES(state_no)
    {
        BOOLEAN any_shift_action;

        /**************************************************************/
        /* Update reduce actions for final items of single productions*/
        /* that are in non-final states.                              */
        /**************************************************************/
        if (update_action[state_no] != NULL)
        {
            struct update_action_element *p;

            red = reduce[state_no];
            for (i = 1; i <= red.size; i++)
                index_of[REDUCE_SYMBOL(red, i)] = i;

            for (p = update_action[state_no]; p != NULL; p = p -> next)
                REDUCE_RULE_NO(red, index_of[p -> symbol]) = p -> action;
        }

        /**************************************************************/
        /* Update initial automaton with transitions into new SP      */
        /* states.                                                    */
        /**************************************************************/
        if (new_action[state_no] != NULL)
        {
            struct action_element *p;

            /**********************************************************/
            /* Mark the index of each symbol on which there is a      */
            /* transition and copy the shift map into the vector      */
            /* shift_transition.                                      */
            /**********************************************************/
            go_to = statset[state_no].go_to;
            for (i = 1; i <= go_to.size; i++)
                index_of[GOTO_SYMBOL(go_to, i)] = i;

            sh = shift[statset[state_no].shift_number];
            for (i = 1; i <= sh.size; i++)
            {
                index_of[SHIFT_SYMBOL(sh, i)] = i;
                shift_transition[SHIFT_SYMBOL(sh, i)] = SHIFT_ACTION(sh, i);
            }

            /**********************************************************/
            /* Iterate over the new action and update the goto map    */
            /* directly for goto actions but update shift_transition  */
            /* for shift actions. Also, keep track as to whether or   */
            /* not there were any shift transitions at all...         */
            /**********************************************************/
            any_shift_action = FALSE;

            for (p = new_action[state_no]; p != NULL; p = p -> next)
            {
                if (p -> symbol IS_A_NON_TERMINAL)
                {
                    if (GOTO_ACTION(go_to, index_of[p -> symbol]) < 0 &&
                        p -> action > 0)
                    {
                        num_goto_reduces--;
                        num_gotos++;
                    }
                    GOTO_ACTION(go_to, index_of[p -> symbol]) = p -> action;
                }
                else
                {
                    if (SHIFT_ACTION(sh, index_of[p -> symbol]) < 0 &&
                        p -> action > 0)
                    {
                        num_shift_reduces--;
                        num_shifts++;
                    }
                    shift_transition[p -> symbol] = p -> action;

                    any_shift_action = TRUE;
                }
            }

            /**********************************************************/
            /* If there were any shift actions, a new shift map may   */
            /* have been created. Hash shift_transition into the      */
            /* shift hash table.                                      */
            /**********************************************************/
            if (any_shift_action)
            {
                struct shift_header_type sh2;
                unsigned long hash_address;

                hash_address = sh.size;
                for (i = 1; i <= sh.size; i++) /* Compute Hash location */
                    hash_address += SHIFT_SYMBOL(sh, i);
                hash_address %= SHIFT_TABLE_SIZE;

            /**************************************************************/
            /* Search HASH_ADDRESS location for shift map that matches    */
            /* the shift map in shift_transition.  If a match is found    */
            /* we leave the loop prematurely, the search index j is not   */
            /* NIL, and it identifies the shift map in the hash table     */
            /* that matched the shift_transition.                         */
            /**************************************************************/
                for (j = shift_table[hash_address];
                     j != NIL; j = new_shift[j].link)
                {
                    sh2 = shift[new_shift[j].shift_number];
                    if (sh.size == sh2.size)
                    {
                        for (i = 1; i <= sh.size; i++)
                            if (SHIFT_ACTION(sh2, i) !=
                                shift_transition[SHIFT_SYMBOL(sh2, i)])
                                    break;
                        if (i > sh.size)
                            break; /* for (j = shift_table[ ... */
                    }
                }

            /**************************************************************/
            /* If j == NIL, the map at hand had not yet being inserted in */
            /* the table, it is inserted.  Otherwise, we have a match,    */
            /* and STATE_NO is reset to share the shift map previously    */
            /* inserted that matches its shift map.                       */
            /**************************************************************/
                if (j == NIL)
                {
                    sh2 = Allocate_shift_map(sh.size);
                    for (i = 1; i <= sh.size; i++)
                    {
                        symbol = SHIFT_SYMBOL(sh, i);
                        SHIFT_SYMBOL(sh2, i) = symbol;
                        SHIFT_ACTION(sh2, i) = shift_transition[symbol];
                    }
                    num_shift_maps++;
                    shift[num_shift_maps] = sh2;

                    statset[state_no].shift_number = num_shift_maps;

                    top++;
                    new_shift[top].shift_number = num_shift_maps;

                    new_shift[top].link = shift_table[hash_address];
                    shift_table[hash_address] = top;
                }
                else
                {
                    statset[state_no].shift_number =
                           new_shift[j].shift_number;
                }
            }
        }
    }

    /******************************************************************/
    /* Free all nodes used in the construction of the conflict_symbols*/
    /* map as this map is no longer useful and its size is based on   */
    /* the base value of num_states.                                  */
    /******************************************************************/
    for ALL_STATES(state_no)
    {
        if (conflict_symbols[state_no] != NULL)
        {
            p = conflict_symbols[state_no] -> next;
            free_nodes(p, conflict_symbols[state_no]);
        }
    }

    /******************************************************************/
    /* All updates have now been made, adjust the number of regular   */
    /* states to include the new SP states.                           */
    /******************************************************************/
    num_states = max_sp_state;

    /******************************************************************/
    /* Free all temporary space allocated earlier.                    */
    /******************************************************************/
    ffree(sp_rules);
    ffree(stack);
    ffree(index_of);
    ffree(next_rule);
    ffree(rule_list);
    ffree(symbol_list);
    ffree(shift_transition);
    ffree(rule_count);
    ffree(new_shift);
    ffree(look_ahead);
    for ALL_SYMBOLS(i)
    {
        if (sp_action[i] != NULL)
        {
            ffree(sp_action[i]);
        }
    }
    ffree(sp_action);
    ffree(is_conflict_symbol);
    ffree(sp_table);
    ffree(new_action);
    ffree(update_action);

    return;
}
