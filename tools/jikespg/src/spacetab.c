static char hostfile[] = __FILE__;

#include <string.h>
#include "common.h"
#include "space.h"
#include "header.h"

static struct node **new_state_element_reduce_nodes;

static long total_bytes,
            num_table_entries;

static int top,
           empty_root,
           single_root,
           multi_root;

static short *row_size,
             *frequency_symbol,
             *frequency_count;

static BOOLEAN *shift_on_error_symbol;

/****************************************************************************/
/*                            REMAP_NON_TERMINALS:                          */
/****************************************************************************/
/*  REMAP_NON_TERMINALS remaps the non-terminal symbols and states based on */
/* frequency of entries.                                                    */
/****************************************************************************/
static void remap_non_terminals(void)
{
    short *frequency_symbol,
          *frequency_count,
          *row_size;

    struct goto_header_type go_to;

    int i,
        state_no,
        symbol;

/**********************************************************************/
/*   The variable FREQUENCY_SYMBOL is used to hold the non-terminals  */
/* in the grammar, and  FREQUENCY_COUNT is used correspondingly to    */
/* hold the number of actions defined on each non-terminal.           */
/* ORDERED_STATE and ROW_SIZE are used in a similar fashion for states*/
/**********************************************************************/
    frequency_symbol = Allocate_short_array(num_non_terminals);
    frequency_symbol -= (num_terminals + 1);
    frequency_count = Allocate_short_array(num_non_terminals);
    frequency_count -= (num_terminals + 1);
    row_size = Allocate_short_array(num_states + 1);

    for ALL_NON_TERMINALS(i)
    {
        frequency_symbol[i] = i;
        frequency_count[i] = 0;
    }
    for ALL_STATES(state_no)
    {
        ordered_state[state_no] = state_no;
        row_size[state_no] = 0;
        go_to = statset[state_no].go_to;
        for (i = 1; i <= go_to.size; i++)
        {
            row_size[state_no]++;
            symbol = GOTO_SYMBOL(go_to, i);
            frequency_count[symbol]++;
        }
    }

    /**********************************************************************/
    /* The non-terminals are sorted in descending order based on the      */
    /* number of actions defined on then, and they are remapped based on  */
    /* the new arrangement obtained by the sorting.                       */
    /**********************************************************************/
    sortdes(frequency_symbol, frequency_count,
            num_terminals + 1, num_symbols, num_states);

    for ALL_NON_TERMINALS(i)
        symbol_map[frequency_symbol[i]] = i;

    /**********************************************************************/
    /*    All non-terminal entries in the state automaton are updated     */
    /* accordingly.  We further subtract NUM_TERMINALS from each          */
    /* non-terminal to make them fall in the range [1..NUM_NON_TERMINLS]  */
    /* instead of [NUM_TERMINALS+1..NUM_SYMBOLS].                         */
    /**********************************************************************/
    for ALL_STATES(state_no)
    {
        go_to = statset[state_no].go_to;
        for (i = 1; i <= go_to.size; i++)
           GOTO_SYMBOL(go_to, i) =
                symbol_map[GOTO_SYMBOL(go_to, i)] - num_terminals;
    }

    /**********************************************************************/
    /* If Goto-Default was requested, we find out how many non-terminals  */
    /* were eliminated as a result, and adjust the GOTO-DEFAULT map,      */
    /* based on the new mapping of the non-terminals.                     */
    /**********************************************************************/
    if (goto_default_bit)
    {
        short *temp_goto_default;

        temp_goto_default = Allocate_short_array(num_non_terminals);
        temp_goto_default -= (num_terminals + 1);

        for (last_symbol = num_symbols;
             last_symbol > num_terminals; last_symbol--)
             if (frequency_count[last_symbol] != 0)
                 break;
        last_symbol -= num_terminals;
        sprintf(msg_line, "Number of non-terminals eliminated: %d",
                          num_non_terminals - last_symbol);
        PRNT(msg_line);

        /**************************************************************/
        /* Remap the GOTO-DEFAULT map.                                */
        /* to hold the original map.                                  */
        /**************************************************************/
        for ALL_NON_TERMINALS(symbol)
            temp_goto_default[symbol_map[symbol]] = gotodef[symbol];
        gotodef += (num_terminals + 1);
        ffree(gotodef);
        gotodef = temp_goto_default;
    }
    else
        last_symbol = num_non_terminals;

    /**********************************************************************/
    /* The states are sorted in descending order based on the number of   */
    /* actions defined on them, and they are remapped based on the new    */
    /* arrangement obtained by the sorting.                               */
    /**********************************************************************/
    sortdes(ordered_state, row_size, 1, num_states, last_symbol);

    frequency_symbol += (num_terminals + 1);
    ffree(frequency_symbol);
    frequency_count += (num_terminals + 1);
    ffree(frequency_count);
    ffree(row_size);

    return;
}


/****************************************************************************/
/*                           OVERLAP_NT_ROWS:                               */
/****************************************************************************/
/* We now overlap the non-terminal table, or more precisely, we compute the */
/* starting position in a vector where each of its rows may be placed       */
/* without clobbering elements in another row.  The starting positions are  */
/* stored in the vector STATE_INDEX.                                        */
/****************************************************************************/
static void overlap_nt_rows(void)
{
    struct goto_header_type go_to;

    int max_indx,
        indx,
        k,
        j,
        i,
        k_bytes,
        percentage,
        state_no,
        symbol;

    long num_bytes;

    num_table_entries = num_gotos + num_goto_reduces + num_states;
    increment_size = MAX((num_table_entries / 100 * increment),
                         (last_symbol + 1));
    table_size = MIN((num_table_entries + increment_size), MAX_TABLE_SIZE);

    /*************************************************************************/
    /* Allocate space for table, and initlaize the AVAIL_POOL list.  The     */
    /* variable FIRST_INDEX keeps track of the first element in the doubly-  */
    /* linked list, and LAST_ELEMENT keeps track of the last element in the  */
    /* list.                                                                 */
    /*   The variable MAX_INDX is used to keep track of the maximum starting */
    /* position for a row that has been used.                                */
    /*************************************************************************/
    next = Allocate_int_array(table_size + 1);
    previous = Allocate_int_array(table_size + 1);

    first_index = 1;
    previous[first_index] = NIL;
    next[first_index] = first_index + 1;
    for (indx = 2; indx < (int) table_size; indx++)
    {
        next[indx] = indx + 1;
        previous[indx] = indx - 1;
    }
    last_index = table_size;
    previous[last_index] = last_index - 1;
    next[last_index] = NIL;

    max_indx = first_index;

    /*******************************************************************/
    /* We now iterate over all the states in their new sorted order as */
    /* indicated by the variable STATE_NO, and determine an "overlap"  */
    /* position for them.                                              */
    /*******************************************************************/
    for ALL_STATES(k)
    {
        state_no = ordered_state[k];

        /****************************************************************/
        /* INDX is set to the beginning of the list of available slots  */
        /* and we try to determine if it might be a valid starting      */
        /* position.  If not, INDX is moved to the next element, and we */
        /* repeat the process until a valid position is found.          */
        /****************************************************************/
        go_to = statset[state_no].go_to;
        indx = first_index;

look_for_match_in_base_table:
        if (indx == NIL)
            indx = table_size + 1;
        if (indx + last_symbol > table_size)
            reallocate();
        for (i = 1; i <= go_to.size; i++)
        {
            if (next[indx + GOTO_SYMBOL(go_to, i)] == OMEGA)
            {
                indx = next[indx];
                goto look_for_match_in_base_table;
            }
        }

        /*****************************************************************/
        /* At this stage, a position(INDX), was found to overlay the row */
        /* in question.  Remove elements associated with all positions   */
        /* that will be taken by row from the doubly-linked list.        */
        /* NOTE tha since SYMBOLs start at 1, the first index can never  */
        /* be a candidate (==> I = INDX + SYMBOL) in this loop.          */
        /*****************************************************************/
        for (j = 1; j <= go_to.size; j++)
        {
            symbol = GOTO_SYMBOL(go_to, j);
            i = indx + symbol;
            if (i == last_index)
            {
                last_index = previous[last_index];
                next[last_index] = NIL;
            }
            else
            {
                next[previous[i]] = next[i];
                previous[next[i]] = previous[i];
            }
            next[i] = OMEGA;
        }

        if (first_index == last_index)
            first_index = NIL;
        else if (indx == first_index)
        {
            first_index = next[first_index];
            previous[first_index] = NIL;
        }
        else if (indx == last_index)
        {
            last_index = previous[last_index];
            next[last_index] = NIL;
        }
        else
        {
            next[previous[indx]] = next[indx];
            previous[next[indx]] = previous[indx];
        }
        next[indx] = OMEGA;
        if (indx > max_indx)
            max_indx = indx;
        state_index[state_no] =  indx;
    }

    if (goto_default_bit || nt_check_bit)
        check_size = max_indx + num_non_terminals;
    else
        check_size = 0;

    for (action_size = max_indx + last_symbol;
         action_size >= max_indx; action_size--)
        if (next[action_size] == OMEGA)
            break;

    accept_act = max_indx + num_rules + 1;
    error_act = accept_act + 1;

    printf("\n");
    if (goto_default_bit || nt_check_bit)
    {
        sprintf(msg_line,"Length of base Check Table: %d", check_size);
        PRNT(msg_line);
    }

    sprintf(msg_line,"Length of base Action Table: %ld", action_size);
    PRNT(msg_line);

    sprintf(msg_line,"Number of entries in base Action Table: %d",
            num_table_entries);
    PRNT(msg_line);

    percentage = ((action_size - num_table_entries) * 1000)
                   / num_table_entries;
    sprintf(msg_line, "Percentage of increase: %d.%d%%",
                      percentage / 10, percentage % 10);
    PRNT(msg_line);

    if (byte_bit)
    {
        num_bytes = 2 * action_size + check_size;
        if (goto_default_bit || nt_check_bit)
        {
            if (last_symbol > 255)
                num_bytes += check_size;
        }
    }
    else
        num_bytes = 2 * (action_size + check_size);

    if (goto_default_bit)
        num_bytes += ((long) 2 * num_non_terminals);
    total_bytes = num_bytes;

    k_bytes = (num_bytes / 1024) + 1;

    sprintf(msg_line,"Storage required for base Tables: %ld Bytes, %dK",
            num_bytes, k_bytes);
    PRNT(msg_line);
    num_bytes = (long) 4 * num_rules;
    if (byte_bit)
    {
        num_bytes -= num_rules;
        if (num_non_terminals < 256)
            num_bytes -= num_rules;
    }
    sprintf(msg_line, "Storage required for Rules: %ld Bytes", num_bytes);
    PRNT(msg_line);

    return;
}


