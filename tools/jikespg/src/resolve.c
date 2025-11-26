static char hostfile[] = __FILE__;

#include "common.h"
#include "reduce.h"
#include "header.h"

/***********************************************************************/
/* VISITED is a structure used to mark state-symbol pairs that have    */
/* been visited in the process of computing follow-sources for a       */
/* given action in conflict.                                           */
/* The field MAP is an array indexable by the states 1..NUM_STATES;    */
/* each element of which points to a set (list) of symbols. Thus, for  */
/* a given state S, and for all symbol X in the list MAP[S], [S,X]     */
/* has been visited. For efficiency, the fields LIST and ROOT are used */
/* to store the set (list) of indexed elements of MAP that are not     */
/* NULL.                                                               */
/* See routines MARK_VISITED, WAS_VISITED, CLEAR_VISITED,              */
/*              INIT_LALRK_PROCESS, EXIT_PROCESS                       */
/***********************************************************************/
static struct visited_element
{
    struct node **map;
    short        *list,
                  root;
} visited;

/***********************************************************************/
/* Given a set of actions that are in conflict on a given symbol, the  */
/* structure SOURCES_ELEMENT is used to store a mapping from each      */
/* such action into a set of configurations that can be reached        */
/* following execution of the action in question up to the point where */
/* the automaton is about to shift the conflict symbol.                */
/* The field CONFIGS is an array indexable by actions which are        */
/* encoded as follows:                                                 */
/*    1. shift-reduce  [-NUM_RULES..-1]                                */
/*    2. reduce        [0..NUM_RULES]                                  */
/*    3. shift         [NUM_RULES+1..NUM_STATES+1].                    */
/* Each element of CONFIGS points to a set (sorted list) of            */
/* configurations.  For efficiency, the fields LIST and ROOT are used  */
/* to store the set (list) of indexed elements of CONFIGS that are not */
/* NULL.                                                               */
/* See routines ALLOCATE_SOURCES, FREE_SOURCES, CLEAR_SOURCES,         */
/*              ADD_CONFIGS, UNION_CONFIG_SETS.                        */
/* See STATE_TO_RESOLVE_CONFLICTS for an explanation of STACK_SEEN.    */
/*                                                                     */
/* A configuration is a stack of states that represents a certain path */
/* in the automaton. The stack is implemented as a list of             */
/* STACK_ELEMENT nodes linked through the field PREVIOUS.              */
/* A set/list of configurations is linked through the field NEXT.      */
/* When attempting to resolve conflicts we try to make sure that the   */
/* set of configurations associated with each action is unique. This   */
/* is achieved by throwing these configurations into a set and making  */
/* sure that there are no duplicates. The field LINK is used for that  */
/* purpose (see routine STACK_WAS_SEEN). The field STATE_NUMBER is     */
/* obviously used to store the number of a state in the automaton. The */
/* field SIZE holds index of the node within the stack. Thus, for the  */
/* first element of the stack this field represents the number of      */
/* elements in the stack; for the last element, this field holds the   */
/* value 1.                                                            */
/* See routines ALLOCATE_STACK_ELEMENT, FREE_STACK_ELEMENT,            */
/*              ADD_DANGLING_STACK, FREE_DANGLING_STACK.               */
/***********************************************************************/
static struct sources_element
{
    struct stack_element **configs,
                         **stack_seen;
    short                 *list,
                           root;
} sources;

struct stack_element
{
    struct stack_element *previous,
                         *next,
                         *link;
    short                 state_number,
                          size;
};
static struct stack_element *stack_pool = NULL,
                            *dangling_stacks = NULL;

/***********************************************************************/
/* The structure STATE_ELEMENT is used to construct lookahead states.  */
/* LA_STATE_ROOT point to a list of lookahead states using the LINK    */
/* field. The field NEXT_SHIFT is used to hash the new shift maps      */
/* associated with lookahead states. The field IN_STATE identifies the */
/* state that shifted into the lookahead state in question. The field  */
/* SYMBOL identifies the symbol on shift the transition was made into  */
/* the lookahead state in question.  The remaining fields are          */
/* self-explanatory.                                                   */
/***********************************************************************/
struct state_element
{
    struct state_element      *link,
                              *next_shift;
    struct reduce_header_type  reduce;
    struct shift_header_type   shift;
    short                      in_state,
                               symbol,
                               state_number,
                               shift_number;
};
static struct state_element *la_state_root = NULL;

/***********************************************************************/
/* The structures SR_CONFLICT_ELEMENT and RR_CONFLICT_ELEMENT are used */
/* th store conflict information. CONFLICT_ELEMENT_POOL is used to     */
/* keep track of a pool conflict element structures (SR or RR) that    */
/* are available for allocation.                                       */
/* See routines ALLOCATE_CONFLICT_ELEMENT and FREE_CONFLICT_ELEMENTS.  */
/***********************************************************************/
struct sr_conflict_element
{
    struct sr_conflict_element *next;
    short                       state_number,
                                item,
                                symbol;
};
static struct sr_conflict_element *sr_conflict_root;

struct rr_conflict_element
{
    struct rr_conflict_element *next;
    short                       symbol,
                                item1,
                                item2;
};
static struct rr_conflict_element *rr_conflict_root;
static void *conflict_element_pool = NULL;

/***********************************************************************/
/* NT_ITEMS and ITEM_LIST are used to construct a mapping from each    */
/* nonterminal into the set of items of which the nonterminal in       */
/* question is the dot symbol. See CONFLICTS_INITIALIZATION.           */
/***********************************************************************/
static short *nt_items  = NULL,
             *item_list = NULL;

/***********************************************************************/
/* LALR_VISITED is used to keep track of (state, nonterminal) pairs    */
/* that are visited in tracing the path of a lalr conflict.SLR_VISITED */
/* is similarly used to keep track of nonterminal symbols that are     */
/* visited in tracing the path of an slr conflict. SYMBOL_SEEN is used */
/* to keep track of nonterminal symbols that are visited in tracing a  */
/* path to the start state (root).                                     */
/*                                                                     */
/* CYCLIC is a boolean vector used to identify states that can enter   */
/* a cycle of transitions on nullable nonterminals.                    */
/* As the computation of CYCLIC requires a modified version of the     */
/* digraph algorithm, the variables STACK, INDEX_OF and TOP are used   */
/* for that algorithm.                                                 */
/*                                                                     */
/* RMPSELF is a boolean vector that indicates whether or not a given   */
/* non-terminal can right-most produce itself. It is only constructed  */
/* when LALR_LEVEL > 1.                                                */
/***********************************************************************/
static BOOLEAN *lalr_visited,
               *slr_visited,
               *symbol_seen,
               *cyclic,
               *rmpself;

static short *stack,
             *index_of,
              top;

static struct state_element **shift_table;

/*******************************************************************/
/*                     ALLOCATE_CONFLICT_ELEMENT:                  */
/*******************************************************************/
/* This function allocates a conflict_element (sr or rr) structure */
/* & returns a pointer to it. If there are nodes in the free pool, */
/* one of them is returned. Otherwise, a new node is allocated     */
/* from the temporary storage pool.                                */
/*******************************************************************/
static void *allocate_conflict_element(void)
{
    void *p;

    p = conflict_element_pool;
    if (p != NULL)
        conflict_element_pool = ((struct sr_conflict_element *) p) -> next;
    else
    {
        p = (void *) talloc(MAX(sizeof(struct sr_conflict_element),
                            sizeof(struct rr_conflict_element)));
        if (p == NULL)
            nospace(__FILE__, __LINE__);
    }

    return p;
}


/**********************************************************************/
/*                         FREE_CONFLICT_ELEMENTS:                    */
/**********************************************************************/
/* This routine returns a list of conflict_element (sr/rr)structures  */
/* to the free pool.                                                  */
/**********************************************************************/
static void free_conflict_elements(void *head, void *tail)
{
    ((struct sr_conflict_element *) tail) -> next =
             (struct sr_conflict_element *) conflict_element_pool;
    conflict_element_pool = head;

    return;
}


/*******************************************************************/
/*                      ALLOCATE_STACK_ELEMENT:                    */
/*******************************************************************/
/* This function allocates a stack_element structure and returns a */
/* pointer to it. If there are nodes in the free pool, one of them */
/* is returned. Otherwise, a new node is allocated from the        */
/* temporary storage pool.                                         */
/*******************************************************************/
static struct stack_element *allocate_stack_element(void)
{
    struct stack_element *p;

    p = stack_pool;
    if (p != NULL)
        stack_pool = p -> next;
    else
    {
        p = (struct stack_element *)
            talloc(sizeof(struct stack_element));
        if (p == NULL)
            nospace(__FILE__, __LINE__);
    }

    return p;
}


/**********************************************************************/
/*                           FREE_STACK_ELEMENTS:                     */
/**********************************************************************/
/* This routine returns a list of stack_element structures to the     */
/* free pool.                                                         */
/**********************************************************************/
static void free_stack_elements(struct stack_element *head,
                                struct stack_element *tail)
{
    tail -> next = stack_pool;
    stack_pool = head;

    return;
}


/***************************************************************************/
/*                        ADD_DANGLING_STACK_ELEMENT:                      */
/***************************************************************************/
/* When an allocated stack_element structure is not directly associated    */
/* with an action, it is added to a circular list of dangling stack_element*/
/* nodes so that its space can be reclaimed.                               */
/***************************************************************************/
static void add_dangling_stack_element(struct stack_element *s)
{
    if (dangling_stacks == NULL)
        s -> next = s;
    else
    {
        s -> next = dangling_stacks -> next;
        dangling_stacks -> next = s;
    }
    dangling_stacks = s;

    return;
}


