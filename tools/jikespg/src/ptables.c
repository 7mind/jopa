static char hostfile[] = __FILE__;

#include "common.h"
#include "header.h"

struct action_element
{
    struct action_element *next;
    short count,
          action;
};

/****************************************************************************/
/*                            PROCESS_SHIFT_ACTIONS:                        */
/****************************************************************************/
/* The array ACTION_COUNT is used to construct a map from each terminal     */
/* into the set (list) of actions defined on that terminal. A count of the  */
/* number of occurences of each action in the automaton is kept.            */
/* This procedure is invoked with a specific shift map which it processes   */
/* and updates the ACTION_COUNT map accordingly.                            */
/****************************************************************************/
static void process_shift_actions(struct action_element **action_count,
                                  int shift_no)
{
    struct shift_header_type sh;
    struct action_element *q;

    int symbol,
        act,
        i;

    sh = shift[shift_no];
    for (i = 1; i <= sh.size; i++)
    {
        symbol = SHIFT_SYMBOL(sh, i);
        act = SHIFT_ACTION(sh, i);
        for (q = action_count[symbol]; q != NULL; q = q -> next)
        {
            if (q -> action == act)
                break;
        }

        if (q == NULL) /* new action not yet seen */
        {
            q = (struct action_element *)
                talloc(sizeof(struct action_element));
            if (q == NULL)
                nospace(__FILE__, __LINE__);

            q -> action = act;
            q -> count = 1;
            q -> next = action_count[symbol];
            action_count[symbol] = q;
        }
        else (q -> count)++;
    }

    return;
}


/****************************************************************************/
/*                            COMPUTE_SHIFT_DEFAULT:                        */
/****************************************************************************/
/* This procedure updates the vector SHIFTDF, indexable by the terminals in */
/* the grammar. Its task is to assign to each element of SHIFTDF, the action*/
/* most frequently defined on the symbol in question.                       */
/****************************************************************************/
static void compute_shift_default(void)
{
    struct action_element **action_count,
                           *q;

    int state_no,
        symbol,
        default_action,
        max_count,
        shift_count = 0,
        shift_reduce_count = 0;

    /******************************************************************/
    /* Set up a a pool of temporary space.                            */
    /******************************************************************/
    reset_temporary_space();

    shiftdf = Allocate_short_array(num_terminals + 1);

    action_count = (struct action_element **)
                   calloc(num_terminals + 1,
                          sizeof(struct action_element *));
    if (action_count == NULL)
        nospace(__FILE__, __LINE__);

    /*******************************************************************/
    /* For each state, invoke PROCESS_SHIFT_ACTIONS to process the     */
    /* shift map associated with that state.                           */
    /*******************************************************************/
    for ALL_STATES(state_no)
    {
        process_shift_actions(action_count,
                              statset[state_no].shift_number);
    }

    for ALL_LA_STATES(state_no)
    {
        process_shift_actions(action_count,
                              lastats[state_no].shift_number);
    }

    /*********************************************************************/
    /* We now iterate over the ACTION_COUNT mapping, and for each        */
    /* terminal t, initialize SHIFTDF[t] to the action that is most      */
    /* frequently defined on t.                                          */
    /*********************************************************************/
    for ALL_TERMINALS(symbol)
    {
        max_count = 0;
        default_action = 0;

        for (q = action_count[symbol]; q != NULL; q = q -> next)
        {
            if (q -> count > max_count)
            {
                max_count = q -> count;
                default_action = q -> action;
            }
        }

        shiftdf[symbol] = default_action;
        if (default_action > 0)  /* A state number ? */
            shift_count += max_count;
        else
            shift_reduce_count += max_count;
    }

    sprintf(msg_line,
            "Number of Shift entries saved by default: %d",
            shift_count);
    PRNT(msg_line);

    sprintf(msg_line,
            "Number of Shift/Reduce entries saved by default: %d",
            shift_reduce_count);
    PRNT(msg_line);

    num_shifts -= shift_count;
    num_shift_reduces -= shift_reduce_count;
    num_entries = num_entries - shift_count - shift_reduce_count;

    ffree(action_count);

    return;
}