/*********************************************************************/
/*                        MERGE_SIMILAR_T_ROWS:                      */
/*********************************************************************/
/* We now try to merge states in the terminal table that are similar.*/
/* Two states S1 and S2 are said to be similar if they contain the   */
/* same shift actions and they reduce to the same set of rules.  In  */
/* addition,  there must not exist a terminal symbol "t" such that:  */
/* REDUCE(S1, t) and REDUCE(S2, t) are defined, and                  */
/* REDUCE(S1, t) ^= REDUCE(S2, t)                                    */
/*********************************************************************/
static void merge_similar_t_rows(void)
{
    struct shift_header_type  sh;
    struct reduce_header_type red;

    short *table;

    int i,
        j,
        rule_no,
        state_no;

   unsigned long hash_address;

   struct node *q,
               *r,
               *tail,
               *reduce_root;

    table = Allocate_short_array(num_shift_maps + 1);

    empty_root = NIL;
    single_root = NIL;
    multi_root = NIL;
    top = 0;

    for (i = 1; i <= (int) max_la_state; i++)
        shift_on_error_symbol[i] = FALSE;

    for (i = 0; i <= num_shift_maps; i++)
        table[i] = NIL;

/*********************************************************************/
/* We now hash all the states into TABLE, based on their shift map   */
/* number.                                                           */
/* The rules in the range of the REDUCE MAP are placed in sorted     */
/* order in a linear linked list headed by REDUCE_ROOT.              */
/*********************************************************************/
    for (state_no = 1; state_no <= (int) max_la_state; state_no++)
    {
        reduce_root = NULL;
        if (state_no > (int) num_states)
            red = lastats[state_no].reduce;
        else
            red = reduce[state_no];

        for (i = 1; i <= red.size; i++)
        {
            rule_no = REDUCE_RULE_NO(red, i);

            for (q = reduce_root; q != NULL; tail = q, q = q -> next)
            { /* Is it or not in REDUCE_ROOT list? */
                if (q -> value == rule_no)
                    goto continue_traverse_reduce_map;
                if (q -> value > rule_no)
                    break;
            }

            r = Allocate_node(); /* Add element to REDUCE_ROOT list */
            r -> value = rule_no;
            if (q == reduce_root)
            {
                r -> next = reduce_root;
                reduce_root = r;
            }
            else
            {
                r -> next = q;
                tail -> next = r;
            }
continue_traverse_reduce_map:;
        }

/*********************************************************************/
/*   We compute the HASH_ADDRESS,  mark if the state has a shift     */
/* action on the ERROR symbol, and search the hash TABLE to see if a */
/* state matching the description is already in there.               */
/*********************************************************************/
        if (state_no > (int) num_states)
            hash_address = lastats[state_no].shift_number;
        else
        {
            if (default_opt == 5)
            {
                sh = shift[statset[state_no].shift_number];
                for (j = 1; (j <= sh.size) &&
                            (! shift_on_error_symbol[state_no]); j++)
                    shift_on_error_symbol[state_no] =
                          (SHIFT_SYMBOL(sh, j) == error_image);
            }
            hash_address = statset[state_no].shift_number;
        }

        for (i = table[hash_address]; i != NIL; i = new_state_element[i].link)
        {
            for (r = reduce_root, q = new_state_element_reduce_nodes[i];
                 (r != NULL) && (q != NULL);
                 r = r -> next, q = q -> next)
            {
                if (r -> value != q -> value)
                    break;
            }

            if (r == q)
                break;
        }

/*********************************************************************/
/* If the state is a new state to be inserted in the table, we now   */
/* do so,  and place it in the proper category based on its reduces, */
/* In any case, the IMAGE field is updated, and so is the relevant   */
/* STATE_LIST element.                                               */
/*                                                                   */
/* If the state contains a shift action on the error symbol and also */
/* contains reduce actions,  we allocate a new element for it and    */
/* place it in the list headed by MULTI_ROOT.  Such states are not   */
/* merged, because we do not take default reductions in them.        */
/*********************************************************************/
        if (shift_on_error_symbol[state_no] && (reduce_root != NULL))
        {
            top++;
            if (i == NIL)
            {
                new_state_element[top].link = table[hash_address];
                table[hash_address] = top;
            }

            new_state_element[top].thread = multi_root;
            multi_root = top;

            new_state_element[top].shift_number = hash_address;
            new_state_element_reduce_nodes[top] = reduce_root;
            state_list[state_no] = NIL;
            new_state_element[top].image = state_no;
        }
        else if (i == NIL)
        {
            top++;
            new_state_element[top].link = table[hash_address];
            table[hash_address] = top;
            if (reduce_root == NULL)
            {
                new_state_element[top].thread = empty_root;
                empty_root = top;
            }
            else if (reduce_root -> next == NULL)
            {
                new_state_element[top].thread = single_root;
                single_root = top;
            }
            else
            {
                new_state_element[top].thread = multi_root;
                multi_root = top;
            }
            new_state_element[top].shift_number = hash_address;
            new_state_element_reduce_nodes[top] = reduce_root;
            state_list[state_no] = NIL;
            new_state_element[top].image = state_no;
        }
        else
        {
            state_list[state_no] = new_state_element[i].image;
            new_state_element[i].image = state_no;

            for (r = reduce_root; r != NULL; tail = r, r = r -> next)
                ;
            if (reduce_root != NULL)
                free_nodes(reduce_root, tail);
        }
    }

    ffree(table);

    return;
}