/***************************************************************************/
/*                       FREE_DANGLING_STACK_ELEMENTS:                     */
/***************************************************************************/
/* This function is invoked to free up all dangling stack_element nodes    */
/* and reset the dangling stack list.                                      */
/* Recall that the dangling stack list is circular.                        */
/***************************************************************************/
static void free_dangling_stack_elements(void)
{
    struct stack_element *tail;

    if (dangling_stacks != NULL)
    {
        tail = dangling_stacks;
        free_stack_elements(dangling_stacks -> next, tail);

        dangling_stacks = NULL;
    }

    return;
}


/***************************************************************************/
/*                              ALLOCATE_SOURCES:                          */
/***************************************************************************/
/* This function allocates and initializes a SOURCE_ELEMENT map.           */
/* See definition of SOURCE_ELEMENT above.                                 */
/***************************************************************************/
static struct sources_element allocate_sources(void)
{
    struct sources_element sources;

    sources.configs = (struct stack_element **)
                      calloc(num_rules + num_rules + num_states + 1,
                             sizeof(struct stack_element *));
    if (sources.configs == NULL)
        nospace(__FILE__, __LINE__);
    sources.configs += num_rules;

    sources.stack_seen = (struct stack_element **)
                         calloc(STATE_TABLE_SIZE,
                                sizeof(struct stack_element *));
    if (sources.stack_seen == NULL)
        nospace(__FILE__, __LINE__);

    sources.list =
        Allocate_short_array(num_rules + num_rules + num_states + 1);
    sources.list += num_rules;

    sources.root = NIL;

    return sources;
}


/***************************************************************************/
/*                               CLEAR_SOURCES:                            */
/***************************************************************************/
/* This function takes as argument a SOURCES_ELEMENT structure which it    */
/* resets to the empty map.                                                */
/* See definition of SOURCE_ELEMENT above.                                 */
/***************************************************************************/
static struct sources_element clear_sources(struct sources_element sources)
{
    struct stack_element *p,
                         *tail;
    int act,
        i;

    for (act = sources.root; act != NIL; act = sources.list[act])
    {
        for (p = sources.configs[act]; p != NULL; tail = p, p = p -> next)
            ;
        free_stack_elements(sources.configs[act], tail);
        sources.configs[act] = NULL;
    }

    sources.root = NIL;

    return sources;
}


/***************************************************************************/
/*                               FREE_SOURCES:                             */
/***************************************************************************/
/* This function takes as argument a SOURCES_ELEMENT structure. First, it  */
/* clears it to reclaim all space that was used by STACK_ELEMENTs and then */
/* it frees the array space used as a base to construct the map.           */
/***************************************************************************/
static void free_sources(struct sources_element sources)
{
    sources = clear_sources(sources);

    sources.configs -= num_rules;
    ffree(sources.configs);

    ffree(sources.stack_seen);

    sources.list -= num_rules;
    ffree(sources.list);

    return;
}


/***************************************************************************/
/*                             UNION_CONFIG_SETS:                          */
/***************************************************************************/
/* This function takes as argument two pointers to sorted lists of stacks. */
/* It merges the lists in the proper order and returns the resulting list. */
/***************************************************************************/
static struct stack_element *union_config_sets(struct stack_element *root1,
                                               struct stack_element *root2)
{
    struct stack_element *p1,
                         *p2,
                         *root,
                         *tail;

    root = NULL;

    /*******************************************************************/
    /* This loop iterates over both lists until one (or both) has been */
    /* completely processed. Each time around the loop, a stack is     */
    /* removed from one of the lists and possibly added to the new     */
    /* list. The new list is initially kept as a circular list to      */
    /* preserve the sorted ordering in which elements are added to it. */
    /*******************************************************************/
    while (root1 != NULL && root2 != NULL)
    {
        /***************************************************************/
        /* Compare the two stacks in front of the lists for equality.  */
        /* We exit this loop when we encounter the end of one (or both)*/
        /* of the stacks or two elements in them that are not the same.*/
        /***************************************************************/
        for (p1 = root1, p2 = root2;
             p1 != NULL && p2 != NULL;
             p1 = p1 -> previous, p2 = p2 -> previous)
        {
            if (p1 -> state_number != p2 -> state_number)
                break;
        }

        /***************************************************************/
        /* We now have 3 cases to consider:                            */
        /*    1. The two stacks are equal? Discard one!                */
        /*    2. List 1 stack is prefix of list 2 stack (p1 == NULL)?  */
        /*       or list 1 stack is less than list 2 stack?            */
        /*       Remove list 1 stack and add it to new list.           */
        /*    3. List 2 stack is either a prefix of list 1 stack, or   */
        /*       it is smaller!                                        */
        /*       Remove list 2 stack and add it to new list.           */
        /***************************************************************/
        if (p1 == p2) /* are both p1 and p2 NULL? */
        {
            p2 = root2;
            root2 = root2 -> next;
            add_dangling_stack_element(p2);
        }
        else if ((p1 == NULL) ||
                ((p2 != NULL) && (p1 -> state_number < p2 -> state_number)))
        {
            p1 = root1;
            root1 = root1 -> next;

            if (root == NULL)
                p1 -> next = p1;
            else
            {
                p1 -> next = root -> next;
                root -> next = p1;
            }
            root = p1;
        }
        else
        {
            p2 = root2;
            root2 = root2 -> next;

            if (root == NULL)
                p2 -> next = p2;
            else
            {
                p2 -> next = root -> next;
                root -> next = p2;
            }
            root = p2;
        }
    }

    /*******************************************************************/
    /* At this stage, at least one (or both) list has been expended    */
    /* (or was empty to start with).                                   */
    /* If the new list is not empty, turn it into a linear list and    */
    /* append the unexpended list to it, if any.                       */
    /* Otherwise, set the new list to the nonempty list if any!        */
    /*******************************************************************/
    if (root != NULL)
    {
        tail = root;
        root = root -> next;
        tail -> next = (root1 == NULL ? root2 : root1);
    }
    else root = (root1 == NULL ? root2 : root1);

    return root;
}


/***************************************************************************/
/*                                ADD_CONFIGS:                             */
/***************************************************************************/
/* This function takes as argument a SOURCES_ELEMENT map, an ACTION and a  */
/* set (sorted list) of configurations. It adds the set of configurations  */
/* to the previous set of configurations associated with the ACTION in the */
/* SOURCES_ELEMENT map.                                                    */
/***************************************************************************/
static struct sources_element add_configs(struct sources_element sources,
                                          int action,
                                          struct stack_element *config_root)
{
    if (config_root == NULL) /* The new set is empty? Do nothing */
        return sources;

    if (sources.configs[action] == NULL) /* The previous was empty? */
    {
        sources.list[action] = sources.root;
        sources.root = action;
    }

    sources.configs[action] = union_config_sets(sources.configs[action],
                                                config_root);

    return sources;
}



/***************************************************************************/
/*                               CLEAR_VISITED:                            */
/***************************************************************************/
/* This function clears out all external space used by the VISITED set and */
/* resets VISITED to the empty set.                                        */
/***************************************************************************/
static void clear_visited(void)
{
    struct node *p,
                *tail;
    int state_no;

    for (state_no = visited.root;
         state_no != NIL; state_no = visited.list[state_no])
    {
        for (p = visited.map[state_no]; p != NULL; tail = p, p = p -> next)
            ;
        free_nodes(visited.map[state_no], tail);
        visited.map[state_no] = NULL;
    }

    visited.root = NIL;

    return;
}

/***************************************************************************/
/*                                WAS_VISITED:                             */
/***************************************************************************/
/* This boolean function checks whether or not a given pair [state, symbol]*/
/* was already inserted in the VISITED set.                                */
/***************************************************************************/
static BOOLEAN was_visited(int state_no, int symbol)
{
    struct node *p;

    for (p = visited.map[state_no]; p != NULL; p = p -> next)
    {
        if (p -> value == symbol)
            break;
    }

    return (p != NULL);
}


/***************************************************************************/
/*                               MARK_VISITED:                             */
/***************************************************************************/
/* This function inserts a given pair [state, symbol] into the VISITED set.*/
/***************************************************************************/
static void mark_visited(int state_no, int symbol)
{
    struct node *p;

    if (visited.map[state_no] == NULL) /* 1st time we see state_no? */
    {
        visited.list[state_no] = visited.root;
        visited.root = state_no;
    }

    p = Allocate_node();
    p -> value = symbol;
    p -> next = visited.map[state_no];
    visited.map[state_no] = p;

    return;
}


/***********************************************************************/
/*                             COMPUTE_CYCLIC:                         */
/***********************************************************************/
/* This procedure is a modified instantiation of the digraph algorithm */
/* to compute the CYCLIC set of states.                                */
/***********************************************************************/
static void compute_cyclic(short state_no)
{
    int indx;
    struct goto_header_type go_to;
    int symbol,
        act,
        i;

    stack[++top] = state_no;
    indx = top;
    cyclic[state_no] = FALSE;
    index_of[state_no] = indx;

    go_to = statset[state_no].go_to;
    for (i = 1; i <= go_to.size; i++)
    {
        symbol = GOTO_SYMBOL(go_to, i);
        act = GOTO_ACTION(go_to, i);
        if (act > 0 && null_nt[symbol])
        {   /* We have a transition on a nullable nonterminal? */
            if (index_of[act] == OMEGA)
                compute_cyclic(act);
            else if (index_of[act] != INFINITY)
                cyclic[state_no] = TRUE;
            cyclic[state_no] = cyclic[state_no] || cyclic[act];

            index_of[state_no] = MIN(index_of[state_no], index_of[act]);
        }
    }

    if (index_of[state_no] == indx)
    {
        do
        {
            act = stack[top--];
            index_of[act] = INFINITY;
        } while(act != state_no);
    }

    return;
}


