static char hostfile[] = __FILE__;

#include "common.h"
#include "header.h"

static void mklr0(void);
static struct state_element *lr0_state_map(struct node *kernel);

/****************************************************************************/
/* STATE_ELEMENT is used to represent states. Each state is mapped into a   */
/* unique number. The components QUEUE and LINK are auxiliary:              */
/*   QUEUE is used to form a sequential linked-list of the states ordered   */
/* STATE_NUMBER and identified by the variable STATE_ROOT.                  */
/*   LINK is used to resolve collisions in hashing the states.              */
/* NEXT_SHIFT is used to resolve collisions in hashing SHIFT maps.          */
/****************************************************************************/
struct state_element
{
    struct state_element     *link,
                             *queue,
                             *next_shift;
    struct node              *kernel_items,
                             *complete_items;
    struct shift_header_type lr0_shift;
    struct goto_header_type  lr0_goto;
    short                    shift_number,
                             state_number;
};

static struct state_element **state_table,
                            **shift_table,
                             *state_root,
                             *state_tail;

static short *shift_action;

static struct goto_header_type  no_gotos_ptr;
static struct shift_header_type no_shifts_ptr;

/*****************************************************************************/
/*                              MKSTATS:                                     */
/*****************************************************************************/
/* In this procedure, we first construct the LR(0) automaton.                */
/*****************************************************************************/
void mkstats(void)
{
    int j;

    no_gotos_ptr.size = 0;     /* For states with no GOTOs */
    no_gotos_ptr.map  = NULL;

    no_shifts_ptr.size = 0;    /* For states with no SHIFTs */
    no_shifts_ptr.map  = NULL;

    mklr0();

    if (error_maps_bit &&
        (table_opt == OPTIMIZE_TIME || table_opt == OPTIMIZE_SPACE))
        produce();

    /**********************************************************************/
    /* Free space trapped by the CLOSURE and CLITEMS maps.                */
    /**********************************************************************/
    for ALL_NON_TERMINALS(j)
    {
        struct node *p, *q;

        q = clitems[j];
        if (q != NULL)
        {
            p = q -> next;
            free_nodes(p, q);
        }

        q = closure[j];
        if (q != NULL)
        {
            p = q -> next;
            free_nodes(p, q);
        }
    }

    closure += (num_terminals + 1);
    ffree(closure);
    clitems += (num_terminals + 1);
    ffree(clitems);

    return;
}