/*********************************************************************/
/*                       MERGE_SHIFT_DOMAINS:                        */
/*********************************************************************/
/*    If shift-default actions are requested, the shift actions      */
/* associated with each state are factored out of the Action matrix  */
/* and all identical rows are merged.  This merged matrix is used to */
/* create a boolean vector that may be used to confirm whether or not*/
/* there is a shift action in a given state S on a given symbol t.   */
/* If we can determine that there is a shift action on a pair (S, t) */
/* we can apply shift default to the Shift actions just like we did  */
/* for the Goto actions.                                             */
/*********************************************************************/
static void merge_shift_domains(void)
{
    short *shift_domain_link,
          *ordered_shift,
          *terminal_list,
          *temp_shift_default;

    short shift_domain_table[SHIFT_TABLE_SIZE];

    struct shift_header_type  sh;
    struct reduce_header_type red;

    BOOLEAN *shift_symbols;

    unsigned long hash_address;

    int i,
        j,
        indx,
        max_indx,
        k_bytes,
        old_table_size,
        k,
        percentage,
        shift_size,
        shift_no,
        symbol,
        state_no;

    long num_bytes;

/*********************************************************************/
/* Some of the rows in the shift action map have already been merged */
/* by the merging of compatible states... We simply need to increase */
/* the size of the granularity by merging these new terminal states  */
/* based only on their shift actions.                                */
/*                                                                   */
/* The array SHIFT_DOMAIN_TABLE is used as the base for a hash table.*/
/* Each submap represented by a row of the shift action map is hashed*/
/* into this table by summing the terminal symbols in its domain.    */
/* The submap is then entered in the hash table and assigned a unique*/
/* number if such a map was not already placed in the hash table.    */
/* Otherwise, the number assigned to the previous submap is also     */
/* associated with the new submap.                                   */
/*                                                                   */
/* The vector SHIFT_IMAGE is used to keep track of the unique number */
/* associated with each unique shift submap.                         */
/* The vector REAL_SHIFT_NUMBER is the inverse of SHIFT_IMAGE. It is */
/* used to associate each unique number to its shift submap.         */
/* The integer NUM_TABLE_ENTRIES is used to count the number of      */
/* elements in the new merged shift map.                             */
/*                                                                   */
/* The arrays ORDERED_SHIFT and ROW_SIZE are also initialized here.  */
/* They are used to sort the rows of the shift actions map later...  */
/*********************************************************************/
    shift_domain_link = Allocate_short_array(num_terminal_states + 1);
    ordered_shift = Allocate_short_array(num_shift_maps + 1);
    terminal_list = Allocate_short_array(num_terminals + 1);
    shift_image = Allocate_short_array(max_la_state + 1);
    real_shift_number = Allocate_short_array(num_shift_maps + 1);
    shift_symbols = Allocate_boolean_array(num_terminals + 1);

    shift_check_index = Allocate_int_array(num_shift_maps + 1);

    for (i = 0; i <= SHIFT_TABLE_UBOUND; i++)
       shift_domain_table[i] = NIL;

    num_table_entries = 0;
    shift_domain_count = 0;

    for (state_no = 1; state_no <= num_terminal_states; state_no++)
    {
        shift_no = new_state_element[state_no].shift_number;
        for (i = 1; i <=num_terminals; i++)
            shift_symbols[i] = FALSE;

        sh = shift[shift_no];
        shift_size = sh.size;
        hash_address = shift_size;
        for (i = 1; i <= shift_size; i++)
        {
            symbol = SHIFT_SYMBOL(sh, i);
            hash_address += symbol;
            shift_symbols[symbol] = TRUE;
        }

        hash_address %= SHIFT_TABLE_SIZE;

        for (i = shift_domain_table[hash_address];
             i != NIL; i = shift_domain_link[i])
        {
            sh = shift[new_state_element[i].shift_number];
            if (sh.size == shift_size)
            {
                for (j = 1; j <= shift_size; j++)
                    if (! shift_symbols[SHIFT_SYMBOL(sh, j)])
                        break;
                if (j > shift_size)
                {
                    shift_image[state_no] = shift_image[i];
                    goto continu;
                }
            }
        }
        shift_domain_link[state_no] = shift_domain_table[hash_address];
        shift_domain_table[hash_address] = state_no;

        shift_domain_count++;
        shift_image[state_no] = shift_domain_count;

        real_shift_number[shift_domain_count] = shift_no;

        ordered_shift[shift_domain_count] = shift_domain_count;
        row_size[shift_domain_count] = shift_size;
        num_table_entries += shift_size;
continu:;
    }

/*********************************************************************/
/*   Compute the frequencies, and remap the terminal symbols         */
/* accordingly.                                                      */
/*********************************************************************/
    for ALL_TERMINALS(symbol)
    {
        frequency_symbol[symbol] = symbol;
        frequency_count[symbol] = 0;
    }

    for (i = 1; i <= shift_domain_count; i++)
    {
        shift_no = real_shift_number[i];
        sh = shift[shift_no];
        for (j = 1; j <= sh.size; j++)
        {
            symbol = SHIFT_SYMBOL(sh, j);
            frequency_count[symbol]++;
        }
    }

    sortdes(frequency_symbol, frequency_count, 1, num_terminals,
            shift_domain_count);

    for ALL_TERMINALS(symbol)
        symbol_map[frequency_symbol[symbol]] =  symbol;

    symbol_map[DEFAULT_SYMBOL] = DEFAULT_SYMBOL;
    eoft_image = symbol_map[eoft_image];
    if (error_maps_bit)
    {
        error_image = symbol_map[error_image];
        eolt_image = symbol_map[eolt_image];
    }

    for (i = 1; i <= num_shift_maps; i++)
    {
        sh = shift[i];
        for (j = 1; j <= sh.size; j++)
             SHIFT_SYMBOL(sh, j) = symbol_map[SHIFT_SYMBOL(sh, j)];
    }

    for (state_no = 1; state_no <= num_terminal_states; state_no++)
    {
        red = new_state_element[state_no].reduce;
        for (i = 1; i <= red.size; i++)
            REDUCE_SYMBOL(red, i) = symbol_map[REDUCE_SYMBOL(red, i)];
    }

/*********************************************************************/
/* If ERROR_MAPS are requested, we also have to remap the original   */
/* REDUCE maps.                                                      */
/*********************************************************************/
    if (error_maps_bit)
    {
        for ALL_STATES(state_no)
        {
            red = reduce[state_no];
            for (i = 1; i <= red.size; i++)
                REDUCE_SYMBOL(red, i) = symbol_map[REDUCE_SYMBOL(red, i)];
        }
    }

    /************************************************************/
    /* Remap the SHIFT_DEFAULT map.                             */
    /************************************************************/
    temp_shift_default = Allocate_short_array(num_terminals + 1);
    for ALL_TERMINALS(symbol)
        temp_shift_default[symbol_map[symbol]] = shiftdf[symbol];
    ffree(shiftdf);
    shiftdf = temp_shift_default;

/*********************************************************************/
/* We now compute the starting position for each Shift check row     */
/* as we did for the terminal states.  The starting positions are    */
/* stored in the vector SHIFT_CHECK_INDEX.                           */
/*********************************************************************/
    sortdes(ordered_shift, row_size, 1, shift_domain_count, num_terminals);

    increment_size = MAX((num_table_entries / 100 * increment),
                         (num_terminals + 1));
    old_table_size = table_size;
    table_size = MIN((num_table_entries + increment_size), MAX_TABLE_SIZE);
    if ((int) table_size > old_table_size)
    {
        ffree(previous);
        ffree(next);
        previous = Allocate_int_array(table_size + 1);
        next = Allocate_int_array(table_size + 1);
    }
    else
        table_size = old_table_size;

    first_index = 1;
    previous[first_index] = NIL;
    next[first_index] = first_index + 1;
    for (indx = 2; indx < (int) table_size; indx++)
    {
        next[indx] = indx + 1;
        previous[indx] = indx - 1;
    }
    last_index = table_size;
    previous[last_index] = last_index - 1;
    next[last_index] = NIL;
    max_indx = first_index;

/*********************************************************************/
/* Look for a suitable index where to overlay the shift check row.   */
/*********************************************************************/
    for (k = 1; k <= shift_domain_count; k++)
    {
        shift_no = ordered_shift[k];
        sh = shift[real_shift_number[shift_no]];
        indx = first_index;

look_for_match_in_sh_chk_tab:
        if (indx == NIL)
            indx = table_size + 1;
        if (indx + num_terminals > (int) table_size)
            reallocate();
        for (i = 1; i <= sh.size; i++)
        {
            symbol = SHIFT_SYMBOL(sh, i);
            if (next[indx + symbol] == OMEGA)
            {
               indx = next[indx];
               goto look_for_match_in_sh_chk_tab;
            }
        }

/*********************************************************************/
/* INDX marks the starting position for the row, remove all the      */
/* positions that are claimed by that shift check row.               */
/* If a position has the value 0,   then it is the starting position */
/* of a Shift row that was previously processed, and that element    */
/* has already been removed from the list of available positions.    */
/*********************************************************************/
        for (j = 1; j <= sh.size; j++)
        {
            symbol = SHIFT_SYMBOL(sh, j);
            i = indx + symbol;
            if (next[i] != 0)
            {
                if (i == last_index)
                {
                    last_index = previous[last_index];
                    next[last_index] = NIL;
                }
                else
                {
                    next[previous[i]] = next[i];
                    previous[next[i]] = previous[i];
                }
            }
            next[i] = OMEGA;
        }

/*********************************************************************/
/* We now remove the starting position itself from the list without  */
/* marking it as taken, since it can still be used for a shift check.*/
/* MAX_INDX is updated if required.                                  */
/* SHIFT_CHECK_INDEX(SHIFT_NO) is properly set to INDX as the        */
/* starting position of STATE_NO.                                    */
/*********************************************************************/
        if (first_index == last_index)
            first_index = NIL;
        else if (indx == first_index)
        {
            first_index = next[first_index];
            previous[first_index] = NIL;
        }
        else if (indx == last_index)
        {
            last_index = previous[last_index];
            next[last_index] = NIL;
        }
        else
        {
            next[previous[indx]] = next[indx];
            previous[next[indx]] = previous[indx];
        }
        next[indx] = 0;

        if (indx > max_indx)
            max_indx = indx;
        shift_check_index[shift_no] = indx;
    }

/*********************************************************************/
/* Update all counts, and report statistics.                         */
/*********************************************************************/
    shift_check_size = max_indx + num_terminals;

    printf("\n");
    sprintf(msg_line,"Length of Shift Check Table: %d",shift_check_size);
    PRNT(msg_line);

    sprintf(msg_line,"Number of entries in Shift Check Table: %d",
            num_table_entries);
    PRNT(msg_line);

    for (k = shift_check_size; k >= max_indx; k--)
        if (next[k] == OMEGA)
            break;
    percentage = (((long) k - num_table_entries) * 1000)
                 / num_table_entries;

    sprintf(msg_line,"Percentage of increase: %d.%d%%",
            (percentage/10),(percentage % 10));
    PRNT(msg_line);

    if (byte_bit)
    {
        num_bytes = shift_check_size;
        if (num_terminals > 255)
            num_bytes +=shift_check_size;
    }
    else
        num_bytes = 2 * shift_check_size;
    num_bytes += 2 * (num_terminal_states + num_terminals);

    k_bytes = (num_bytes / 1024) + 1;

    sprintf(msg_line,
            "Storage required for Shift Check Table: %ld Bytes, %dK",
            num_bytes, k_bytes);
    PRNT(msg_line);
    total_bytes += num_bytes;

    ffree(ordered_shift);
    ffree(terminal_list);
    ffree(shift_symbols);
    ffree(shift_domain_link);

    return;
}