/***********************************************************************/
/*                           TRACE_ROOT:                               */
/***********************************************************************/
/*    In tracing an error, we will be moving backward in the state     */
/* automaton looking for items with the conflict symbol as look-ahead. */
/* In the case of SLR, we may have to analoguously look at an          */
/* arbitrary set of items involved.  In moving around these graphs, it */
/* is possible to encounter a cycle, in which case, we simply want to  */
/* back out of the cycle and try another path. We therefore need to    */
/* keep track of which nodes have already been visited.  For LALR      */
/* conflicts, we use the LA_PTR field of the GOTO_ELEMENTs as an index */
/* to a BOOLEAN array LALR_VISITED.  For SLR conflicts, a boolean      */
/* array, SLR_VISITED, indexable by non-terminals, is used.  For       */
/* trace-backs to the root item, the boolean array SYMBOL_SEEN, also   */
/* also indexable by non-terminals, is used.                           */
/***********************************************************************/
static BOOLEAN trace_root(int lhs_symbol)
{
    int item;

    if (lhs_symbol == accept_image)
        return(TRUE);

    if (symbol_seen[lhs_symbol])
        return(FALSE);

    symbol_seen[lhs_symbol] = TRUE;
    for (item = nt_items[lhs_symbol]; item != NIL; item = item_list[item])
    {
        if (trace_root(rules[item_table[item].rule_number].lhs))
        {
            print_item(item);
            return(TRUE);
        }
    }

    return(FALSE);
}


/***********************************************************************/
/*                             PRINT_ROOT_PATH:                        */
/***********************************************************************/
/* The procedure below is invoked to retrace a path from the initial   */
/* item to a given item (ITEM_NO) passed to it as argument.            */
/***********************************************************************/
static void print_root_path(int item_no)
{
    symbol_seen = Allocate_boolean_array(num_non_terminals);
    symbol_seen -= (num_terminals + 1);

    if (trace_root(rules[item_table[item_no].rule_number].lhs))
    {
        fprintf(syslis, "\n"); /* Leave one blank line after root trace. */
        ENDPAGE_CHECK;
    }

    symbol_seen += (num_terminals + 1);
    ffree(symbol_seen);

    return;
}


/***********************************************************************/
/*                          LALR_PATH_RETRACED:                        */
/***********************************************************************/
/* This procedure takes as argument, a state number, STATE_NO, an      */
/* index into the goto map of state_no, GOTO_INDX, which identifies a  */
/* starting point for a search for the CONFLICT_SYMBOL. It attempts to */
/* find a path in the automaton (from the starting point) that leads   */
/* to a state where the conflict symbol can be read. If a path is      */
/* found, all items along the path are printed and SUCCESS is returned.*/
/*  Otherwise, FAILURE is returned.                                    */
/***********************************************************************/
static BOOLEAN lalr_path_retraced(int state_no,
                                  int goto_indx,
                                  int conflict_symbol)
{
    int symbol,
        item,
        state,
        i;

    struct goto_header_type go_to;

    struct node *p,
                *q,
                *tail,
                *w;

    BOOLEAN found;

    go_to = statset[state_no].go_to;
    lalr_visited[GOTO_LAPTR(go_to, goto_indx)] = TRUE;

    found = FALSE;

    state = GOTO_ACTION(go_to, goto_indx);
    for (p = (state > 0 ? statset[state].kernel_items
                        : adequate_item[-state]);
         (p != NULL) && (! found); p = p -> next)
    {
        item = p -> value - 1;
        if IS_IN_SET(first, item_table[item].suffix_index, conflict_symbol)
        {   /* Conflict_symbol can be read in state? */
            if (trace_opt == TRACE_FULL)
                print_root_path(item);
            found = TRUE;
        }
        else if IS_IN_SET(first, item_table[item].suffix_index, empty)
        {
            symbol = rules[item_table[item].rule_number].lhs;
            w = lpgaccess(state_no, item);
            for (q = w; q != NULL; tail = q, q = q -> next)
            {
                go_to = statset[q -> value].go_to;
                for (i = 1; GOTO_SYMBOL(go_to, i) != symbol; i++)
                    ;

                if (! lalr_visited[GOTO_LAPTR(go_to, i)])
                {
                    if (lalr_path_retraced(q -> value, i, conflict_symbol))
                    {
                        found = TRUE;
                        break;
                    }
                }
            }

            for (; q != NULL; tail = q, q = q -> next)
                ;
            free_nodes(w, tail);
        }
    }

    if (found)
        print_item(item);

    return(found);
}


/***********************************************************************/
/*                      PRINT_RELEVANT_LALR_ITEMS:                     */
/***********************************************************************/
/*   In this procedure, we attempt to retrace an LALR conflict path    */
/* (there may be more than one) of CONFLICT_SYMBOL in the state        */
/* automaton that led to ITEM_NO in state STATE_NO.                    */
/***********************************************************************/
static void print_relevant_lalr_items(int state_no,
                                      int item_no,
                                      int conflict_symbol)
{
    int lhs_symbol,
        i;

    struct node *p,
                *tail,
                *v;

    lhs_symbol = rules[item_table[item_no].rule_number].lhs;
    if (lhs_symbol == accept_image)
        return;

    lalr_visited = Allocate_boolean_array(la_top + 1);

    v = lpgaccess(state_no, item_no);
    for (p = v; p != NULL; tail = p, p = p -> next)
    {
        struct goto_header_type go_to;

        go_to = statset[p -> value].go_to;
        for (i = 1; GOTO_SYMBOL(go_to, i) != lhs_symbol; i++)
            ;

        if (lalr_path_retraced(p -> value, i, conflict_symbol))
            break;
    }

    for (; p != NULL; tail = p, p = p -> next)
        ;

    free_nodes(v, tail);

    ffree(lalr_visited);

    return;
}


/***********************************************************************/
/*                              SLR_TRACE:                             */
/***********************************************************************/
/* The procedure below is invoked to retrace a path that may have      */
/* introduced the CONFLICT_SYMBOL in the FOLLOW set of the nonterminal */
/* that produces ITEM_NO.  Note that such a path must exist.           */
/***********************************************************************/
static BOOLEAN slr_trace(int lhs_symbol, int conflict_symbol)
{
    int item;

    if (slr_visited[lhs_symbol])
        return(FALSE);

    slr_visited[lhs_symbol] = TRUE;

    for (item = nt_items[lhs_symbol]; item != NIL; item = item_list[item])
    {
        if IS_IN_SET(first, item_table[item].suffix_index, conflict_symbol)
        {
            if (trace_opt == TRACE_FULL)
                print_root_path(item);
            break;
        }
        if IS_IN_SET(first, item_table[item].suffix_index, empty)
        {
            if (slr_trace(rules[item_table[item].rule_number].lhs,
                          conflict_symbol))
                break;
        }
    }

    if (item != NIL)
    {
        print_item(item);
        return(TRUE);
    }
    else
        return(FALSE);
}


/***********************************************************************/
/*                           PRINT_RELEVANT_SLR_ITEMS:                 */
/***********************************************************************/
/* This procedure is invoked to print an SLR path of items that leads  */
/* to the conflict symbol.                                             */
/***********************************************************************/
static void print_relevant_slr_items(int item_no, int conflict_symbol)
{
    slr_visited = Allocate_boolean_array(num_non_terminals);
    slr_visited -= (num_terminals + 1);

    if (slr_trace(rules[item_table[item_no].rule_number].lhs,
                   conflict_symbol))
        ;
    slr_visited += (num_terminals + 1);
    ffree(slr_visited);

    return;
}


/***********************************************************************/
/*                       CONFLICTS_INITIALIZATION:                     */
/***********************************************************************/
/* This routine is invoked when a grammar contains conflicts, and the  */
/* first conflict is detected.                                         */
/***********************************************************************/
static void conflicts_initialization(void)
{
    int i;

    /*******************************************************************/
    /* NT_ITEMS and ITEM_LIST are used in reporting SLR conflicts, and */
    /* in recreating paths from the Start item. See the routines       */
    /* PRINT_RELEVANT_SLR_ITEMS and PRINT_ROOT_PATH.                   */
    /*******************************************************************/
    nt_items = Allocate_short_array(num_non_terminals);
    nt_items -= (num_terminals + 1);
    item_list = Allocate_short_array(num_items + 1);

    i = (PRINT_LINE_SIZE - 11) / 2 - 1;
    PR_HEADING;
    fill_in(msg_line, i, '-');
    fprintf(syslis, "\n%s CONFLICTS %s\n", msg_line, msg_line);
    output_line_no += 2;

/***********************************************************************/
/*   SLR conflicts may be caused by a symbol in the FOLLOW set of a    */
/* left hand side, which is not actually in the LALR look-ahead set in */
/* that context.  Therefore, there may not exist a path in the state   */
/* automaton from the state where the conflict was detected to another */
/* state where it was introduced.  In such a case, we try to retrace a */
/* path that contributed the conflict-symbol to the FOLLOW set via a   */
/* sequence of productions.                                            */
/*                                                                     */
/* To assist in this task, we build below a map from each non-terminal */
/* A to the set of items of which A is the dot SYMBOL. I.e., all items */
/* of the form [x .A y] where x and y are arbitrary strings, and A is  */
/* a non-terminal. This map is also used in retracing a path from the  */
/* Start item to any other item.                                       */
/***********************************************************************/
    for ALL_NON_TERMINALS(i)
        nt_items[i] = NIL;

    for ALL_ITEMS(i)
    {
        if (item_table[i].symbol IS_A_NON_TERMINAL)
        {
            item_list[i] = nt_items[item_table[i].symbol];
            nt_items[item_table[i].symbol] =  i;
        }
    }

    return;
}