/*****************************************************************************/
/*                               MKLR0:                                      */
/*****************************************************************************/
/* This procedure constructs an LR(0) automaton.                             */
/*****************************************************************************/
static void mklr0(void)
{
    /*****************************************************************/
    /* STATE_TABLE is the array used to hash the states. States are  */
    /* identified by their Kernel set of items. Hash locations are   */
    /* computed for the states. As states are inserted in the table, */
    /* they are threaded together via the QUEUE component of         */
    /* STATE_ELEMENT. The variable STATE_ROOT points to the root of  */
    /* the thread, and the variable STATE_TAIL points to the tail.   */
    /*                                                               */
    /*   After constructing a state, Shift and Goto actions are      */
    /* defined on that state based on a partition of the set of items*/
    /* in that state. The partitioning is based on the symbols       */
    /* following the dot in the items. The array PARTITION is used   */
    /* for that partitioning. LIST and ROOT are used to construct    */
    /* temporary lists of symbols in a state on which Shift or Goto  */
    /* actions are defined.                                          */
    /*   NT_LIST and NT_ROOT are used to build temporary lists of    */
    /* non-terminals.                                                */
    /*****************************************************************/

    struct node *p,
                *q,
                *r,
                *new_item,
                *tail,
                *item_ptr,
                **partition,
                *closure_root,
                *closure_tail;

    struct state_element *state,
                         *new_state;

    BOOLEAN end_node;

    int goto_size,
        shift_size,
        i,
        state_no,
        next_item_no,
        item_no,
        symbol,
        rule_no,
        shift_root,
        nt_root,
        root;

    struct goto_header_type go_to;

    short *shift_list,
          *nt_list,
          *list;

    /******************************************************************/
    /* Set up a a pool of temporary space.                            */
    /******************************************************************/
    reset_temporary_space();

    list = Allocate_short_array(num_symbols + 1);
    shift_action = Allocate_short_array(num_terminals + 1);
    shift_list = Allocate_short_array(num_terminals + 1);
    nt_list = Allocate_short_array(num_non_terminals);
    nt_list -= (num_terminals + 1);
    partition = (struct node **)
                calloc(num_symbols + 1, sizeof(struct node *));
    if (partition == NULL)
        nospace(__FILE__, __LINE__);
    state_table = (struct state_element **)
                  calloc(STATE_TABLE_SIZE, sizeof(struct state_element *));
    if (state_table == NULL)
        nospace(__FILE__, __LINE__);
    shift_table = (struct state_element **)
                  calloc(SHIFT_TABLE_SIZE, sizeof(struct state_element *));
    if (shift_table == NULL)
        nospace(__FILE__, __LINE__);

/* INITIALIZATION -----------------------------------------------------------*/
    goto_size = 0;
    shift_size = 0;

    state_root = NULL;

    for (i = 0; i <= num_terminals; i++)
        shift_action[i] = OMEGA;

    nt_root = NIL;
    for ALL_NON_TERMINALS(i)
        nt_list[i] = OMEGA;

   /* PARTITION, STATE_TABLE and SHIFT_TABLE are initialized by calloc */

/* END OF INITIALIZATION ----------------------------------------------------*/

    /*****************************************************************/
    /* Kernel of the first state consists of the first items in each */
    /* rule produced by Accept non-terminal.                         */
    /*****************************************************************/
    q = NULL;
    for (end_node = ((p = clitems[accept_image]) == NULL);
         ! end_node; /* Loop over circular list */
         end_node = (p == clitems[accept_image]))
    {
        p = p -> next;

        new_item = Allocate_node();
        new_item -> value = p -> value;
        new_item -> next = q;
        q = new_item;
    }

    /*****************************************************************/
    /* Insert first state in STATE_TABLE and keep constructing states*/
    /* until we no longer can.                                       */
    /*****************************************************************/

    for (state = lr0_state_map(q); /* insert initial state */
         state != NULL; /* and process next state until no more */
         state = state -> queue)
    {
        /******************************************************************/
        /* Now we construct a list of all non-terminals that can be       */
        /* introduced in this state through closure.  The CLOSURE of each */
        /* non-terminal has been previously computed in MKFIRST.          */
        /******************************************************************/
        for (q = state -> kernel_items;
             q != NULL; /* iterate over kernel set of items */
             q = q -> next)
        {
            item_no = q -> value;
            symbol = item_table[item_no].symbol;  /* symbol after dot */
            if (symbol IS_A_NON_TERMINAL)     /* Dot symbol */
            {
                if (nt_list[symbol] == OMEGA) /* not yet seen */
                {
                    nt_list[symbol] = nt_root;
                    nt_root = symbol;

                    for (end_node = ((p = closure[symbol]) == NULL);
                         ! end_node; /* Loop over circular list */
                         end_node = (p == closure[symbol]))
                    {   /* add its closure to list */
                        p = p -> next;

                        if (nt_list[p -> value] == OMEGA)
                        {
                            nt_list[p -> value] = nt_root;
                            nt_root = p -> value;
                        }
                    }
                }
            }
        }

        /*******************************************************************/
        /*   We now construct lists of all start items that the closure    */
        /* non-terminals produce.  A map from each non-terminal to its set */
        /* start items has previously been computed in MKFIRST. (CLITEMS)  */
        /* Empty items are placed directly in the state, whereas non_empty */
        /* items are placed in a temporary list rooted at CLOSURE_ROOT.    */
        /*******************************************************************/
        closure_root = NULL; /* used to construct list of closure items */
        for (symbol = nt_root; symbol != NIL;
             nt_list[symbol] = OMEGA, symbol = nt_root)
        {
            nt_root = nt_list[symbol];

            for (end_node = ((p = clitems[symbol]) == NULL);
                 ! end_node;  /* Loop over circular list */
                 end_node = (p == clitems[symbol]))
            {
                p = p -> next;

                item_no = p -> value;
                new_item = Allocate_node();
                new_item -> value = item_no;

                if (item_table[item_no].symbol == empty) /* complete item */
                {
                    /* Add to COMPLETE_ITEMS set in state */
                    new_item -> next = state -> complete_items;
                    state -> complete_items = new_item;
                }
                else
                {   /* closure item, add to closure list */
                    if (closure_root == NULL)
                        closure_root = new_item;
                    else
                        closure_tail -> next = new_item;
                    closure_tail = new_item;
                }
            }
        }

        if  (closure_root != NULL) /* any non-complete closure items? */
        {
            /* construct list of them and kernel items */
            closure_tail -> next = state -> kernel_items;
            item_ptr = closure_root;
        }
        else  /* else just consider kernel items */
            item_ptr = state -> kernel_items;

        /*******************************************************************/
        /*   In this loop, the PARTITION map is constructed. At this point,*/
        /* ITEM_PTR points to all the non_complete items in the closure of */
        /* the state, plus all the kernel items.  We note that the kernel  */
        /* items may still contain complete-items, and if any is found, the*/
        /* COMPLETE_ITEMS list is updated.                                 */
        /*******************************************************************/
        root = NIL;
        for (; item_ptr != NULL; item_ptr = item_ptr -> next)
        {
            item_no = item_ptr -> value;
            symbol = item_table[item_no].symbol;
            if (symbol != empty)  /* incomplete item */
            {
                next_item_no = item_no + 1;
                
                if (partition[symbol] == NULL)
                {   /* PARTITION not defined on symbol */
                    list[symbol] = root;  /* add to list */
                    root = symbol;
                    if (symbol IS_A_TERMINAL) /* Update transition count */
                        shift_size++;
                    else
                        goto_size++;
                }
                for (p = partition[symbol];
                     p != NULL;
                     tail = p, p = p -> next)
                {
                    if (p -> value > next_item_no)
                        break;
                }

                r = Allocate_node();
                r -> value = next_item_no;
                r -> next = p;

                if (p == partition[symbol]) /* Insert at beginning */
                    partition[symbol] = r;
                else
                    tail -> next = r;
            }
            else /* Update complete item set with item from kernel */
            {
                 p = Allocate_node();
                 p -> value = item_no;
                 p -> next = state -> complete_items;
                 state -> complete_items = p;
            }
        }
        if (closure_root != NULL)
            free_nodes(closure_root, closure_tail);

        /*******************************************************************/
        /* We now iterate over the set of partitions and update the state  */
        /* automaton and the transition maps: SHIFT and GOTO. Each         */
        /* partition represents the kernel of a state.                     */
        /*******************************************************************/
        if (goto_size > 0)
        {
            go_to = Allocate_goto_map(goto_size);
            state -> lr0_goto = go_to;
        }
        else
            state -> lr0_goto = no_gotos_ptr;

        shift_root = NIL;
        for (symbol = root;
             symbol != NIL; /* symbols on which transition is defined */
             symbol = list[symbol])
        {
            short action = OMEGA;

            /*****************************************************************/
            /* If the partition contains only one item, and it is adequate   */
            /* (i.e. the dot immediately follows the last symbol), and       */
            /* READ-REDUCE is requested, a new state is not created, and the */
            /* action is marked as a Shift-reduce or a Goto-reduce. Otherwise*/
            /* if a state with that kernel set does not yet exist, we create */
            /* it.                                                           */
            /*****************************************************************/
            q = partition[symbol]; /* kernel of a new state */
            if (read_reduce_bit && q -> next == NULL)
            {
                item_no = q -> value;
                if (item_table[item_no].symbol == empty)
                {
                    rule_no = item_table[item_no].rule_number;
                    if (rules[rule_no].lhs != accept_image)
                    {
                        action = -rule_no;
                        free_nodes(q, q);
                    }
                }
            }

            if (action == OMEGA) /* Not a Read-Reduce action */
            {
                new_state = lr0_state_map(q);
                action = new_state -> state_number;
            }

            /****************************************************************/
            /* At this stage, the partition list has been freed (for an old */
            /* state or an ADEQUATE item), or used (for a new state).  The  */
            /* PARTITION field involved should be reset.                    */
            /****************************************************************/
            partition[symbol] = NULL;           /* To be reused */

            /*****************************************************************/
            /* At this point, ACTION contains the value of the state to Shift*/
            /* to, or rule to Read-Reduce on. If the symbol involved is a    */
            /* terminal, we update the Shift map; else, it is a non-terminal */
            /* and we update the Goto map.                                   */
            /* Shift maps are constructed temporarily in SHIFT_ACTION.       */
            /* Later, they are inserted into a map of unique Shift maps, and */
            /* shared by states that contain identical shifts.               */
            /* Since the lookahead set computation is based on the GOTO maps,*/
            /* all these maps and their element maps should be kept as       */
            /* separate entities.                                            */
            /*****************************************************************/
            if (symbol IS_A_TERMINAL) /* terminal? add to SHIFT map */
            {
                shift_action[symbol] = action;
                shift_list[symbol] = shift_root;
                shift_root = symbol;
                if (action > 0)
                    num_shifts++;
                else
                    num_shift_reduces++;
            }

            /*****************************************************************/
            /* NOTE that for Goto's we update the field LA_PTR of GOTO. This */
            /* field will be used later in the routine MKRDCTS to point to a */
            /* look-ahead set.                                               */
            /*****************************************************************/
            else
            {
                GOTO_SYMBOL(go_to, goto_size) = symbol; /* symbol field */
                GOTO_ACTION(go_to, goto_size) = action; /* state field  */
                GOTO_LAPTR(go_to,  goto_size) = OMEGA;  /* la_ptr field */
                goto_size--;
                if (action > 0)
                    num_gotos++;
                else
                    num_goto_reduces++;
            }
        }

        /*******************************************************************/
        /* We are now going to update the set of Shift-maps. Ths idea is   */
        /* to do a look-up in a hash table based on SHIFT_TABLE to see if  */
        /* the Shift map associated with the current state has already been*/
        /* computed. If it has, we simply update the SHIFT_NUMBER and the  */
        /* SHIFT field of the current state. Otherwise, we allocate and    */
        /* construct a SHIFT_ELEMENT map, update the current state, and    */
        /* add it to the set of Shift maps in the hash table.              */
        /*   Note that the SHIFT_NUMBER field in the STATE_ELEMENTs could  */
        /* have been factored out and associated instead with the          */
        /* SHIFT_ELEMENTs. That would have saved some space, but only in   */
        /* the short run. This field was purposely stored in the           */
        /* STATE_ELEMENTs, because once the states have been constructed,  */
        /* they are not kept, whereas the SHIFT_ELEMENTs are kept.         */
        /*    One could have also threaded through the states that contain */
        /* original shift maps so as to avoid duplicate assignments in     */
        /* creating the SHIFT map later. However, this would have          */
        /* increased the storage requirement, and would probably have saved*/
        /* (at most) a totally insignificant amount of time.               */
        /*******************************************************************/
update_shift_maps:
        {
            unsigned long hash_address;

            struct shift_header_type sh;

            struct state_element *p;

            hash_address = shift_size;
            for (symbol = shift_root;
                 symbol != NIL; symbol = shift_list[symbol])
            {
                hash_address += ABS(shift_action[symbol]);
            }
            hash_address %= SHIFT_TABLE_SIZE;

            for (p = shift_table[hash_address];
                 p != NULL; /* Search has table for shift map */
                 p = p -> next_shift)
            {
                sh = p -> lr0_shift;
                if (sh.size == shift_size)
                {
                    for (i = 1; i <= shift_size; i++) /* Compare shift maps */
                    {
                        if (SHIFT_ACTION(sh, i) !=
                            shift_action[SHIFT_SYMBOL(sh, i)])
                            break;
                    }

                    if (i > shift_size)  /* Are they equal ? */
                    {
                        state -> lr0_shift    = sh;
                        state -> shift_number = p -> shift_number;
                        for (symbol = shift_root;
                             symbol != NIL; symbol = shift_list[symbol])
                        { /* Clear SHIFT_ACTION */
                            shift_action[symbol] = OMEGA;
                        }
                        shift_size = 0;   /* Reset for next round */
                        goto leave_update_shift_maps;
                    }
                }
            }

            if (shift_size > 0)
            {
                sh = Allocate_shift_map(shift_size);
                num_shift_maps++;
                state -> shift_number = num_shift_maps;
            }
            else
            {
                state -> shift_number = 0;
                sh = no_shifts_ptr;
            }

            state -> lr0_shift = sh;
            state -> next_shift = shift_table[hash_address];
            shift_table[hash_address] = state;

            for (symbol = shift_root;
                 symbol != NIL; symbol = shift_list[symbol])
            {
                SHIFT_SYMBOL(sh, shift_size) = symbol;
                SHIFT_ACTION(sh, shift_size) = shift_action[symbol];
                shift_action[symbol] = OMEGA;
                shift_size--;
            }
        } /*end update_shift_maps */

leave_update_shift_maps:;
    }

    /*********************************************************************/
    /* Construct STATSET, a "compact" and final representation of        */
    /* State table, and SHIFT which is a set of all shift maps needed.   */
    /* NOTE that assignments to elements of SHIFT may occur more than    */
    /* once, but that's ok. It is probably faster to  do that than to    */
    /* set up an elaborate scheme to avoid the multiple assignment which */
    /* may in fact cost more.  Look at it this way: it is only a pointer */
    /* assignment, namely a Load and a Store.                            */
    /* Release all NODEs used by  the maps CLITEMS and CLOSURE.          */
    /*********************************************************************/
    {
        struct state_element *p;

    /*********************************************************************/
    /* If the grammar is LALR(k), k > 1, more states may be added and    */
    /* the size of the shift map increased.                              */
    /*********************************************************************/
        shift = (struct shift_header_type *)
                calloc(num_states + 1, sizeof(struct shift_header_type));
        if (shift == NULL)
            nospace(__FILE__, __LINE__);
        shift[0] = no_shifts_ptr;        /* MUST be initialized for LALR(k) */
        statset = (struct statset_type *)
                  calloc(num_states + 1, sizeof(struct statset_type));
        if (statset == NULL)
            nospace(__FILE__, __LINE__);

        for (p = state_root; p != NULL; p = p -> queue)
        {
            state_no = p -> state_number;
            statset[state_no].kernel_items = p -> kernel_items;
            statset[state_no].complete_items = p -> complete_items;
            shift[p -> shift_number] = p -> lr0_shift;
            statset[state_no].shift_number = p -> shift_number;
            statset[state_no].go_to = p -> lr0_goto;
        }
    }

    ffree(list);
    ffree(shift_action);
    ffree(shift_list);
    nt_list += (num_terminals + 1);
    ffree(nt_list);
    ffree(partition);
    ffree(state_table);
    ffree(shift_table);

    return;
}