/*********************************************************************/
/*                         OVERLAY_SIM_T_ROWS:                       */
/*********************************************************************/
/* By now, similar states have been grouped together, and placed in  */
/* one of three linear linked lists headed by the root pointers:     */
/* MULTI_ROOT, SINGLE_ROOT, and EMPTY_ROOT.                          */
/* We iterate over each of these lists and construct new states out  */
/* of these groups of similar states when they are compatible. Then, */
/* we remap the terminal symbols.                                    */
/*********************************************************************/
static void overlay_sim_t_rows(void)
{
    int j,
        k,
        i,
        rule_no,
        state_no,
        symbol,
        default_rule,
        state_subset_root,
        state_set_root,
        state_root,
        reduce_size,
        num_shifts_saved = 0,
        num_reductions_saved = 0,
        default_saves = 0;

    short *rule_count,
          *reduce_action;

    struct shift_header_type  sh;
    struct reduce_header_type red;
    struct reduce_header_type new_red;

    struct node *q,
                *tail;

    rule_count = Allocate_short_array(num_rules + 1);
    reduce_action = Allocate_short_array(num_terminals + 1);

/*********************************************************************/
/*     We first iterate over the groups of similar states in the     */
/* MULTI_ROOT list.  These states have been grouped together,        */
/* because they have the same Shift map, and reduce to the same      */
/* rules, but we must check that they are fully compatible by making */
/* sure that no two states contain reduction to a different rule on  */
/* the same symbol.                                                  */
/*     The idea is to take a state out of the group, and merge with  */
/* it as many other compatible states from the group as possible.    */
/* remaining states from the group that caused clashes are thrown    */
/* back into the MULTI_ROOT list as a new group of states.           */
/*********************************************************************/
    for (i = multi_root; i != NIL; i = new_state_element[i].thread)
    {
        for (q = new_state_element_reduce_nodes[i]; q != NULL; q = q -> next)
        {
            rule_count[q -> value] = 0;
        }

/*********************************************************************/
/* REDUCE_ACTION is used to keep track of reductions that are to be  */
/* applied on terminal symbols as the states are merged.  We pick    */
/* out the first state (STATE_NO) from the group of states involved, */
/* initialize REDUCE_ACTION with its reduce map, and count the number*/
/* of reductions associated with each rule in that state.            */
/*********************************************************************/
        state_no = new_state_element[i].image;
        if (state_no > (int) num_states)
            red = lastats[state_no].reduce;
        else
            red = reduce[state_no];

        for ALL_TERMINALS(j)
            reduce_action[j] = OMEGA;
        for (j = 1; j <= red.size; j++)
        {
            rule_no = REDUCE_RULE_NO(red, j);
            reduce_action[REDUCE_SYMBOL(red, j)] = rule_no;
            rule_count[rule_no]++;
        }

/*********************************************************************/
/* STATE_SET_ROOT is used to traverse the rest of the list that form */
/* the group of states being processed.  STATE_SUBSET_ROOT is used   */
/* to construct the new list that will consist of all states in the  */
/* group that are compatible starting with the initial state.        */
/* STATE_ROOT is used to construct a list of all states in the group */
/* that are not compatible with the initial state.                   */
/*********************************************************************/
        state_set_root = state_list[state_no];
        state_subset_root = state_no;
        state_list[state_subset_root] = NIL;
        state_root = NIL;

        for (state_no = state_set_root;
             state_no != NIL; state_no = state_set_root)
        {
            state_set_root = state_list[state_set_root];

/*********************************************************************/
/* We traverse the reduce map of the state taken out from the group  */
/* and check to see if it is compatible with the subset being        */
/* constructed so far.                                               */
/*********************************************************************/
            if (state_no > (int) num_states)
                red = lastats[state_no].reduce;
            else
                red = reduce[state_no];
            for (j = 1; j <= red.size; j++)
            {
                symbol = REDUCE_SYMBOL(red, j);
                if (reduce_action[symbol] != OMEGA)
                {
                    if (reduce_action[symbol] != REDUCE_RULE_NO(red, j))
                        break;
                }
            }

/*********************************************************************/
/* If J > Q->REDUCE_ELEMENT.N then we traversed the whole reduce map,*/
/* and all the reductions involved are compatible with the new state */
/* being constructed.  The state involved is added to the subset, the*/
/* rule counts are updated, and the REDUCE_ACTIONS map is updated.   */
/*     Otherwise, we add the state involved to the STATE_ROOT list   */
/* which will be thrown back in the MULTI_ROOT list.                 */
/*********************************************************************/
            if (j > red.size)
            {
                state_list[state_no] = state_subset_root;
                state_subset_root = state_no;
                for (j = 1; j <= red.size; j++)
                {
                    symbol = REDUCE_SYMBOL(red, j);
                    if (reduce_action[symbol] == OMEGA)
                    {
                        rule_no = REDUCE_RULE_NO(red, j);
                        if (rules[rule_no].lhs == accept_image)
                            rule_no = 0;
                        reduce_action[symbol] = rule_no;
                        rule_count[rule_no]++;
                    }
                    else
                        num_reductions_saved++;
                }
            }
            else
            {
                state_list[state_no] = state_root;
                state_root = state_no;
            }
        }

/*********************************************************************/
/* Figure out the best default rule candidate, and update            */
/* DEFAULT_SAVES.                                                    */
/* Recall that all accept actions were changed into reduce actions   */
/* by rule 0.                                                        */
/*********************************************************************/
        k = 0;
        reduce_size = 0;
        default_rule = error_act;
        for (q = new_state_element_reduce_nodes[i];
             q != NULL; tail = q, q = q -> next)
        {
            rule_no = q -> value;
            reduce_size += rule_count[rule_no];
            if ((rule_count[rule_no] > k) && (rule_no != 0)
                && ! shift_on_error_symbol[state_subset_root])
            {
                k = rule_count[rule_no];
                default_rule = rule_no;
            }
        }
        default_saves += k;
        reduce_size -= k;

/*********************************************************************/
/* If STATE_ROOT is not NIL then there are states in the group that  */
/* did not meet the compatibility test.  Throw those states back in  */
/* front of MULTI_ROOT as a group.                                   */
/*********************************************************************/
        if (state_root != NIL)
        {
            top++;
            new_state_element[top].thread = new_state_element[i].thread;
            new_state_element[i].thread = top;
            if (state_root > (int) num_states)
                new_state_element[top].shift_number =
                          lastats[state_root].shift_number;
            else
                new_state_element[top].shift_number =
                                     statset[state_root].shift_number;
            new_state_element_reduce_nodes[top] =
                                     new_state_element_reduce_nodes[i];
            new_state_element[top].image = state_root;
        }
        else
            free_nodes(new_state_element_reduce_nodes[i], tail);

/*********************************************************************/
/* Create Reduce map for the newly created terminal state.           */
/* We may assume that SYMBOL field of defaults is already set to     */
/* the DEFAULT_SYMBOL value.                                         */
/*********************************************************************/
        new_red = Allocate_reduce_map(reduce_size);

        REDUCE_SYMBOL(new_red, 0) = DEFAULT_SYMBOL;
        REDUCE_RULE_NO(new_red, 0) = default_rule;
        for ALL_TERMINALS(symbol)
        {
            if (reduce_action[symbol] != OMEGA)
            {
                if (reduce_action[symbol] != default_rule)
                {
                    REDUCE_SYMBOL(new_red, reduce_size) = symbol;
                    if (reduce_action[symbol] == 0)
                        REDUCE_RULE_NO(new_red, reduce_size) = accept_act;
                    else
                        REDUCE_RULE_NO(new_red, reduce_size) =
                                       reduce_action[symbol];
                    reduce_size--;
                }
            }
        }
        new_state_element[i].reduce = new_red;
        new_state_element[i].image = state_subset_root;
    }

/*********************************************************************/
/* We now process groups of states that have reductions to a single  */
/* rule.  Those states are fully compatible, and the default is the  */
/* rule in question.                                                 */
/* Any of the REDUCE_ELEMENT maps that belongs to a state in the     */
/* group of states being processed may be reused for the new  merged */
/* state.                                                            */
/*********************************************************************/
    for (i = single_root; i != NIL; i = new_state_element[i].thread)
    {
        state_no = new_state_element[i].image;
        q = new_state_element_reduce_nodes[i];
        rule_no = q -> value;
        free_nodes(q, q);
        if (rules[rule_no].lhs == accept_image)
        {
            red = reduce[state_no];
            reduce_size = red.size;

            new_red = Allocate_reduce_map(reduce_size);

            REDUCE_SYMBOL(new_red, 0) = DEFAULT_SYMBOL;
            REDUCE_RULE_NO(new_red, 0) = error_act;

            for (j = 1; j <= reduce_size; j++)
            {
                REDUCE_SYMBOL(new_red, j) = REDUCE_SYMBOL(red, j);
                REDUCE_RULE_NO(new_red, j) = accept_act;
            }
        }
        else
        {
            for ALL_TERMINALS(j)
                reduce_action[j] = OMEGA;

            for (; state_no != NIL; state_no = state_list[state_no])
            {
                if (state_no > (int) num_states)
                    red = lastats[state_no].reduce;
                else
                    red = reduce[state_no];
                for (j = 1; j <= red.size; j++)
                {
                    symbol = REDUCE_SYMBOL(red, j);
                    if (reduce_action[symbol] == OMEGA)
                    {
                        reduce_action[symbol] = rule_no;
                        default_saves++;
                    }
                    else
                       num_reductions_saved++;
                }
            }

            new_red = Allocate_reduce_map(0);

            REDUCE_SYMBOL(new_red, 0) = DEFAULT_SYMBOL;
            REDUCE_RULE_NO(new_red, 0) = rule_no;
        }
        new_state_element[i].reduce = new_red;
    }

/*********************************************************************/
/* Groups of states that have no reductions are also compatible.     */
/* Their default is ERROR_ACTION.                                    */
/*********************************************************************/
    for (i = empty_root; i != NIL; i = new_state_element[i].thread)
    {
        state_no = new_state_element[i].image;
        if (state_no > (int) num_states)
            red = lastats[state_no].reduce;
        else
            red = reduce[state_no];
        REDUCE_SYMBOL(red, 0) = DEFAULT_SYMBOL;
        REDUCE_RULE_NO(red, 0) = error_act;
        new_state_element[i].reduce = red;
    }

    num_terminal_states = top;

    frequency_symbol = Allocate_short_array(num_terminals + 1);
    frequency_count = Allocate_short_array(num_terminals + 1);
    row_size = Allocate_short_array(max_la_state + 1);

    if (shift_default_bit)
        merge_shift_domains();

/*********************************************************************/
/* We now reorder the terminal states based on the number of actions */
/* in them, and remap the terminal symbols if they were not already  */
/* remapped in the previous block for the SHIFT_CHECK vector.        */
/*********************************************************************/
    for ALL_TERMINALS(symbol)
    {
        frequency_symbol[symbol] = symbol;
        frequency_count[symbol] = 0;
    }

    for (i = 1; i <= num_terminal_states; i++)
    {
        ordered_state[i] = i;
        row_size[i] = 0;
        sh = shift[new_state_element[i].shift_number];
        for (j = 1; j <= sh.size; j++)
        {
            symbol = SHIFT_SYMBOL(sh, j);
            if ((! shift_default_bit) ||
                (SHIFT_ACTION(sh, j) != shiftdf[symbol]))
            {
                row_size[i]++;
                frequency_count[symbol]++;
            }
        }

        for (state_no = state_list[new_state_element[i].image];
             state_no != NIL; state_no = state_list[state_no])
        {
            num_shifts_saved += row_size[i];
        }

        /***********************************************/
        /* Note that the Default action is skipped !!! */
        /***********************************************/
        red = new_state_element[i].reduce;
        for (j = 1; j <= red.size; j++)
        {
            symbol = REDUCE_SYMBOL(red, j);
            row_size[i]++;
            frequency_count[symbol]++;
        }
    }
    sprintf(msg_line,
            "Number of unique terminal states: %d",
            num_terminal_states);
    PRNT(msg_line);
    sprintf(msg_line, "Number of Shift actions saved by merging: %d",
                      num_shifts_saved);
    PRNT(msg_line);
    sprintf(msg_line, "Number of Reduce actions saved by merging: %d",
                      num_reductions_saved);
    PRNT(msg_line);
    sprintf(msg_line, "Number of Reduce saved by default: %d",
                      default_saves);
    PRNT(msg_line);

    sortdes(ordered_state, row_size, 1, num_terminal_states,
            num_terminals);

    if (! shift_default_bit)
    {
        sortdes(frequency_symbol, frequency_count, 1, num_terminals,
                num_terminal_states);
        for ALL_TERMINALS(symbol)
            symbol_map[frequency_symbol[symbol]] =  symbol;
        symbol_map[DEFAULT_SYMBOL] = DEFAULT_SYMBOL;
        eoft_image = symbol_map[eoft_image];
        if (error_maps_bit)
        {
            error_image = symbol_map[error_image];
            eolt_image = symbol_map[eolt_image];
        }
        for (i = 1; i <= num_shift_maps; i++)
        {
            sh = shift[i];
            for (j = 1; j <= sh.size; j++)
                SHIFT_SYMBOL(sh, j) = symbol_map[SHIFT_SYMBOL(sh, j)];
        }

        for (state_no = 1; state_no <= num_terminal_states; state_no++)
        {
            red = new_state_element[state_no].reduce;
            for (i = 1; i <= red.size; i++)
                REDUCE_SYMBOL(red, i) = symbol_map[REDUCE_SYMBOL(red, i)];
        }

/*********************************************************************/
/* If ERROR_MAPS are requested, we also have to remap the original   */
/* REDUCE maps.                                                      */
/*********************************************************************/
        if (error_maps_bit)
        {
            for ALL_STATES(state_no)
            {
                red = reduce[state_no];
                for (i = 1; i <= red.size; i++)
                    REDUCE_SYMBOL(red, i) = symbol_map[REDUCE_SYMBOL(red, i)];
            }
        }
    }
    num_table_entries = num_shifts + num_shift_reduces + num_reductions
                        - num_shifts_saved - num_reductions_saved
                        - default_saves + num_terminal_states;

    ffree(rule_count);
    ffree(reduce_action);
    ffree(row_size);
    ffree(frequency_count);
    ffree(frequency_symbol);
    ffree(shift_on_error_symbol);
    ffree(new_state_element_reduce_nodes);

    return;
}