/***********************************************************************/
/*                            PROCESS_CONFLICTS:                       */
/***********************************************************************/
/*   If conflicts are detected, tehy are placed in two lists headed by */
/* SR_CONFLICT_ROOT and RR_CONFLICT_ROOT.  We scan these lists, and    */
/* report the conflicts.                                               */
/***********************************************************************/
static void process_conflicts(int state_no)
{
    int symbol,
        rule_no,
        n;

    char temp[SYMBOL_SIZE + 1];

    if (nt_items == NULL)
        conflicts_initialization();

    print_state(state_no); /* Print state containing conflicts */

    /*******************************************************************/
    /* Process shift-reduce conflicts.                                 */
    /*******************************************************************/
    if (sr_conflict_root != NULL)
    {
        struct sr_conflict_element *p,
                                   *tail;

        for (p = sr_conflict_root; p != NULL; tail = p, p = p -> next)
        {
            symbol = p -> symbol;
            rule_no = item_table[p -> item].rule_number;

            restore_symbol(temp, RETRIEVE_STRING(symbol));
            printf("*** Shift/reduce conflict on \"%s\" with rule %d\n",
                   temp, rule_no);
            fprintf(syslis,
                    "\n*** Shift/reduce conflict on \"%s\" with rule %d\n",
                    temp, rule_no);

            if (trace_opt != NOTRACE)
            {
                if (! slr_bit)
                    print_relevant_lalr_items(state_no, p -> item, symbol);
                else
                    print_relevant_slr_items(p -> item, symbol);
                print_item(p -> item);
            }
        }

        free_conflict_elements(sr_conflict_root, tail);
    }

    /*******************************************************************/
    /* Process reduce-reduce conflicts.                                */
    /*******************************************************************/
    if (rr_conflict_root != NULL)
    {
        struct rr_conflict_element *p,
                                   *tail;

        for (p = rr_conflict_root; p != NULL; tail = p, p = p -> next)
        {
            symbol = p -> symbol;
            n = item_table[p -> item1].rule_number;
            rule_no = item_table[p -> item2].rule_number;

            restore_symbol(temp, RETRIEVE_STRING(symbol));
            printf("*** Reduce/reduce conflict "
                   "on \"%s\" between rule %d and %d\n",
                   temp, n, rule_no);
            fprintf(syslis,
                    "\n*** Reduce/reduce conflict "
                    "on \"%s\" between rule %d and %d\n",
                    temp, n, rule_no);
            output_line_no +=2;
            if (trace_opt != NOTRACE)
            {
                if (! slr_bit)
                    print_relevant_lalr_items(state_no, p -> item1, symbol);
                else
                    print_relevant_slr_items(p -> item1, symbol);
                print_item(p -> item1);
                fill_in(msg_line, PRINT_LINE_SIZE - 3, '-');
                fprintf(syslis,"\n%s",msg_line);
                ENDPAGE_CHECK;
                if (! slr_bit)
                    print_relevant_lalr_items(state_no, p -> item2, symbol);
                else
                    print_relevant_slr_items(p -> item2, symbol);
                print_item(p -> item2);
            }
        }

        free_conflict_elements(rr_conflict_root, tail);
    }

    return;
}


/***********************************************************************/
/*                           ADD_CONFLICT_SYMBOL:                      */
/***********************************************************************/
/* Add SYMBOL to the set of symbols CONFLICT_SYMBOLS[STATE_NO].        */
/***********************************************************************/
static void add_conflict_symbol(int state_no, int symbol)
{
    struct node *p;

    p = Allocate_node();
    p -> value = symbol;
    if (conflict_symbols[state_no] == NULL)
        p -> next = p;
    else
    {
        p -> next = conflict_symbols[state_no] -> next;
        conflict_symbols[state_no] -> next = p;
    }
    conflict_symbols[state_no] = p;

    return;
}


/***********************************************************************/
/*                             FOLLOW_SOURCES:                         */
/***********************************************************************/
/* This function takes as argument a configuration STACK, a SYMBOL on  */
/* which a transition can be made in the configuration and a terminal  */
/* lookahead symbol, LA_SYMBOL. It executes the transition on SYMBOL   */
/* and simulates all paths taken in the automaton after that transition*/
/* until new state(s) are reached where a transition is possible on    */
/* the lookahead symbol. It then returns the new set of configurations */
/* found on which a transition on LA_SYMBOL is possible.               */
/***********************************************************************/
static struct stack_element *follow_sources(struct stack_element *stack,
                                            int symbol, int la_symbol)
{
    struct shift_header_type sh;
    struct goto_header_type  go_to;

    struct stack_element *configs,
                         *q;
    struct node *item_ptr;

    int item_no,
        state_no,
        rule_no,
        lhs_symbol,
        act,
        i;

    configs = NULL; /* Initialize the output set of configurations */

    /*******************************************************************/
    /* If the starting configuration consists of a single state and    */
    /* the initial [state, symbol] pair has already been visited,      */
    /* return the null set. Otherwise, mark the pair visited and ...   */
    /*******************************************************************/
    state_no = stack -> state_number;
    if (stack -> size == 1)
    {
        if (was_visited(state_no, symbol) ||
            (state_no == 1 && symbol == accept_image))
            return configs;

        mark_visited(state_no, symbol);
    }

    /*******************************************************************/
    /* Find the transition defined on the symbol...                    */
    /* If the SYMBOL is a nonterminal and we can determine that the    */
    /* lookahead symbol (LA_SYMBOL) cannot possibly follow the         */
    /* nonterminal in question in this context, we simply abandon the  */
    /* search and return the NULL set.                                 */
    /*******************************************************************/
    if (symbol IS_A_NON_TERMINAL)
    {
        go_to = statset[state_no].go_to;
        for (i = 1; GOTO_SYMBOL(go_to, i) != symbol; i++)
            ;
        if (la_index[GOTO_LAPTR(go_to, i)] == OMEGA)
        {
            int stack_top = 0;
            la_traverse(state_no, i, &stack_top);
        }

        if (! IS_IN_SET(la_set, GOTO_LAPTR(go_to, i), la_symbol))
            return configs;

        act = GOTO_ACTION(go_to, i);
    }
    else
    {
        sh = shift[statset[state_no].shift_number];
        for (i = 1; SHIFT_SYMBOL(sh, i) != symbol; i++)
            ;
        act = SHIFT_ACTION(sh, i);
    }

    /*******************************************************************/
    /* If the ACTion on the symbol is a shift or a goto, ...           */
    /*******************************************************************/
    if (act > 0)
    {
        /***************************************************************/
        /* We check to see if the new state contains an action on the  */
        /* lookahead symbol. If that's the case then we create a new   */
        /* configuration by appending ACT to the starting configuration*/
        /* and add this newly formed configuration to the set(list) of */
        /* configurations...                                           */
        /***************************************************************/
        sh = shift[statset[act].shift_number];
        for (i = 1; i <= sh.size; i++)
        {
            if (SHIFT_SYMBOL(sh, i) == la_symbol)
                break;
        }
        if (i <= sh.size) /* there is a transition on la_symbol in act */
        {
            q = allocate_stack_element();
            q -> state_number = act;
            q -> size = stack -> size + 1;
            q -> previous = stack;
            q -> next = NULL;

            configs = q;
        }

        /***************************************************************/
        /* If the new state cannot get into a cycle of null            */
        /* transitions, we check to see if it contains any transition  */
        /* on a nullable nonterminal. For each such transition, we     */
        /* append the new state to the stack and recursively invoke    */
        /* FOLLOW_SOURCES to check if a transition on LA_SYMBOL cannot */
        /* follow such a null transition.                              */
        /***************************************************************/
        if (! cyclic[act])
        {
            go_to = statset[act].go_to;
            for (i = 1; i <= go_to.size; i++)
            {
                symbol = GOTO_SYMBOL(go_to, i);
                if (null_nt[symbol])
                {
                    struct stack_element *new_configs;

                    q = allocate_stack_element();
                    q -> state_number = act;
                    q -> size = stack -> size + 1;
                    q -> previous = stack;
                    q -> next = NULL;

                    new_configs = follow_sources(q, symbol, la_symbol);
                    if (new_configs == NULL)
                        free_stack_elements(q, q);
                    else
                    {
                        add_dangling_stack_element(q);
                        configs = union_config_sets(configs, new_configs);
                    }
                }
            }
        }
    }

    /*******************************************************************/
    /* We now iterate over the kernel set of items associated with the */
    /* ACTion defined on SYMBOL...                                     */
    /*******************************************************************/
    for (item_ptr = (act > 0 ? statset[act].kernel_items
                             : adequate_item[-act]);
         item_ptr != NULL; item_ptr = item_ptr -> next)
    {
        item_no = item_ptr -> value;

        /***************************************************************/
        /* For each item that is a final item whose left-hand side     */
        /* is neither the starting symbol nor a symbol that can        */
        /* right-most produce itself...                                */
        /***************************************************************/
        if (item_table[item_no].symbol == empty)
        {
            rule_no = item_table[item_no].rule_number;
            lhs_symbol = rules[rule_no].lhs;
            if (lhs_symbol != accept_image && ! rmpself[lhs_symbol])
            {
                /*******************************************************/
                /* If the length of the prefix of the item preceeding  */
                /* the dot is shorter that the length of the stack, we */
                /* retrace the item's path within the stack and        */
                /* invoke FOLLOW_SOURCES with the prefix of the stack  */
                /* where the item was introduced through closure, the  */
                /* left-hand side of the item and the lookahead symbol.*/
                /*******************************************************/
                if (item_table[item_no].dot < stack -> size)
                {
                    q = stack;
                    for (i = 1; i < item_table[item_no].dot; i++)
                        q = q -> previous;

                    q = follow_sources(q, lhs_symbol, la_symbol);
                    configs = union_config_sets(configs, q);
                }
                else
                {
                    struct node *v,
                                *p,
                                *tail;

                    /***************************************************/
                    /* Compute the item in the root state of the stack,*/
                    /* and find the root state...                      */
                    /***************************************************/
                    item_no -= stack -> size;
                    for (q = stack; q -> size != 1; q = q -> previous)
                        ;

                    /***************************************************/
                    /* We are now back in the main automaton, find all */
                    /* sources where the item was introduced through   */
                    /* closure start a new configuration and invoke    */
                    /* FOLLOW_SOURCES with the appropriate arguments to*/
                    /* calculate the set of configurations associated  */
                    /* with these sources.                             */
                    /***************************************************/
                    v = lpgaccess(q -> state_number, item_no);
                    for (p = v; p != NULL; tail = p, p = p -> next)
                    {
                        struct stack_element *new_configs;

                        q = allocate_stack_element();
                        q -> state_number = p -> value;
                        q -> size = 1;
                        q -> previous = NULL;
                        q -> next = NULL;

                        new_configs = follow_sources(q, lhs_symbol, la_symbol);
                        if (new_configs == NULL)
                            free_stack_elements(q, q);
                        else
                        {
                            add_dangling_stack_element(q);
                            configs = union_config_sets(configs, new_configs);
                        }
                    }

                    free_nodes(v, tail);
                }
            }
        }
    }

    return configs;
}