/*****************************************************************************/
/*                             COMPUTE_GOTO_DEFAULT:                         */
/*****************************************************************************/
/*   COMPUTE_GOTO_DEFAULT constructs the vector GOTODEF, which is indexed by */
/* the non-terminals in the grammar. Its task is to assign to each element   */
/* of the array the Action which is most frequently defined on the symbol in */
/* question, and remove all such actions from the state automaton.           */
/*****************************************************************************/
static void compute_goto_default(void)
{
    struct goto_header_type go_to;

    struct action_element **action_count,
                           *q;

    int state_no,
        symbol,
        default_action,
        act,
        i,
        k,
        max_count,
        goto_count = 0,
        goto_reduce_count = 0;

    /******************************************************************/
    /* Set up a a pool of temporary space.                            */
    /******************************************************************/
    reset_temporary_space();

    gotodef = Allocate_short_array(num_non_terminals);
    gotodef -= (num_terminals + 1);

    action_count = (struct action_element **)
                   calloc(num_non_terminals,
                          sizeof(struct action_element *));
    action_count -= (num_terminals + 1);
    if (action_count == NULL)
        nospace(__FILE__, __LINE__);

    /*******************************************************************/
    /* The array ACTION_COUNT is used to construct a map from each     */
    /* non-terminal into the set (list) of actions defined on that     */
    /* non-terminal. A count of how many occurences of each action     */
    /* is also kept.                                                   */
    /* This loop is analoguous to the loop in PROCESS_SHIFT_ACTIONS.   */
    /*******************************************************************/
    for ALL_STATES(state_no)
    {
        go_to = statset[state_no].go_to;
        for (i = 1; i <= go_to.size; i++)
        {
            symbol = GOTO_SYMBOL(go_to, i);
            act = GOTO_ACTION(go_to, i);
            for (q = action_count[symbol]; q != NULL; q = q -> next)
            {
                if (q -> action == act)
                    break;
            }

            if (q == NULL) /* new action not yet seen */
            {
                q = (struct action_element *)
                    talloc(sizeof(struct action_element));
                if (q == NULL)
                    nospace(__FILE__, __LINE__);

                q -> action = act;
                q -> count = 1;
                q -> next = action_count[symbol];
                action_count[symbol] = q;
            }
            else (q -> count)++;
        }
    }

    /*******************************************************************/
    /* We now iterate over the mapping created above and for each      */
    /* non-terminal A, initialize GOTODEF(A) to the action that is     */
    /* most frequently defined on A.                                   */
    /*******************************************************************/
    for ALL_NON_TERMINALS(symbol)
    {
        max_count = 0;
        default_action = 0;

        for (q = action_count[symbol]; q != NULL; q = q -> next)
        {
            if (q -> count > max_count)
            {
                max_count = q -> count;
                default_action = q -> action;
            }
        }

        gotodef[symbol] = default_action;
        if (default_action > 0) /* A state number? */
            goto_count += max_count;
        else
            goto_reduce_count += max_count;
    }

    /***********************************************************************/
    /*   We now iterate over the automaton and eliminate all GOTO actions  */
    /* for which there is a DEFAULT.                                       */
    /***********************************************************************/
    for ALL_STATES(state_no)
    {
        k = 0;
        go_to = statset[state_no].go_to;
        for (i = 1; i <= go_to.size; i++)
        {
            if (gotodef[GOTO_SYMBOL(go_to, i)] != GOTO_ACTION(go_to, i))
            {
                k++;
                GOTO_SYMBOL(go_to, k) = GOTO_SYMBOL(go_to, i);
                GOTO_ACTION(go_to, k) = GOTO_ACTION(go_to, i);
            }
        }
        statset[state_no].go_to.size = k;   /* Readjust size */
    }

    sprintf(msg_line,
            "Number of Goto entries saved by default: %d",
            goto_count);
    PRNT(msg_line);
    sprintf(msg_line,
            "Number of Goto/Reduce entries saved by default: %d",
            goto_reduce_count);
    PRNT(msg_line);

    num_gotos -= goto_count;
    num_goto_reduces -= goto_reduce_count;
    num_entries = num_entries - goto_count - goto_reduce_count;

    action_count += (num_terminals + 1);
    ffree(action_count);

    return;
}