/*********************************************************************/
/*                           OVERLAP_T_ROWS:                         */
/*********************************************************************/
/* We now compute the starting position for each terminal state just */
/* as we did for the non-terminal states.                            */
/* The starting positions are stored in the vector TERM_STATE_INDEX. */
/*********************************************************************/
static void overlap_t_rows(void)
{
    struct shift_header_type  sh;
    struct reduce_header_type red;

    short *terminal_list;

    int root_symbol,
        symbol,
        i,
        k,
        k_bytes,
        percentage,
        state_no,
        old_size,
        max_indx,
        indx;

    long num_bytes;

    terminal_list = Allocate_short_array(num_terminals + 1);
    term_state_index = Allocate_int_array(max_la_state + 1);

    increment_size = MAX((num_table_entries * increment / 100),
                         (num_terminals + 1));
    old_size = table_size;
    table_size = MIN((num_table_entries + increment_size), MAX_TABLE_SIZE);
    if ((int) table_size > old_size)
    {
        ffree(previous);
        ffree(next);
        next = Allocate_int_array(table_size + 1);
        previous = Allocate_int_array(table_size + 1);
    }
    else
        table_size = old_size;

    first_index = 1;
    previous[first_index] = NIL;
    next[first_index] = first_index + 1;
    for (indx = 2; indx < (int) table_size; indx++)
    {
        next[indx] = indx + 1;
        previous[indx] = indx - 1;
    }
    last_index = table_size;
    previous[last_index] = last_index - 1;
    next[last_index] = NIL;

    max_indx = first_index;

    for (k = 1; k <= num_terminal_states; k++)
    {
        state_no = ordered_state[k];
/*********************************************************************/
/* For the terminal table, we are dealing with two lists, the SHIFT  */
/* list, and the REDUCE list. Those lists are merged together first  */
/* in TERMINAL_LIST.  Since we have to iterate over the list twice,  */
/* this merging makes things easy.                                   */
/*********************************************************************/
        root_symbol = NIL;
        sh = shift[new_state_element[state_no].shift_number];
        for (i = 1; i <= sh.size; i++)
        {
            symbol = SHIFT_SYMBOL(sh, i);
            if (! shift_default_bit ||
                (SHIFT_ACTION(sh, i) != shiftdf[symbol]))
            {
                terminal_list[symbol] = root_symbol;
                root_symbol = symbol;
            }
        }

        red = new_state_element[state_no].reduce;
        for (i = 1; i <= red.size; i++)
        {
            terminal_list[REDUCE_SYMBOL(red, i)] = root_symbol;
            root_symbol = REDUCE_SYMBOL(red, i);
        }

/*********************************************************************/
/* Look for a suitable index where to overlay the state.             */
/*********************************************************************/
        indx = first_index;

look_for_match_in_term_table:
        if (indx == NIL)
            indx = table_size + 1;
        if (indx + num_terminals > (int) table_size)
            reallocate();
        for (symbol = root_symbol;
             symbol != NIL; symbol = terminal_list[symbol])
        {
            if (next[indx + symbol] == OMEGA)
            {
                indx = next[indx];
                goto look_for_match_in_term_table;
            }
        }

/*********************************************************************/
/* INDX marks the starting position for the state, remove all the    */
/* positions that are claimed by terminal actions in the state.      */
/*********************************************************************/
        for (symbol = root_symbol;
             symbol != NIL; symbol = terminal_list[symbol])
        {
            i = indx + symbol;
            if (i == last_index)
            {
                last_index = previous[last_index];
                next[last_index] = NIL;
            }
            else
            {
                next[previous[i]] = next[i];
                previous[next[i]] = previous[i];
            }
            next[i] = OMEGA;
        }

/*********************************************************************/
/* We now remove the starting position itself from the list, and     */
/* mark it as taken(CHECK(INDX) = OMEGA)                             */
/* MAX_INDX is updated if required.                                  */
/* TERM_STATE_INDEX(STATE_NO) is properly set to INDX as the starting*/
/* position of STATE_NO.                                             */
/*********************************************************************/
        if (first_index == last_index)
            first_index = NIL;
        else if (indx == first_index)
        {
            first_index = next[first_index];
            previous[first_index] = NIL;
        }
        else if (indx == last_index)
        {
            last_index = previous[last_index];
            next[last_index] = NIL;
        }
        else
        {
            next[previous[indx]] = next[indx];
            previous[next[indx]] = previous[indx];
        }

        next[indx] = OMEGA;

        if (indx > max_indx)
            max_indx = indx;
        term_state_index[state_no] = indx;
    }

/*********************************************************************/
/* Update all counts, and report statistics.                         */
/*********************************************************************/
    term_check_size = max_indx + num_terminals;
    for (term_action_size = max_indx + num_terminals;
         term_action_size >= max_indx; term_action_size--)
        if (next[term_action_size] == OMEGA)
            break;

    printf("\n");
    sprintf(msg_line, "Length of Terminal Check Table: %d",
                      term_check_size);
    PRNT(msg_line);

    sprintf(msg_line, "Length of Terminal Action Table: %d",
                      term_action_size);
    PRNT(msg_line);

    sprintf(msg_line, "Number of entries in Terminal Action Table: %d",
                      num_table_entries);
    PRNT(msg_line);

    percentage = (((long)term_action_size - num_table_entries) * 1000)
                  / num_table_entries;

    sprintf(msg_line, "Percentage of increase: %d.%d%%",
                      (percentage / 10), (percentage % 10));
    PRNT(msg_line);

    if (byte_bit)
    {
        num_bytes = 2 * term_action_size + term_check_size;
        if (num_terminals > 255)
            num_bytes += term_check_size;
    }
    else
        num_bytes = 2 * (term_action_size + term_check_size);

    if (shift_default_bit)
        num_bytes += (2 * num_terminal_states);

    k_bytes = (num_bytes / 1024) + 1;

    sprintf(msg_line,
            "Storage required for Terminal Tables: %ld Bytes, %dK",
            num_bytes, k_bytes);
    PRNT(msg_line);

    total_bytes += num_bytes;

/*********************************************************************/
/* Report total number of storage used.                              */
/*********************************************************************/
    k_bytes = (total_bytes / 1024) + 1;
    sprintf(msg_line,
            "Total storage required for Tables: %ld Bytes, %dK",
            total_bytes, k_bytes);
    PRNT(msg_line);

/*********************************************************************/
/* We now write out the tables to the SYSTAB file.                   */
/*********************************************************************/

    table_size = MAX(check_size, term_check_size);
    table_size = MAX(table_size, shift_check_size);
    table_size = MAX(table_size, action_size);
    table_size = MAX(table_size, term_action_size);

    ffree(terminal_list);
    ffree(next);
    ffree(previous);
}