/***********************************************************************/
/*                                NEXT_LA:                             */
/***********************************************************************/
/* This function has a similar structure as FOLLOW_SOURCES.  But,      */
/* instead of computing configurations that can be reached, it         */
/* computes lookahead symbols that can be reached.  It takes as        */
/* argument a configuration STACK, a SYMBOL on which a transition can  */
/* be made in the configuration and a set variable, LOOK_AHEAD, where  */
/* the result is to be stored.  When NEXT_LA is invoked from the       */
/* outside, LOOK_AHEAD is assumed to be initialized to the empty set.  */
/* NEXT_LA first executes the transition on SYMBOL and thereafter, all */
/* terminal symbols that can be read are added to LOOKAHEAD.           */
/***********************************************************************/
static void next_la(struct stack_element *stack,
                    int symbol, SET_PTR look_ahead)
{
    struct shift_header_type sh;
    struct goto_header_type  go_to;

    struct stack_element *q;
    struct node *item_ptr;

    int item_no,
        state_no,
        rule_no,
        lhs_symbol,
        act,
        i;

    /*******************************************************************/
    /* The only symbol that can follow the end-of-file symbol is the   */
    /* end-of-file symbol.                                             */
    /*******************************************************************/
    if (symbol == eoft_image)
    {
        SET_BIT_IN(look_ahead, 0, eoft_image);
        return;
    }

    state_no = stack -> state_number;

    /*******************************************************************/
    /* Find the transition defined on the symbol...                    */
    /*******************************************************************/
    if (symbol IS_A_NON_TERMINAL)
    {
        go_to = statset[state_no].go_to;
        for (i = 1; GOTO_SYMBOL(go_to, i) != symbol; i++)
            ;
        act = GOTO_ACTION(go_to, i);
    }
    else
    {
        sh = shift[statset[state_no].shift_number];
        for (i = 1; SHIFT_SYMBOL(sh, i) != symbol; i++)
            ;
        act = SHIFT_ACTION(sh, i);
    }

    /*******************************************************************/
    /* If the ACTion on the symbol is a shift or a goto, then all      */
    /* terminal symbols that can be read in ACT are added to           */
    /* LOOK_AHEAD.                                                     */
    /*******************************************************************/
    if (act > 0)
    {
        SET_UNION(look_ahead, 0, read_set, act);
    }

    /*******************************************************************/
    /* We now iterate over the kernel set of items associated with the */
    /* ACTion defined on SYMBOL...                                     */
    /* Recall that the READ_SET of ACT is but the union of the FIRST   */
    /* map defined on the suffixes of the items in the kernel of ACT.  */
    /*******************************************************************/
    for (item_ptr = (act > 0 ? statset[act].kernel_items
                             : adequate_item[-act]);
         item_ptr != NULL; item_ptr = item_ptr -> next)
    {
        item_no = item_ptr -> value;

        /***************************************************************/
        /* For each item that is a final item whose left-hand side     */
        /* is neither the starting symbol nor a symbol that can        */
        /* right-most produce itself...                                */
        /***************************************************************/
        if (IS_IN_SET(first, item_table[item_no - 1].suffix_index, empty))
        {
            rule_no = item_table[item_no].rule_number;
            lhs_symbol = rules[rule_no].lhs;
            if (lhs_symbol != accept_image && ! rmpself[lhs_symbol])
            {
                /*******************************************************/
                /* If the length of the prefix of the item preceeding  */
                /* the dot is shorter that the length of the stack, we */
                /* retrace the item's path within the stack and        */
                /* invoke NEXT_LA with the prefix of the stack         */
                /* where the item was introduced through closure, the  */
                /* left-hand side of the item and LOOK_AHEAD.          */
                /*******************************************************/
                if (item_table[item_no].dot < stack -> size)
                {
                    q = stack;
                    for (i = 1; i < item_table[item_no].dot; i++)
                        q = q -> previous;
                    next_la(q, lhs_symbol, look_ahead);
                }
                else
                {
                    struct node *v,
                                *p,
                                *tail;

                    /***************************************************/
                    /* Compute the item in the root state of the stack,*/
                    /* and find the root state...                      */
                    /***************************************************/
                    item_no -= stack -> size;
                    for (q = stack; q -> size != 1; q = q -> previous)
                        ;

                    /***************************************************/
                    /* We are now back in the main automaton, find all */
                    /* sources where the item was introduced through   */
                    /* closure and add all terminal symbols in the     */
                    /* follow set of the left-hand side symbol in each */
                    /* source to LOOK_AHEAD.                           */
                    /***************************************************/
                    v = lpgaccess(q -> state_number, item_no);
                    for (p = v; p != NULL; tail = p, p = p -> next)
                    {
                        go_to = statset[p -> value].go_to;
                        for (i = 1; GOTO_SYMBOL(go_to, i) != lhs_symbol; i++)
                            ;

                        /***********************************************/
                        /* If look-ahead after left hand side is not   */
                        /* yet computed,call LA_TRAVERSE to compute it.*/
                        /***********************************************/
                        if (la_index[GOTO_LAPTR(go_to, i)] == OMEGA)
                        {
                            int stack_top = 0;
                            la_traverse(p -> value, i, &stack_top);
                        }

                        SET_UNION(look_ahead, 0,
                                  la_set, GOTO_LAPTR(go_to, i));
                    }

                    free_nodes(v, tail);
                }
            }
        }
    }

    return;
}


/***********************************************************************/
/*                              STACK_WAS_SEEN:                        */
/***********************************************************************/
/* This function takes as argument an array, STACK_SEEN, with          */
/* STATE_TABLE_SIZE elements (indexable in the range                   */
/* 0..STATE_TABLE_SIZE-1) which is the base of a hash table and a      */
/* STACK. It searches the hash table to see if it already contained    */
/* the stack in question. If yes, it returns TRUE. Otherwise, it       */
/* inserts the stack into the table and returns FALSE.                 */
/***********************************************************************/
static BOOLEAN stack_was_seen(struct stack_element **stack_seen,
                              struct stack_element *stack)
{
    unsigned long hash_address;
    struct stack_element *p,
                         *q,
                         *r;

    hash_address = stack -> size;   /* Initialize hash address */
    for (p = stack; p != NULL; p = p -> previous)
        hash_address += p -> state_number;
    hash_address %= STATE_TABLE_SIZE;

    for (p = stack_seen[hash_address]; p != NULL; p = p -> link)
    {
        if (stack -> size == p -> size)
        {
            for (q = stack, r = p;
                 q != NULL;
                 q = q -> previous, r = r -> previous)
            {
                if (q -> state_number != r -> state_number)
                    break;
            }
            if (q == NULL)
                return TRUE;
        }
    }

    stack -> link = stack_seen[hash_address];
    stack_seen[hash_address] = stack;

    return FALSE;
}