/*****************************************************************************/
/*                            LR0_STATE_MAP:                                 */
/*****************************************************************************/
/* LR0_STATE_MAP takes as an argument a pointer to a kernel set of items. If */
/* no state based on that kernel set already exists, then a new one is       */
/* created and added to STATE_TABLE. In any case, a pointer to the STATE of  */
/* the KERNEL is returned.                                                   */
/*****************************************************************************/
static struct state_element *lr0_state_map(struct node *kernel)
{
    unsigned long hash_address = 0;
    struct node *p;
    struct state_element *state_ptr,
                         *ptr;

    /*********************************************/
    /*       Compute the hash address.           */
    /*********************************************/
    for (p = kernel; p != NULL; p = p -> next)
    {
        hash_address += p -> value;
    }
    hash_address %= STATE_TABLE_SIZE;

    /*************************************************************************/
    /* Check whether a state is already defined by the KERNEL set.           */
    /*************************************************************************/
    for (state_ptr = state_table[hash_address];
         state_ptr != NULL; state_ptr = state_ptr -> link)
    {
        struct node *q,
                    *r;

        for (p = state_ptr -> kernel_items, q = kernel;
             p != NULL && q != NULL;
             p = p -> next, r = q, q = q -> next)
        {
            if (p -> value != q -> value)
                break;
        }

        if (p == q)  /* Both P and Q are NULL? */
        {
            free_nodes(kernel, r);
            return(state_ptr);
        }
    }


    /*******************************************************************/
    /* Add a new state based on the KERNEL set.                        */
    /*******************************************************************/
    ptr = (struct state_element *) talloc(sizeof(struct state_element));
    if (ptr == (struct state_element *) NULL)
        nospace(__FILE__, __LINE__);

    num_states++;
    SHORT_CHECK(num_states);
    ptr -> queue = NULL;
    ptr -> kernel_items = kernel;
    ptr -> complete_items = NULL;
    ptr -> state_number = num_states;
    ptr -> link = state_table[hash_address];
    state_table[hash_address] = ptr;
    if (state_root == NULL)
        state_root = ptr;
    else
        state_tail -> queue = ptr;

    state_tail = ptr;

    return(ptr);
}