/*********************************************************************/
/*                         PRINT_TABLES:                             */
/*********************************************************************/
/* We now write out the tables to the SYSTAB file.                   */
/*********************************************************************/
static void print_tables(void)
{
    int *check,
        *action;

    int la_state_offset,
        i,
        j,
        k,
        indx,
        act,
        result_act,
        default_count = 0,
        goto_count = 0,
        goto_reduce_count = 0,
        reduce_count = 0,
        la_shift_count = 0,
        shift_count = 0,
        shift_reduce_count = 0,
        rule_no,
        symbol,
        state_no;

    char *tok;

    long offset;

    check = Allocate_int_array(table_size + 1);
    action = Allocate_int_array(table_size + 1);

    /******************************************************************/
    /* Prepare header card with proper information, and write it out. */
    /******************************************************************/
    offset = error_act;
    if (read_reduce_bit)
        offset += num_rules;
    la_state_offset = offset;

    if (offset > (MAX_TABLE_SIZE + 1))
    {
        sprintf(msg_line, "Table contains entries that are > "
                "%d; Processing stopped.", MAX_TABLE_SIZE + 1);
        PRNTERR(msg_line);
        exit(12);
    }

    output_buffer[0] = 'S';
    output_buffer[1] = (goto_default_bit ? '1' : '0');
    output_buffer[2] = (nt_check_bit ? '1' : '0');
    output_buffer[3] = (read_reduce_bit ? '1' : '0');
    output_buffer[4] = (single_productions_bit ? '1' : '0');
    output_buffer[5] = (shift_default_bit ? '1' : '0');
    output_buffer[6] = (rules[1].lhs == accept_image ? '1' : '0');
                       /* are there more than 1 start symbols? */
    output_buffer[7] = (error_maps_bit ? '1' : '0');
    output_buffer[8] = (byte_bit && last_symbol <= 255 ? '1' : '0');
    output_buffer[9] = escape;

    output_ptr = output_buffer + 10;
    field(num_terminals, 5);
    field(num_non_terminals, 5);
    field(num_rules, 5);
    field(num_states, 5);
    field(check_size, 5);
    field(action_size, 5);
    field(term_check_size, 5);
    field(term_action_size, 5);
    field(state_index[1] + num_rules, 5);
    field(eoft_image, 5);
    field(accept_act, 5);
    field(error_act, 5);
    field(la_state_offset, 5);
    field(lalr_level, 5);
    *output_ptr++ = '\n';

    /*********************************************************/
    /* We write the terminal symbols map.                    */
    /*********************************************************/
    for ALL_TERMINALS(symbol)
    {
        int len;

        tok = RETRIEVE_STRING(symbol);
        if (tok[0] == '\n')  /* we're dealing with special symbol?  */
            tok[0] = escape; /* replace initial marker with escape. */
        len = strlen(tok);
        field(symbol_map[symbol], 4);
        field(len, 4);
        if (len <= 64)
            strcpy(output_ptr, tok);
        else
        {
            memcpy(output_ptr, tok, 64);
            output_ptr += 64;
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
            tok += 64;

            for (len = strlen(tok); len > 72; len = strlen(tok))
            {
                memcpy(output_ptr, tok, 72);
                output_ptr += 72;
                *output_ptr++ = '\n';
                BUFFER_CHECK(systab);
                tok += 72;
            }
            memcpy(output_ptr, tok, len);
        }
        output_ptr += len;
        *output_ptr++ = '\n';
        BUFFER_CHECK(systab);
    }

    /*********************************************************/
    /* We write the non-terminal symbols map.                */
    /*********************************************************/
    for ALL_NON_TERMINALS(symbol)
    {
        int len;

        tok = RETRIEVE_STRING(symbol);
        if (tok[0] == '\n')  /* we're dealing with special symbol?  */
            tok[0] = escape; /* replace initial marker with escape. */
        len = strlen(tok);
        field(symbol_map[symbol]-num_terminals, 4);
        field(len, 4);
        if (len <= 64)
            strcpy(output_ptr, tok);
        else
        {
            memcpy(output_ptr, tok, 64);
            output_ptr += 64;
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
            tok += 64;

            for (len = strlen(tok); len > 72; len = strlen(tok))
            {
                memcpy(output_ptr, tok, 72);
                output_ptr += 72;
                *output_ptr++ = '\n';
                BUFFER_CHECK(systab);
                tok += 72;
            }
            memcpy(output_ptr, tok, len);
        }
        output_ptr += len;
        *output_ptr++ = '\n';
        BUFFER_CHECK(systab);
    }

    /*********************************************************/
    /* Initialize TABLES with default actions.               */
    /*********************************************************/
    for (i = 1; i <= check_size; i++)
        check[i] = DEFAULT_SYMBOL;

    for (i = 1; i <= (int) action_size; i++)
        action[i] = error_act;

    /********************************************************************/
    /*    Update the default non-terminal action of each state with the */
    /* appropriate corresponding terminal state starting index.         */
    /********************************************************************/
    for (i = 1; i <= num_terminal_states; i++)
    {
        indx = term_state_index[i];
        state_no = new_state_element[i].image;

    /*********************************************************************/
    /*   Update the action link between the non-terminal and terminal    */
    /* tables.  If error-maps are requested, an indirect linking is made */
    /* as follows:                                                       */
    /*  Each non-terminal row identifies its original state number, and  */
    /* a new vector START_TERMINAL_STATE indexable by state numbers      */
    /* identifies the starting point of each state in the terminal table.*/
    /*********************************************************************/
        if (state_no <= (int) num_states)
        {
            for (; state_no != NIL; state_no = state_list[state_no])
            {
                action[state_index[state_no]] = indx;
            }
        }
        else
        {
            for (; state_no != NIL; state_no = state_list[state_no])
            {
                act = la_state_offset + indx;
                state_index[state_no] = act;
            }
        }
    }

    /*********************************************************************/
    /*  Now update the non-terminal tables with the non-terminal actions.*/
    /*********************************************************************/
    for ALL_STATES(state_no)
    {
        struct goto_header_type go_to;

        indx = state_index[state_no];
        go_to = statset[state_no].go_to;
        for (j = 1; j <= go_to.size; j++)
        {
            symbol = GOTO_SYMBOL(go_to, j);
            i = indx + symbol;
            if (goto_default_bit || nt_check_bit)
                check[i] = symbol;
            act = GOTO_ACTION(go_to, j);
            if (act > 0)
            {
                action[i] = state_index[act] + num_rules;
                goto_count++;
            }
            else
            {
                action[i] = -act;
                goto_reduce_count++;
            }
        }
    }

    /*********************************************************************/
    /* Write size of right hand side of rules followed by CHECK table.   */
    /*********************************************************************/
    k = 0;
    for (i = 1; i <= num_rules; i++)
    {
        field(RHS_SIZE(i), 4);
        k++;
        if (k == 18)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
            k = 0;
        }
    }

    for (i = 1; i <= check_size; i++)
    {
        field(check[i], 4);
        k++;
        if (k == 18)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
            k = 0;
        }
    }

    if (k != 0)
    {
        *output_ptr++ = '\n';
        BUFFER_CHECK(systab);
    }

    /*********************************************************************/
    /* Write left hand side symbol of rules followed by ACTION table.    */
    /*********************************************************************/
    k = 0;
    for (i = 1; i <= num_rules; i++)
    {
        field(symbol_map[rules[i].lhs] - num_terminals, 6);
        k++;
        if (k == 12)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
            k = 0;
        }
    }

    for (i = 1; i <= (int) action_size; i++)
    {
        field(action[i], 6);
        k++;
        if (k == 12)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
            k = 0;
        }
    }

    if (k != 0)
    {
        *output_ptr++ = '\n';
        BUFFER_CHECK(systab);
    }

    /********************************************************************/
    /* Initialize the terminal tables,and update with terminal actions. */
    /********************************************************************/
    for (i = 1; i <= term_check_size; i++)
        check[i] = DEFAULT_SYMBOL;

    for (i = 1; i <= term_action_size; i++)
        action[i] = error_act;

    for (state_no = 1; state_no <= num_terminal_states; state_no++)
    {
        struct shift_header_type  sh;
        struct reduce_header_type red;

        indx = term_state_index[state_no];
        sh = shift[new_state_element[state_no].shift_number];
        for (j = 1; j <= sh.size; j++)
        {
            symbol = SHIFT_SYMBOL(sh, j);
            act = SHIFT_ACTION(sh, j);
            if ((! shift_default_bit) || (act != shiftdf[symbol]))
            {
                i = indx + symbol;
                check[i] = symbol;

                if (act > (int) num_states)
                {
                    result_act = state_index[act];
                    la_shift_count++;
                }
                else if (act > 0)
                {
                    result_act = state_index[act] + num_rules;
                    shift_count++;
                }
                else
                {
                    result_act = -act + error_act;
                    shift_reduce_count++;
                }

                if (result_act > (MAX_TABLE_SIZE + 1))
                {
                    sprintf(msg_line,
                        "Table contains look-ahead shift entry that is >"
                        " %d; Processing stopped.", MAX_TABLE_SIZE + 1);
                    PRNTERR(msg_line);
                    return;
                }

                action[i] = result_act;
            }
        }

        red = new_state_element[state_no].reduce;
        for (j = 1; j <= red.size; j++)
        {
            symbol = REDUCE_SYMBOL(red, j);
            rule_no = REDUCE_RULE_NO(red, j);
            i = indx + symbol;
            check[i] = symbol;
            action[i] = rule_no;
            reduce_count++;
        }

        rule_no = REDUCE_RULE_NO(red, 0);
        if (rule_no != error_act)
            default_count++;

        check[indx] = DEFAULT_SYMBOL;
        if (shift_default_bit)
            action[indx] = state_no;
        else
            action[indx] = rule_no;
    }

    PRNT("\n\nActions in Compressed Tables:");
    sprintf(msg_line,"     Number of Shifts: %d",shift_count);
    PRNT(msg_line);

    sprintf(msg_line,"     Number of Shift/Reduces: %d",shift_reduce_count);
    PRNT(msg_line);

    if (max_la_state > num_states)
    {
        sprintf(msg_line,
                "     Number of Look-Ahead Shifts: %d",
                la_shift_count);
        PRNT(msg_line);
    }

    sprintf(msg_line,"     Number of Gotos: %d",goto_count);
    PRNT(msg_line);

    sprintf(msg_line,"     Number of Goto/Reduces: %d",goto_reduce_count);
    PRNT(msg_line);

    sprintf(msg_line,"     Number of Reduces: %d",reduce_count);
    PRNT(msg_line);

    sprintf(msg_line,"     Number of Defaults: %d",default_count);
    PRNT(msg_line);

    /********************************************************************/
    /* Write Terminal Check Table.                                      */
    /********************************************************************/
    k = 0;
    for (i = 1; i <= term_check_size; i++)
    {
        field(check[i], 4);
        k++;
        if (k == 18)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
            k = 0;
        }
    }

    if (k != 0)
    {
        *output_ptr++ = '\n';
        BUFFER_CHECK(systab);
    }

    /********************************************************************/
    /* Write Terminal Action Table.                                      */
    /********************************************************************/
    k = 0;
    for (i = 1; i <= term_action_size; i++)
    {
        field(action[i], 6);
        k++;
        if (k == 12)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
            k = 0;
        }
    }

    if (k != 0)
    {
        *output_ptr++ = '\n';
        BUFFER_CHECK(systab);
    }

    /********************************************************************/
    /* If GOTO_DEFAULT is requested, we print out the GOTODEF vector.   */
    /********************************************************************/
    if (goto_default_bit)
    {
        k = 0;
        for ALL_NON_TERMINALS(symbol)
        {
            act = gotodef[symbol];
            if (act < 0)
                result_act = -act;
            else if (act == 0)
                result_act = error_act;
            else
                result_act = state_index[act] + num_rules;

            field(result_act, 6);
            k++;
            if (k == 12)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(systab);
                k = 0;
            }
        }

        if (k != 0)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
        }
    }