/***********************************************************************/
/*                      STATE_TO_RESOLVE_CONFLICTS:                    */
/***********************************************************************/
/* STATE_TO_RESOLVE_CONFLICTS is a function that attempts to resolve   */
/* conflicts by doing more look-ahead.  If the conflict resolution     */
/* is successful, then a new state is created and returned; otherwise, */
/* the NULL pointer is returned.                                       */
/***********************************************************************/
static struct state_element *state_to_resolve_conflicts
       (struct sources_element sources, int la_symbol, int level)
{
    struct sources_element new_sources;

    struct node **action,
                 *p,
                 *tail;

    short *symbol_list,
          *action_list,
          *rule_count,
           symbol_root,
           shift_root,
           reduce_root;

    SET_PTR look_ahead;

    struct stack_element *stack;

    struct state_element **la_shift_state,
                          *state;

    int num_shift_actions,
        num_reduce_actions,
        default_rule,
        count,
        i,
        symbol,
        act;

    new_sources = allocate_sources();

    action = (struct node **)
             calloc(num_terminals + 1, sizeof(struct node *));
    if (action == NULL)
        nospace(__FILE__, __LINE__);

    symbol_list = Allocate_short_array(num_terminals + 1);
    action_list = Allocate_short_array(num_terminals + 1);
    rule_count = Allocate_short_array(num_rules + 1);

    look_ahead = (SET_PTR)
                 calloc(1, term_set_size * sizeof(BOOLEAN_CELL));
    if (look_ahead == NULL)
        nospace(__FILE__, __LINE__);

    la_shift_state = (struct state_element **)
                     calloc(num_terminals + 1,
                            sizeof(struct state_element *));
    if (la_shift_state == NULL)
        nospace(__FILE__, __LINE__);

    /*******************************************************************/
    /* Initialize new lookahead state. Initialize counters. Check and  */
    /* adjust HIGHEST_LEVEL reached so far, if necessary.              */
    /*******************************************************************/
    state = NULL;

    num_shift_actions = 0;
    num_reduce_actions = 0;
    shift_root = NIL;
    reduce_root = NIL;

    if (level > highest_level)
        highest_level = level;

    /*******************************************************************/
    /* One of the parameters received is a SOURCES map whose domain is */
    /* a set of actions and each of these actions is mapped into a set */
    /* of configurations that can be reached after that action is      */
    /* executed (in the state where the conflicts were detected).      */
    /* In this loop, we compute an ACTION map which maps each each     */
    /* terminal symbol into 0 or more actions in the domain of SOURCES.*/
    /*                                                                 */
    /* NOTE in a sources map, if a configuration is associated with    */
    /* more than one action then the grammar is not LALR(k) for any k. */
    /* We check for that condition below. However, this check is there */
    /* for purely cosmetic reason. It is not necessary for the         */
    /* algorithm to work correctly and its removal will speed up this  */
    /* loop somewhat (for conflict-less input).                        */
    /* The first loop below initializes the hash table used for        */
    /* lookups ...                                                     */
    /*******************************************************************/
    for (i = 0; i < STATE_TABLE_SIZE; i++)
         sources.stack_seen[i] = NULL;

    symbol_root = NIL;
    for (act = sources.root; act != NIL; act = sources.list[act])
    {
        /***************************************************************/
        /* For each action we iterate over its associated set of       */
        /* configurations and invoke NEXT_LA to compute the lookahead  */
        /* set for that configuration. These lookahead sets are in     */
        /* turn unioned together to form a lookahead set for the       */
        /* action in question.                                         */
        /***************************************************************/
        INIT_SET(look_ahead);
        for (stack = sources.configs[act];
             stack != NULL; stack = stack -> next)
        {
            if (stack_was_seen(sources.stack_seen, stack))
            {/* This is the superfluous code mentioned above! */
                highest_level = INFINITY;
                goto clean_up_and_return;
            }

            next_la(stack, la_symbol, look_ahead);
        }
        RESET_BIT(look_ahead, empty);         /* EMPTY never in LA set */

        /***************************************************************/
        /* For each lookahead symbol computed for this action, add an  */
        /* action to the ACTION map and keep track of the symbols on   */
        /* which any action is defined.                                */
        /* If new conflicts are detected and we are already at the     */
        /* lookahead level requested, we terminate the computation...  */
        /***************************************************************/
        count = 0;
        for ALL_TERMINALS(symbol)
        {
            if (IS_ELEMENT(look_ahead, symbol))
            {
                count++;

                if (action[symbol] == NULL)
                {
                    symbol_list[symbol] = symbol_root;
                    symbol_root = symbol;
                }
                else if (level == lalr_level)
                    goto clean_up_and_return;

                p = Allocate_node();
                p -> value = act;
                p -> next = action[symbol];
                action[symbol] = p;
            }
        }

        /***************************************************************/
        /* If the action in question is a reduction then we keep track */
        /* of how many times it was used.                              */
        /***************************************************************/
        if (act >= 0 && act <= num_rules)
            rule_count[act] = count;
    }

    /*******************************************************************/
    /* We now iterate over the symbols on which actions are defined.   */
    /* If we detect conflicts on any symbol, we compute new sources    */
    /* and try to recover by computing more lookahead. Otherwise, we   */
    /* update the counts and create two lists: a list of symbols on    */
    /* which shift actions are defined and a list of symbols on which  */
    /* reduce actions are defined.                                     */
    /*******************************************************************/
    for (symbol = symbol_root;
         symbol != NIL; symbol = symbol_list[symbol])
    {
        /***************************************************************/
        /* We have four cases to consider:                             */
        /*    1. There are conflicts on SYMBOL                         */
        /*    2. The action on SYMBOL is a shift-reduce                */
        /*    3. The action on SYMBOL is a shift                       */
        /*    4. The action on SYMBOL is a reduce                      */
        /***************************************************************/
        if (action[symbol] -> next != NULL)
        {
            new_sources = clear_sources(new_sources);
            for (p = action[symbol]; p != NULL; tail = p, p = p -> next)
            {
                act = p -> value;
                if (act >= 0 && act <= num_rules)
                    rule_count[act]--;

                clear_visited();

                for (stack = sources.configs[act];
                     stack != NULL; stack = stack -> next)
                {
                    struct stack_element *new_configs;

                    new_configs = follow_sources(stack, la_symbol, symbol);
                    new_sources = add_configs(new_sources, act, new_configs);
                }
            }
            free_nodes(action[symbol], tail);
            action[symbol] = NULL;

            state = state_to_resolve_conflicts(new_sources, symbol, level + 1);

            if (state == NULL)
                goto clean_up_and_return;

            la_shift_state[symbol] = state;

            p = Allocate_node();
            p -> value = state -> state_number;
            p -> next = NULL;
            action[symbol] = p;

            num_shift_actions++;
            action_list[symbol] = shift_root;
            shift_root = symbol;
        }
        else if (action[symbol] -> value < 0)
        {
            num_shift_actions++;
            action_list[symbol] = shift_root;
            shift_root = symbol;
        }
        else if (action[symbol] -> value > num_rules)
        {
            num_shift_actions++;
            (action[symbol] -> value) -= num_rules;
            action_list[symbol] = shift_root;
            shift_root = symbol;
        }
        else
        {
            num_reduce_actions++;
            action_list[symbol] = reduce_root;
            reduce_root = symbol;
        }
    }

    /*******************************************************************/
    /* We now iterate over the reduce actions in the domain of sources */
    /* and compute a default action.                                   */
    /*******************************************************************/
    default_rule = OMEGA;
    count = 0;
    for (act = sources.root; act != NIL; act = sources.list[act])
    {
        if (act >= 0 && act <= num_rules)
        {
            if (rule_count[act] > count)
            {
                count = rule_count[act];
                default_rule = act;
            }
        }
    }

    /*******************************************************************/
    /* By now, we are ready to create a new look-ahead state. The      */
    /* actions for the state are in the ACTION vector, and the         */
    /* constants: NUM_SHIFT_ACTIONS and NUM_REDUCE_ACTIONS indicate    */
    /* the number of shift and reduce actions in the ACTION vector.    */
    /* Note that the IN_STATE field of each look-ahead state created   */
    /* is initially set to the number associated with that state. If   */
    /* all the conflicts detected in the state, S, that requested the  */
    /* creation of a look-ahead state are resolved, then this field    */
    /* is updated with S.                                              */
    /* Otherwise, this field indicates that this look-ahead state is   */
    /* dangling - no other state point to it.                          */
    /*******************************************************************/
    state = (struct state_element *)
            talloc(sizeof(struct state_element));
    if (state == (struct state_element *) NULL)
        nospace(__FILE__, __LINE__);

    state -> link = la_state_root;
    la_state_root = state;

    max_la_state++;
    SHORT_CHECK(max_la_state);
    state -> symbol = la_symbol;
    state -> state_number = max_la_state;
    state -> in_state = max_la_state;   /* Initialize it to something! */

    /*******************************************************************/
    /* If there are any shift-actions in this state, we create a shift */
    /* map for them if one does not yet exist, otherwise, we reuse the */
    /* old existing one.                                               */
    /*******************************************************************/
    if (num_shift_actions > 0)
    {
        unsigned long hash_address;

        struct shift_header_type sh;

        struct state_element *p;

        /***************************************************************/
        /* In this loop, we compute the hash address as the number of  */
        /* shift actions, plus the sum of all the symbols on which a   */
        /* shift action is defined.  As a side effect, we also take    */
        /* care of some other issues. Shift actions which were encoded */
        /* to distinguish them from reduces action are decoded.        */
        /* The counters for shift and shift-reduce actions are updated.*/
        /* For all Shift actions to look-ahead states, the IN_STATE    */
        /* field of these look-ahead target states are updated.        */
        /***************************************************************/
        hash_address = num_shift_actions;   /* Initialize hash address */
        for (symbol = shift_root;
             symbol != NIL; symbol = action_list[symbol])
        {
            hash_address += symbol;

            if (action[symbol] -> value < 0)
                num_shift_reduces++;
            else if (action[symbol] -> value <= num_states)
                num_shifts++;
            else /* lookahead-shift */
                la_shift_state[symbol] -> in_state = max_la_state;
        }
        hash_address %= SHIFT_TABLE_SIZE;

        /***************************************************************/
        /* Search list associated with HASH_ADDRESS, and if the shift  */
        /* map in question is found, update the SHIFT, and SHIFT_NUMBER*/
        /* fields of the new Look-Ahead State.                         */
        /***************************************************************/
        for (p = shift_table[hash_address]; p != NULL; p = p -> next_shift)
        {  /* Search hash table for shift map */
            sh = p -> shift;
            if (sh.size == num_shift_actions)
            {
                for (i = 1; i <= num_shift_actions; i++)
                { /* compare shift maps */
                    if (SHIFT_ACTION(sh, i) !=
                        action[SHIFT_SYMBOL(sh, i)] -> value)
                       break;
                }

                if (i > num_shift_actions) /* are they equal ? */
                {
                    state -> shift = sh;
                    state -> shift_number = p -> shift_number;
                    break;
                }
            }
        }

        /***************************************************************/
        /* Shift map was not found.  We have to create a new one and   */
        /* insert it into the table.                                   */
        /***************************************************************/
        if (p == NULL)
        {
            num_shift_maps++;

            sh = Allocate_shift_map(num_shift_actions);

            state -> shift = sh;
            state -> shift_number = num_shift_maps;
            state -> next_shift = shift_table[hash_address];
            shift_table[hash_address] = state;

            for (symbol = shift_root, i = 1;
                 symbol != NIL; symbol = action_list[symbol], i++)
            {
                SHIFT_SYMBOL(sh, i) = symbol;
                SHIFT_ACTION(sh, i) = action[symbol] -> value;
            }
        }
    }
    else
    {
        state -> shift.size = 0;
        state -> shift_number = 0;
    }

    /*******************************************************************/
    /* Construct Reduce map.                                           */
    /* When SPACE or TIME tables are requested, no default actions are */
    /* taken.                                                          */
    /*******************************************************************/
    Build_reduce_map:
    {
        struct reduce_header_type red;
        int i;

        if (default_rule != OMEGA &&
            table_opt != OPTIMIZE_TIME &&
            table_opt != OPTIMIZE_SPACE &&
            default_opt != 0)
            num_reduce_actions -= rule_count[default_rule];

        num_reductions += num_reduce_actions;

        red = Allocate_reduce_map(num_reduce_actions);
        state -> reduce = red;
        for (symbol = reduce_root, i = 1;
             symbol != NIL; symbol = action_list[symbol])
        {
            if (default_opt == 0 ||
                action[symbol] -> value != default_rule ||
                table_opt == OPTIMIZE_TIME ||
                table_opt == OPTIMIZE_SPACE)
            {
                REDUCE_SYMBOL (red, i) = symbol;
                REDUCE_RULE_NO(red, i) = action[symbol] -> value;

                i++;
            }
        }

        REDUCE_SYMBOL(red, 0) = DEFAULT_SYMBOL;
        if (default_opt > 0)
            REDUCE_RULE_NO(red, 0) = default_rule;
        else
            REDUCE_RULE_NO(red, 0) = OMEGA;
    }


    /*******************************************************************/
    /* Release all space allocated to process this lookahead state and */
    /* return.                                                         */
    /*******************************************************************/
clean_up_and_return:
    free_sources(new_sources);
    for (symbol = symbol_root; symbol != NIL; symbol = symbol_list[symbol])
    {
        for (p = action[symbol]; p != NULL; tail = p, p = p -> next)
             ;
        if (action[symbol] != NULL)
            free_nodes(action[symbol], tail);
    }
    ffree(action);
    ffree(symbol_list);
    ffree(action_list);
    ffree(rule_count);
    ffree(look_ahead);
    ffree(la_shift_state);

    return state;
}