/***********************************************************************/
/*                        PROCESS_TABLES:                              */
/***********************************************************************/
/* Remap symbols, apply transition default actions  and call           */
/* appropriate table compression routine.                              */
/***********************************************************************/
void process_tables(void)
{
    int i,
        j,
        state_no,
        rule_no,
        symbol;

    struct goto_header_type   go_to;
    struct shift_header_type  sh;
    struct reduce_header_type red;

    /*******************************************************************/
    /*        First, we decrease by 1 the constants NUM_SYMBOLS        */
    /* and NUM_TERMINALS, remove the EMPTY symbol(1) and remap the     */
    /* other symbols beginning at 1.  If default reduction is          */
    /* requested, we assume a special DEFAULT_SYMBOL with number zero. */
    /*******************************************************************/
    eoft_image--;
    accept_image--;
    if (error_maps_bit)
    {
        error_image--;
        eolt_image--;
    }
    num_terminals--;
    num_symbols--;

    /***************************************************************/
    /* Remap all the symbols used in GOTO and REDUCE actions.      */
    /* Remap all the symbols used in GD_RANGE.                     */
    /* Remap all the symbols used in the range of SCOPE.           */
    /* Release space trapped by the maps IN_STAT and FIRST.        */
    /***************************************************************/
    for ALL_STATES(state_no)
    {
        go_to = statset[state_no].go_to;
        for (i = 1; i <= go_to.size; i++)
            GOTO_SYMBOL(go_to, i)--;

        red = reduce[state_no];
        for (i = 1; i <= red.size; i++)
            REDUCE_SYMBOL(red, i)--;
    }
    for ALL_LA_STATES(state_no)
    {
        red = lastats[state_no].reduce;
        for (i = 1; i <= red.size; i++)
            REDUCE_SYMBOL(red, i)--;
    }

    for (i = 1; i <= gotodom_size; i++)
        gd_range[i]--;

    for (i = 1; i <= num_scopes; i++)
    {
        scope[i].lhs_symbol--;
        scope[i].look_ahead--;
    }
    for (i = 1; i <= scope_rhs_size; i++)
    {
        if (scope_right_side[i] != 0)
            scope_right_side[i]--;
    }

    /*******************************************************************/
    /* Remap all symbols in the domain of the Shift maps.              */
    /*******************************************************************/
    for (i = 1; i <= num_shift_maps; i++)
    {
        sh = shift[i];
        for (j = 1; j <= sh.size; j++)
            SHIFT_SYMBOL(sh, j)--;
    }

    /*******************************************************************/
    /* Remap the left-hand side of all the rules.                      */
    /*******************************************************************/
    for ALL_RULES(rule_no)
        rules[rule_no].lhs--;

    /*******************************************************************/
    /* Remap the dot symbols in ITEM_TABLE.                            */
    /*******************************************************************/
    if (error_maps_bit)
    {
        for ALL_ITEMS(i)
            item_table[i].symbol--;
    }

    /*******************************************************************/
    /* We update the SYMNO map.                                        */
    /*******************************************************************/
    for ALL_SYMBOLS(symbol)
        symno[symbol] = symno[symbol + 1];

    /*******************************************************************/
    /* If Goto Default and/or Shift Default were requested, process    */
    /* appropriately.                                                  */
    /*******************************************************************/
    if (shift_default_bit)
        compute_shift_default();

    if (goto_default_bit)
        compute_goto_default();

    /******************************************************************/
    /* Release the pool of temporary space.                           */
    /******************************************************************/
    free_temporary_space();

    /*****************************************************************/
    /* We allocate the necessary structures, open the appropriate    */
    /* output file and call the appropriate compression routine.     */
    /*****************************************************************/
    if (error_maps_bit)
    {
        naction_symbols = (SET_PTR)
                          calloc(num_states + 1,
                                 non_term_set_size * sizeof(BOOLEAN_CELL));
        if (naction_symbols == NULL)
            nospace(__FILE__, __LINE__);
        action_symbols = (SET_PTR)
                         calloc(num_states + 1,
                                term_set_size * sizeof(BOOLEAN_CELL));
        if (action_symbols == NULL)
            nospace(__FILE__, __LINE__);
    }

    output_buffer = (char *) calloc(IOBUFFER_SIZE, sizeof(char));
    if (output_buffer == NULL)
        nospace(__FILE__, __LINE__);

    if ((! c_bit) && (! cpp_bit) && (! java_bit))
    {
#if defined(C370) && !defined(CW)
        int size;

        size = PARSER_LINE_SIZE ;
        if (record_format != 'F')
            size += 4;
        sprintf(msg_line, "w, recfm=%cB, lrecl=%d",
                          record_format, size);

        if((systab = fopen(tab_file, msg_line)) == NULL)
#else
        if((systab = fopen(tab_file, "w")) == NULL)
#endif
        {
            fprintf(stderr,
                    "***ERROR: Table file \"%s\" cannot be opened\n",
                    tab_file);
            exit(12);
        }
    }

    if (table_opt == OPTIMIZE_SPACE)
        cmprspa();
    else if (table_opt == OPTIMIZE_TIME)
        cmprtim();

    if ((! c_bit) && (! cpp_bit) && (! java_bit))
        fclose(systab);

/*********************************************************************/
/* If printing of the states was requested,  print the new mapping   */
/* of the states.                                                    */
/*********************************************************************/
    if (states_bit)
    {
        PR_HEADING;
        fprintf(syslis,
                "\nMapping of new state numbers into "
                "original numbers:\n");
        for ALL_STATES(i)
             fprintf(syslis,
                     "\n%5d  ==>>  %5d",ordered_state[i], state_list[i]);
        fprintf(syslis,"\n");
    }

    return;
}