/*********************************************************************/
/* If SHIFT_DEFAULT is requested, we print out the Default Reduce    */
/* map, the Shift State map, the Shift Check vector, and the SHIFTDF */
/* vector.                                                           */
/*********************************************************************/
    if (shift_default_bit)
    {
        /* Print out header */
        field(num_terminal_states, 5);
        field(shift_check_size, 5);
        *output_ptr++ = '\n';

        k = 0;
        for (state_no = 1; state_no <= num_terminal_states; state_no++)
        {
            struct reduce_header_type red;

            red = new_state_element[state_no].reduce;
            field(REDUCE_RULE_NO(red, 0), 4);
            k++;
            if (k == 18)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(systab);
                k = 0;
            }
        }
        if (k != 0)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
        }

        /*************************************************************/
        /* First, check whether or not maximum value in SHIFT_STATE  */
        /* table exceeds 9999. If so, stop. Otherwise, write out     */
        /* SHIFT_STATE table.                                        */
        /*************************************************************/
        if ((shift_check_size - num_terminals) > 9999)
        {
            PRNTERR("SHIFT_STATE map contains > 9999 elements");
            return;
        }

        k = 0;
        for (state_no = 1; state_no <= num_terminal_states; state_no++)
        {
            field(shift_check_index[shift_image[state_no]], 4);
            k++;
            if (k == 18)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(systab);
                k = 0;
            }
        }
        if (k != 0)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
        }