/***********************************************************************/
/*                              INIT_RMPSELF:                          */
/***********************************************************************/
/* This procedure is invoked when LALR_LEVEL > 1 to construct the      */
/* RMPSELF set which identifies the nonterminals that can right-most   */
/* produce themselves. It takes as argumen the map PRODUCES which      */
/* identifies for each nonterminal the set of nonterminals that it can */
/* right-most produce.                                                 */
/***********************************************************************/
void init_rmpself(SET_PTR produces)
{
    int nt;

    rmpself = Allocate_boolean_array(num_non_terminals);
    rmpself -= (num_terminals + 1);

    /*******************************************************************/
    /* Note that each element of the map produces is a boolean vector  */
    /* that is indexable in the range 1..num_non_terminals. Since each */
    /* nonterminal is offset by the value num_terminals (to distinguish*/
    /* it from the terminals),it must therefore be adjusted accordingly*/
    /* when dereferencing an element in the range of the produces map. */
    /*******************************************************************/
    for ALL_NON_TERMINALS(nt)
        rmpself[nt] = IS_IN_NTSET(produces, nt, nt - num_terminals);

    return;
}


/***********************************************************************/
/*                          INIT_LALRK_PROCESS:                        */
/***********************************************************************/
/* If LALR(k), k > 1, is requested, we may have to create more shift   */
/* maps. Initialize SHIFT_TABLE. Note that each element of SHIFT_TABLE */
/* is automatically initialized to NULL by CALLOC.                     */
/* (See STATE_TO_RESOLVE_CONFLICTS)                                    */
/* Second, we check whether or not the grammar is not LR(k) for any k  */
/* because there exist a nonterminal A such that                       */
/*                                                                     */
/*                     A =>+rm A                                       */
/*                                                                     */
/* Finally, allocate and compute the CYCLIC vector which identifies    */
/* states that can enter a cycle via transitions on nullable           */
/* nonterminals. If such a cyle exists, the grammar can also be        */
/* claimed to be not LR(k) for any k.                                  */
/***********************************************************************/
void init_lalrk_process(void)
{
    int state_no,
        symbol;

    not_lrk = FALSE;

    if (lalr_level > 1)
    {
        shift_table = (struct state_element **)
                      calloc(SHIFT_TABLE_SIZE,
                             sizeof(struct state_element *));
        if (shift_table == NULL)
            nospace(__FILE__, __LINE__);

        for ALL_NON_TERMINALS(symbol)
            not_lrk = not_lrk || rmpself[symbol];

        cyclic = Allocate_boolean_array(num_states + 1);
        index_of = Allocate_short_array(num_states + 1);
        stack = Allocate_short_array(num_states + 1);
        for ALL_STATES(state_no)
            index_of[state_no] = OMEGA;
        top = 0;
        for ALL_STATES(state_no)
        {
            if (index_of[state_no] == OMEGA)
                compute_cyclic(state_no);
            not_lrk = not_lrk || cyclic[state_no];
        }

        ffree(stack);
        ffree(index_of);

        sources = allocate_sources();

        visited.map = (struct node **)
                      calloc(num_states + 1, sizeof(struct node *));
        if (visited.map == NULL)
            nospace(__FILE__, __LINE__);

        visited.list = Allocate_short_array(num_states + 1);
        visited.root = NIL;
    }

    return;
}

/***********************************************************************/
/*                        EXIT_LALRK_PROCESS:                          */
/***********************************************************************/
/* Free all support structures that were allocated to help compute     */
/* additional lookahead.                                               */
/***********************************************************************/
void exit_lalrk_process(void)
{
    if (lalr_level > 1)
    {
        rmpself += (num_terminals + 1);
        ffree(rmpself);
        ffree(shift_table);
        ffree(cyclic);

        free_sources(sources);
        clear_visited();
        ffree(visited.map);
        ffree(visited.list);
    }

    return;
}


/***********************************************************************/
/*                            FREE_CONFLICT_SPACE                      */
/***********************************************************************/
/* If we had to report conflicts, free the SLR support structures.     */
/***********************************************************************/
void free_conflict_space(void)
{
    if (nt_items != NULL)
    {
        nt_items += (num_terminals + 1);
        ffree(nt_items);
        ffree(item_list);
    }

    return;
}


/***********************************************************************/
/*                           RESOLVE_CONFLICTS:                        */
/***********************************************************************/
/* If conflicts were detected and LALR(k) processing was requested,    */
/* where k > 1, then we attempt to resolve the conflicts by computing  */
/* more lookaheads.  Shift-Reduce conflicts are processed first,       */
/* followed by Reduce-Reduce conflicts.                                */
/***********************************************************************/
void resolve_conflicts(int state_no, struct node **action,
                       short *symbol_list, int symbol_root)
{
    struct shift_header_type sh;

    struct node *p,
                *tail;

    struct stack_element *q;

    struct state_element *state;

    int i,
        act,
        item_no,
        lhs_symbol,
        symbol;

    /*******************************************************************/
    /* Note that a shift action to a state "S" is encoded with the     */
    /* value (S+NUM_RULES) to help distinguish it from reduce actions. */
    /* Reduce actions lie in the range [0..NUM_RULES].   Shift-reduce  */
    /* actions lie in the range [-NUM_RULES..-1].                      */
    /*******************************************************************/
    sr_conflict_root = NULL;

    sh = shift[statset[state_no].shift_number];
    for (i = 1; i <= sh.size; i++)
    {
        symbol = SHIFT_SYMBOL(sh, i);

        if (single_productions_bit && action[symbol] != NULL)
            add_conflict_symbol(state_no, symbol);

        if (lalr_level > 1 && action[symbol] != NULL)
        {
            sources = clear_sources(sources);

            q = allocate_stack_element();
            q -> state_number = state_no;
            q -> size = 1;
            q -> previous = NULL;
            q -> next = NULL;

            act = SHIFT_ACTION(sh, i);
            if (act > 0)
                 sources = add_configs(sources, act + num_rules, q);
            else
                 sources = add_configs(sources, act, q);

            for (p = action[symbol]; p != NULL; tail = p, p = p -> next)
            {
                struct stack_element *new_configs;
                struct node *v,
                            *s;

                item_no = p -> value;
                act = item_table[item_no].rule_number;
                lhs_symbol = rules[act].lhs;

                clear_visited();

                v = lpgaccess(state_no, item_no);
                for (s = v; s != NULL; tail = s, s = s -> next)
                {
                    q = allocate_stack_element();
                    q -> state_number = s -> value;
                    q -> size = 1;
                    q -> previous = NULL;
                    q -> next = NULL;

                    new_configs = follow_sources(q, lhs_symbol, symbol);
                    if (new_configs == NULL)
                        free_stack_elements(q, q);
                    else
                    {
                        add_dangling_stack_element(q);
                        sources = add_configs(sources, act, new_configs);
                    }
                }

                free_nodes(v, tail);
            }

        /***************************************************************/
        /* The function STATE_TO_RESOLVE_CONFLICTS returns a pointer   */
        /* value to a STATE_ELEMENT which has been constructed to      */
        /* resolve the conflicts in question. If the value returned by */
        /* that function is NULL, then it was not possible to resolve  */
        /* the conflicts.  In any case, STATE_TO_RESOLVE_CONFLICTS     */
        /* frees the space that is used by the action map headed by    */
        /* ACTION_ROOT.                                                */
        /***************************************************************/
            state = state_to_resolve_conflicts(sources, symbol, 2);

            if (state != NULL)
            {
                state -> in_state = state_no;

                free_nodes(action[symbol], tail);
                action[symbol] = NULL;
            }
        }

        /***************************************************************/
        /* If unresolved shift-reduce conflicts are detected on symbol,*/
        /*  add them to the list of conflicts so they can be reported  */
        /* (if the CONFLICT option is on) and count them.              */
        /***************************************************************/
        if (action[symbol] != NULL)
        {
            act = SHIFT_ACTION(sh, i);

            for (p = action[symbol]; p != NULL; tail = p, p = p -> next)
            {
                if (conflicts_bit)
                {
                    struct sr_conflict_element *q;

                    q = (struct sr_conflict_element *)
                        allocate_conflict_element();
                    q -> state_number = act;
                    q -> item = p -> value;
                    q -> symbol = symbol;

                    q -> next = sr_conflict_root;
                    sr_conflict_root = q;
                }

                num_sr_conflicts++;
            }

            /***********************************************************/
            /* Remove reduce actions defined on symbol so as to give   */
            /* precedence to the shift.                                */
            /***********************************************************/
            free_nodes(action[symbol], tail);
            action[symbol] = NULL;
        }
    }

    /*******************************************************************/
    /* We construct a map from each action to a list of states as we   */
    /* did for the Shift-reduce conflicts. A boolean vector ITEM_SEEN  */
    /* is used to prevent duplication of actions. This problem does    */
    /* not occur with Shift-Reduce conflicts.                          */
    /*******************************************************************/
    rr_conflict_root = NULL;

    for (symbol = symbol_root;
         symbol != NIL; symbol = symbol_list[symbol])
    {
        if (action[symbol] != NULL)
        {
            if (single_productions_bit && action[symbol] -> next != NULL)
                add_conflict_symbol(state_no, symbol);

            if (lalr_level > 1 && action[symbol] -> next != NULL)
            {
                sources = clear_sources(sources);

                for (p = action[symbol]; p != NULL; tail = p, p = p -> next)
                {
                    struct stack_element *new_configs;
                    struct node *v,
                                *s;

                    item_no = p -> value;
                    act = item_table[item_no].rule_number;
                    lhs_symbol = rules[act].lhs;

                    clear_visited();

                    v = lpgaccess(state_no, item_no);
                    for (s = v; s != NULL; tail = s, s = s -> next)
                    {
                        q = allocate_stack_element();
                        q -> state_number = s -> value;
                        q -> size = 1;
                        q -> previous = NULL;
                        q -> next = NULL;

                        new_configs = follow_sources(q, lhs_symbol, symbol);
                        if (new_configs == NULL)
                            free_stack_elements(q, q);
                        else
                        {
                            add_dangling_stack_element(q);
                            sources = add_configs(sources, act, new_configs);
                        }
                    }

                    free_nodes(v, tail);
                }

        /***************************************************************/
        /*     STATE_TO_RESOLVE_CONFLICTS will return a pointer to a   */
        /* STATE_ELEMENT if the conflicts were resolvable with more    */
        /* lookaheads, otherwise, it returns NULL.                     */
        /***************************************************************/
                state = state_to_resolve_conflicts(sources, symbol, 2);

                if (state != NULL)
                {
                    state -> in_state = state_no;

                    free_nodes(action[symbol], tail);
                    action[symbol] = NULL;
                }
            }

            /***********************************************************/
            /* If unresolved reduce-reduce conflicts are detected on   */
            /* symbol, add them to the list of conflicts so they can be*/
            /* reported (if the CONFLICT option is on) and count them. */
            /***********************************************************/
            if (action[symbol] != NULL)
            {
                act = action[symbol] -> value;

                for (p = action[symbol] -> next;
                     p != NULL; tail = p, p = p -> next)
                {
                    if (conflicts_bit)
                    {
                        struct rr_conflict_element *q;

                        q = (struct rr_conflict_element *)
                            allocate_conflict_element();
                        q -> symbol = symbol;
                        q -> item1  = act;
                        q -> item2  = p -> value;

                        q -> next = rr_conflict_root;
                        rr_conflict_root = q;
                    }

                    num_rr_conflicts++;
                }

                /***********************************************************/
                /* Remove all reduce actions that are defined on symbol    */
                /* except the first one. That rule is the one with the     */
                /* longest right-hand side that was associated with symbol.*/
                /* See code in MKRED.C.                                    */
                /***********************************************************/
                if (action[symbol] -> next != NULL)
                {
                    free_nodes(action[symbol] -> next, tail);
                    action[symbol] -> next = NULL;
                }
            }
        }
    }

    /*******************************************************************/
    /* If any unresolved conflicts were detected, process them.        */
    /*******************************************************************/
    if (sr_conflict_root != NULL || rr_conflict_root != NULL)
        process_conflicts(state_no);

    free_dangling_stack_elements();

    return;
}


/***********************************************************************/
/*                           CREATE_LASTATS:                           */
/***********************************************************************/
/* Transfer the look-ahead states to their permanent destination, the  */
/* array LASTATS and update the original automaton with the relevant   */
/* transitions into the lookahead states.                              */
/***********************************************************************/
void create_lastats(void)
{
    struct state_element **new_shift_actions,
                          *p;

    struct shift_header_type sh;

    short *shift_action,
          *shift_list,
          *shift_count,
          *state_list;

    int shift_root,
        state_root,
        shift_size,
        i,
        symbol,
        state_no,
        shift_no;

    /*******************************************************************/
    /* Allocate LASTATS structure to permanently construct lookahead   */
    /* states and reallocate SHIFT map as we may have to construct     */
    /* new shift maps.                                                 */
    /*******************************************************************/
    lastats = (struct lastats_type *)
              calloc(max_la_state - num_states,
                     sizeof(struct lastats_type));
    if (lastats == NULL)
        nospace(__FILE__, __LINE__);
    lastats -= (num_states + 1);

    shift = (struct shift_header_type *)
            realloc(shift,
                    (max_la_state + 1) * sizeof(struct shift_header_type));
    if (shift == NULL)
        nospace(__FILE__, __LINE__);

    /*******************************************************************/
    /* Allocate temporary space used to construct final lookahead      */
    /* states.                                                         */
    /*******************************************************************/
    new_shift_actions = (struct state_element **)
                        calloc(num_states + 1, sizeof(struct state_element *));
    if (new_shift_actions == NULL)
        nospace(__FILE__, __LINE__);

    shift_action = Allocate_short_array(num_terminals + 1);
    shift_list   = Allocate_short_array(num_terminals + 1);
    shift_count  = Allocate_short_array(max_la_state + 1);
    state_list   = Allocate_short_array(max_la_state + 1);

    /*******************************************************************/
    /* The array shift_action will be used to construct a shift map    */
    /* for a given state. It is initialized here to the empty map.     */
    /* The array shift_count is used to count how many references      */
    /* there are to each shift map.                                    */
    /*******************************************************************/
    for ALL_TERMINALS(symbol)
        shift_action[symbol] = OMEGA;

    for (i = 0; i <= (int) max_la_state; i++)
        shift_count[i] = 0;

    for ALL_STATES(state_no)
        shift_count[statset[state_no].shift_number]++;

    /*******************************************************************/
    /* Traverse the list of lookahead states and initialize the        */
    /* final lastat element appropriately. Also, construct a mapping   */
    /* from each relevant initial state into the list of lookahead     */
    /* states into which it can shift. We also keep track of these     */
    /* initial states in a list headed by state_root.                  */
    /*******************************************************************/
    state_root = NIL;
    for (p = la_state_root; p != NULL; p = p -> link)
    {
        lastats[p -> state_number].in_state = p -> in_state;
        lastats[p -> state_number].shift_number = p -> shift_number;
        lastats[p -> state_number].reduce = p -> reduce;
        if (p -> shift.size != 0)
            shift[p -> shift_number] = p -> shift;

        state_no = p -> in_state;
        if (state_no <= (int) num_states)
        {
            if (new_shift_actions[state_no] == NULL)
            {
                state_list[state_no] = state_root;
                state_root = state_no;
            }
            p -> next_shift = new_shift_actions[state_no];
            new_shift_actions[state_no] = p;
        }
    }

    /*******************************************************************/
    /* We now traverse the list of initial states that can shift into  */
    /* lookahead states and update their shift map appropriately.      */
    /*******************************************************************/
    for (state_no = state_root;
         state_no != NIL; state_no = state_list[state_no])
    {
        /***************************************************************/
        /* Copy the shift map associated with STATE_NO into the direct */
        /* access map SHIFT_ACTION.                                    */
        /***************************************************************/
        shift_no = statset[state_no].shift_number;
        sh = shift[shift_no];
        shift_root = NIL;
        for (i = 1; i <= sh.size; i++)
        {
            symbol = SHIFT_SYMBOL(sh, i);
            shift_action[symbol] = SHIFT_ACTION(sh, i);

            shift_list[symbol] = shift_root;
            shift_root = symbol;
        }

        /***************************************************************/
        /* Add the lookahead shift transitions to the initial shift    */
        /* map.                                                        */
        /***************************************************************/
        shift_size = sh.size;
        for (p = new_shift_actions[state_no]; p != NULL; p = p -> next_shift)
        {
            if (shift_action[p -> symbol] == OMEGA)
            {
                shift_size++;

                shift_list[p -> symbol] = shift_root;
                shift_root = p -> symbol;
            }
            else if (shift_action[p -> symbol] < 0)
                num_shift_reduces--;
            else
                num_shifts--;

            shift_action[p -> symbol] = p -> state_number;
        }

        /***************************************************************/
        /* There are two conditions under which we have to construct   */
        /* a brand new shift map:                                      */
        /*     1. The initial shift map was shared with other states.  */
        /*     2. The updated shift map contains more elements than    */
        /*        the initial one.                                     */
        /***************************************************************/
        if (shift_count[shift_no] > 1)
        {
            shift_count[shift_no]--;
            num_shift_maps++;
            sh = Allocate_shift_map(shift_size);
            shift[num_shift_maps] = sh;
            statset[state_no].shift_number = num_shift_maps;
        }
        else if (shift_size > sh.size)
        {
            sh = Allocate_shift_map(shift_size);
            shift[shift_no] = sh;
        }

        /***************************************************************/
        /* Reconstruct the relevant shift map.                         */
        /***************************************************************/
        for (symbol = shift_root, i = 1;
             symbol != NIL;
             shift_action[symbol] = OMEGA, symbol = shift_list[symbol], i++)
        {
            SHIFT_SYMBOL(sh, i) = symbol;
            SHIFT_ACTION(sh, i) = shift_action[symbol];
        }
    }

    /*******************************************************************/
    /* Free all local temporary structures and return.                 */
    /*******************************************************************/
    ffree(new_shift_actions);
    ffree(shift_action);
    ffree(shift_list);
    ffree(shift_count);
    ffree(state_list);

    return;
}