/*********************************************************************/
/* Set the Check vector with the symbols in the domain of the shift  */
/* maps.                                                             */
/*********************************************************************/
        for (i = 1; i <= shift_check_size; i++)
            check[i] = DEFAULT_SYMBOL;

        for (i = 1; i <= shift_domain_count; i++)
        {
            struct shift_header_type sh;

            indx = shift_check_index[i];
            sh = shift[real_shift_number[i]];
            for (j = 1; j <= sh.size; j++)
            {
                symbol = SHIFT_SYMBOL(sh, j);
                check[indx + symbol] = symbol;
            }
        }

        k = 0;
        for (i = 1; i <= shift_check_size; i++)
        {
            field(check[i], 4);
            k++;
            if (k == 18)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(systab);
                k = 0;
            }
        }
        if (k != 0)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
        }

        k = 0;
        for ALL_TERMINALS(symbol)
        {
            act = shiftdf[symbol];
            if (act < 0)
                result_act = -act + error_act;
            else if (act == 0)
                result_act = error_act;
            else if (act > (int) num_states)
                result_act = state_index[act];
            else
                result_act = state_index[act] + num_rules;

            if (result_act > (MAX_TABLE_SIZE + 1))
            {
                sprintf(msg_line,
                    "Table contains look-ahead shift entry that is >"
                    " %d; Processing stopped.", MAX_TABLE_SIZE + 1);
                PRNTERR(msg_line);
                return;
            }

            field(result_act, 6);
            k++;
            if (k == 12)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(systab);
                k = 0;
            }
        }
        if (k != 0)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
        }
    }

/*********************************************************************/
/* We first sort the new state numbers.  A bucket sort technique     */
/* is used using the ACTION vector as a base to simulate the         */
/* buckets.  NOTE: the iteration over the buckets is done backward   */
/* because we also construct a list of the original state numbers    */
/* that reflects the permutation of the new state numbers.           */
/* During the backward iteration,  we construct the list as a stack. */
/*********************************************************************/
    if (error_maps_bit || states_bit)
    {
        int max_indx;

        max_indx = accept_act - num_rules - 1;
        for (i = 1; i <= max_indx; i++)
            action[i] = OMEGA;
        for ALL_STATES(state_no)
            action[state_index[state_no]] = state_no;

        j = num_states + 1;
        for (i = max_indx; i >= 1; i--)
        {
            state_no = action[i];
            if (state_no != OMEGA)
            {
                j--;
                ordered_state[j] = i + num_rules;
                state_list[j] = state_no;
            }
        }
    }

    ffree(check);
    ffree(action);

    if (error_maps_bit)
        process_error_maps();

    fwrite(output_buffer, sizeof(char),
           output_ptr - &output_buffer[0], systab);

    return;
}


/*********************************************************************/
/*                             CMPRSPA:                              */
/*********************************************************************/
void cmprspa(void)
{
    state_index = Allocate_int_array(max_la_state + 1);

    ordered_state = Allocate_short_array(max_la_state + 1);
    symbol_map = Allocate_short_array(num_symbols + 1);
    state_list = Allocate_short_array(max_la_state + 1);
    shift_on_error_symbol = Allocate_boolean_array(max_la_state + 1);
    new_state_element = (struct new_state_type *)
                        calloc(max_la_state + 1,
                               sizeof(struct new_state_type));
    if (new_state_element == NULL)
        nospace(__FILE__, __LINE__);

    new_state_element_reduce_nodes = (struct node **)
                                     calloc(max_la_state + 1,
                                            sizeof(struct node *));
    if (new_state_element_reduce_nodes == NULL)
        nospace(__FILE__, __LINE__);

    remap_non_terminals();
    overlap_nt_rows();
    merge_similar_t_rows();
    overlay_sim_t_rows();
    overlap_t_rows();
    if (c_bit || cpp_bit || java_bit)
        print_space_parser();
    else
        print_tables();

    return;
}
