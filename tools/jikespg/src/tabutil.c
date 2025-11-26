static char hostfile[] = __FILE__;

#include <string.h>
#include "common.h"
#include "header.h"

static const char digits[] = "0123456789";

/**************************************************************************/
/*                           PRNT_SHORTS:                                 */
/**************************************************************************/
void prnt_shorts(char *title, int init, int bound, int perline, short *array)
{
    int i,
        k;

    mystrcpy(title);

    padline();
    k = 0;
    for (i = init; i <= bound; i++)
    {
        itoc(array[i]);
        *output_ptr++ = COMMA;
        k++;
        if (k == perline && i != bound)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(sysdcl);
            padline();
            k = 0;
        }
    }
    if (k != 0)
    {
        *(output_ptr - 1) = '\n';
        BUFFER_CHECK(sysdcl);
    }

    if (java_bit)
         mystrcpy("    };\n");
    else mystrcpy("                 };\n");

    return;
}


/**************************************************************************/
/*                              PRNT_INTS:                                */
/**************************************************************************/
void prnt_ints(char *title, int init, int bound, int perline, int *array)
{
    int i,
        k;

    mystrcpy(title);

    padline();
    k = 0;
    for (i = init; i <= bound; i++)
    {
        itoc(array[i]);
        *output_ptr++ = COMMA;
        k++;
        if (k == perline && i != bound)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(sysdcl);
            padline();
            k = 0;
        }
    }
    if (k != 0)
    {
        *(output_ptr - 1) = '\n';
        BUFFER_CHECK(sysdcl);
    }

    if (java_bit)
         mystrcpy("    };\n");
    else mystrcpy("                 };\n");

    return;
}


/***************************************************************************/
/*                               MYSTRCPY:                                 */
/***************************************************************************/
void mystrcpy(char *str)
{
    while (*str != '\0')
         *output_ptr++ = *str++;

    BUFFER_CHECK(sysdcl);

    return;
}


/***************************************************************************/
/*                               PADLINE:                                  */
/***************************************************************************/
void padline(void)
{
    register int i;

    for (i = 0; i < 12; i++)
        *output_ptr++ = ' ';

    return;
}


/***************************************************************************/
/*                                 ITOC:                                   */
/***************************************************************************/
/* ITOC takes as arguments an integer NUM. NUM is an integer containing at */
/* most 11 digits which is converted into a character string and placed in  */
/* the iobuffer.  Leading zeros are eliminated and if the number is        */
/* negative, a leading "-" is added.                                       */
/***************************************************************************/
void itoc(int num)
{
    register int  val;
    register char *p;
    char tmp[12];

    val = ABS(num);
    tmp[11] = '\0';
    p = &tmp[11];
    do
    {
        p--;
        *p = digits[val % 10];
        val /= 10;
    } while(val > 0);

    if (num < 0)
    {
        p--;
        *p = '-';
    }

    while(*p != '\0')
        *(output_ptr++) = *(p++);

    return;
}


/***************************************************************************/
/*                                 FIELD:                                  */
/***************************************************************************/
/* FIELD takes as arguments two integers: NUM and LEN.  NUM is an integer  */
/* containing at most LEN digits which is converted into a character       */
/* string and placed in the iobuffer.                                      */
/* Leading zeros are replaced by blanks and if the number is negative,  a  */
/* leading "-" is added.                                                   */
/***************************************************************************/
void field(int num, int len)
{
    register int val;
    register char *p;

    val = ABS(num);
    p = output_ptr + len;
    do
    {
        p--;
        *p = digits[val % 10];
        val /= 10;
    } while(val > 0 && p > output_ptr);

    if (num < 0 && p > output_ptr)
    {
        p--;
        *p = '-';
    }

    while(p > output_ptr)
    {
        p--;
        *p = ' ';
    }

    output_ptr += len;

    return;
}


/***************************************************************************/
/*                            SORTDES:                                     */
/***************************************************************************/
/*  SORTDES sorts the elements of ARRAY and COUNT in the range LOW..HIGH   */
/* based on the values of the elements of COUNT. Knowing that the maximum  */
/* value of the elements of count cannot exceed MAX and cannot be lower    */
/* than zero, we can use a bucket sort technique.                          */
/***************************************************************************/
void sortdes(short array[], short count[], int low, int high, int max)
{
    short *bucket,
          *list;

    register int element,
             i,
             k;

    /*****************************************************************/
    /* BUCKET is used to hold the roots of lists that contain the    */
    /* elements of each bucket.  LIST is used to hold these lists.   */
    /*****************************************************************/
    bucket = Allocate_short_array(max + 1);
    list = Allocate_short_array(high - low + 1);

    for (i = 0; i <= max; i++)
        bucket[i] = NIL;

/*********************************************************************/
/* We now partition the elements to be sorted and place them in their*/
/* respective buckets.  We iterate backward over ARRAY and COUNT to  */
/* keep the sorting stable since elements are inserted in the buckets*/
/* in stack-fashion.                                                 */
/*                                                                   */
/*   NOTE that it is known that the values of the elements of ARRAY  */
/* also lie in the range LOW..HIGH.                                  */
/*********************************************************************/
    for (i = high; i >= low; i--)
    {
        k = count[i];
        element = array[i];
        list[element - low] = bucket[k];
        bucket[k] = element;
    }

/*********************************************************************/
/* Iterate over each bucket, and place elements in ARRAY and COUNT   */
/* in sorted order.  The iteration is done backward because we want  */
/* the arrays sorted in descending order.                            */
/*********************************************************************/
    k = low;
    for (i = max; i >= 0; i--)
    {
        for (element = bucket[i];
             element != NIL; element = list[element - low], k++)
        {
            array[k] = element;
            count[k] = i;
        }
    }
    ffree(bucket);
    ffree(list);

    return;
}


/***************************************************************************/
/*                              REALLOCATE:                                */
/***************************************************************************/
/*   This procedure is invoked when the TABLE being used is not large      */
/* enough.  A new table is allocated, the information from the old table   */
/* is copied, and the old space is released.                               */
/***************************************************************************/
void reallocate(void)
{
    int *n,
        *p;

    register int old_size,
                 i;

    if (table_size == MAX_TABLE_SIZE)
    {
        sprintf(msg_line, "Table has exceeded maximum limit of %d",
                          MAX_TABLE_SIZE);
        PRNTERR(msg_line);
        exit(12);
    }

    old_size = table_size;
    table_size = MIN(table_size + increment_size, MAX_TABLE_SIZE);

    if (verbose_bit)
    {
        if (table_opt == OPTIMIZE_TIME)
        {
            sprintf(msg_line,
                    "Reallocating storage for TIME table, "
                    "adding %d entries", table_size - old_size);
        }
        else
        {
            sprintf(msg_line,
                    "Reallocating storage for SPACE table, "
                    "adding %d entries", table_size - old_size);
        }
        PRNT(msg_line);
    }

    n = Allocate_int_array(table_size + 1);
    p = Allocate_int_array(table_size + 1);

    for (i = 1; i <= old_size; i++) /* Copy old information */
    {
        n[i] = next[i];
        p[i] = previous[i];
    }

    ffree(next);
    ffree(previous);

    next = n;
    previous = p;

    if (first_index == NIL)
    {
        first_index = old_size + 1;
        previous[first_index] = NIL;
    }
    else
    {
        next[last_index] = old_size + 1;
        previous[old_size + 1] = last_index;
    }

    next[old_size + 1] = old_size + 2;
    for (i = old_size + 2; i < (int) table_size; i++)
    {
        next[i] = i + 1;
        previous[i] = i - 1;
    }
    last_index = table_size;
    next[last_index] = NIL;
    previous[last_index] = last_index - 1;

    return;
}


/*****************************************************************************/
/*                            PROCESS_ERROR_MAPS:                            */
/*****************************************************************************/
/* if ERROR_MAPS are requested, we print them out in the following order:    */
/*                                                                           */
/*   1) The FOLLOW map (NEWFOLL)                                             */
/*   2) The SORTED_STATE vector                                              */
/*   3) The ORIGINAL_STATE vector                                            */
/*   4) The map from states into valid symbols on which actions are          */
/*      defined within the state in question: ACTION_SYMBOLS                 */
/*   5) The map from each symbol into the set of staes that can              */
/*      possibly be reached after a transition on the symbol in              */
/*      question: TRANSITION_STATES                                          */
/*                                                                           */
/*****************************************************************************/
void process_error_maps(void)
{
    short *state_start,
          *state_stack,
          *temp,
          *original = NULL,
          *symbol_root,
          *symbol_count,
          *term_list,
          *as_size,
          *action_symbols_range,
          *naction_symbols_range;

    int offset,
        item_no,
        lhs_symbol,
        state_no,
        symbol,
        max_len,
        i,
        k,

        terminal_ubound,
        non_terminal_ubound;

    long num_bytes;

    char tok[SYMBOL_SIZE + 1];

    terminal_ubound = (table_opt == OPTIMIZE_TIME
                                 ?  num_symbols
                                 :  num_terminals);

    non_terminal_ubound = (table_opt == OPTIMIZE_TIME
                                     ?  num_symbols
                                     :  num_non_terminals);

    symbol_root = Allocate_short_array(num_symbols + 1);
    symbol_count = Allocate_short_array(num_symbols + 1);
    state_start = Allocate_short_array(num_states + 2);
    state_stack = Allocate_short_array(num_states + 1);
    term_list = Allocate_short_array(num_symbols + 1);

    PRNT("\nError maps storage:");

/*********************************************************************/
/* The FOLLOW map is written out as two vectors where the first      */
/* vector indexed by a Symbol gives the starting location in the     */
/* second vector where the elements of the follow set of that symbol */
/* starts.  Note that since the terminal and non-terminal symbols    */
/* have been intermixed, the FOLLOW map is written out with the      */
/* complete set of symbols as its domain even though it is only      */
/* defined on non-terminals.                                         */
/*                                                                   */
/* We now compute and write the starting location for each symbol.   */
/* The offset for the first symbol is 1,  and hence does not         */
/* have to be computed.  However,  we compute an extra offset to     */
/* indicate the extent of the last symbol.                           */
/*********************************************************************/
    for (symbol = 1; symbol <= non_terminal_ubound; symbol++)
    {
        symbol_count[symbol] = 0;
        symbol_root[symbol] = OMEGA;
    }

    for ALL_NON_TERMINALS(lhs_symbol)
    {
        if (table_opt == OPTIMIZE_TIME)
            symbol = symbol_map[lhs_symbol];
        else
            symbol = symbol_map[lhs_symbol] - num_terminals;
        symbol_root[symbol] = lhs_symbol;
        for ALL_TERMINALS(i)
        {
            if IS_IN_SET(follow, (lhs_symbol + 1), (i + 1))
               symbol_count[symbol]++;
        }
    }

    offset = 1;
    k = 1;
    field(offset, 6);     /* Offset of the first state */
    for (symbol = 1; symbol <= non_terminal_ubound; symbol++)
    {
        offset += symbol_count[symbol];
        field(offset, 6);
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

    /***************************************************************/
    /*  We now write the elements in the range of the FOLLOW map.  */
    /***************************************************************/
    k = 0;
    for (symbol = 1; symbol <= non_terminal_ubound; symbol++)
    {
        lhs_symbol = symbol_root[symbol];
        if (lhs_symbol != OMEGA)
        {
            for ALL_TERMINALS(i)
            {
                if IS_IN_SET(follow, (lhs_symbol + 1), (i + 1))
                {
                    field(symbol_map[i], 4);
                    k++;
                    if (k == 18)
                    {
                        *output_ptr++ = '\n';
                        BUFFER_CHECK(systab);
                        k = 0;
                    }
                }
            }
        }
    }
    if (k != 0)
    {
        *output_ptr++ = '\n';
        BUFFER_CHECK(systab);
    }

    /*****************************************************************/
    /* Compute and list amount of space required for the Follow map. */
    /*****************************************************************/
    if (table_opt == OPTIMIZE_TIME)
    {
        num_bytes = 2 * (num_symbols + offset);
        if (byte_bit)
        {
            if (last_non_terminal <= 255)
                num_bytes = num_bytes - offset + 1;
        }
    }
    else
    {
        num_bytes = 2 * (num_non_terminals + offset);
        if (byte_bit)
        {
            if (num_terminals <= 255)
                num_bytes = num_bytes - offset + 1;
        }
    }
    sprintf(msg_line,
            "    Storage required for FOLLOW map: %d Bytes",
            num_bytes);
    PRNT(msg_line);

    /**************************************************************/
    /* We now write out the states in sorted order: SORTED_STATE. */
    /**************************************************************/
    k = 0;
    for ALL_STATES(i)
    {
        field(ordered_state[i], 6);
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

    /*****************************************************************/
    /* Compute and list space required for SORTED_STATE map.         */
    /*****************************************************************/
    num_bytes = 2 * num_states;
    sprintf(msg_line,
            "    Storage required for SORTED_STATE map: %d Bytes",
            num_bytes);
    PRNT(msg_line);

    /********************************************************************/
    /* We now write a vector parallel to SORTED_STATE that gives us the */
    /* original number associated with the state: ORIGINAL_STATE.       */
    /********************************************************************/
    k = 0;
    for (i = 1; i <= (int) num_states; i++)
    {
        field(state_list[i], 6);
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

    /*****************************************************************/
    /* Compute and list space required for ORIGINAL_STATE map.       */
    /*****************************************************************/
    num_bytes = 2 * num_states;
    sprintf(msg_line,
            "    Storage required for ORIGINAL_STATE map: %d Bytes",
            num_bytes);
    PRNT(msg_line);

    /********************************************************************/
    /* We now construct a bit map for the set of terminal symbols that  */
    /* may appear in each state. Then, we invoke PARTSET to apply the   */
    /* Partition Heuristic and print it.                                */
    /********************************************************************/
    as_size = Allocate_short_array(num_states + 1);

    if (table_opt == OPTIMIZE_TIME)
    {
        original = Allocate_short_array(num_symbols + 1);

        /*************************************************************/
        /* In a compressed TIME table, the terminal and non-terminal */
        /* symbols are mixed together when they are remapped.        */
        /* We shall now recover the original number associated with  */
        /* each terminal symbol since it lies very nicely in the     */
        /* range 1..NUM_TERMINALS.  This will save a considerable    */
        /* amount of space in the bit_string representation of sets  */
        /* as well as time when operations are performed on those    */
        /* bit-strings.                                              */
        /*************************************************************/
        for ALL_TERMINALS(symbol)
            original[symbol_map[symbol]] = symbol;
    }

/***********************************************************************/
/* NOTE that the arrays ACTION_SYMBOLS and NACTION_SYMBOLS are global  */
/* variables that are allocated in the procedure PROCESS_TABLES by     */
/* calloc which automatically initializes them to 0.                   */
/***********************************************************************/
    for ALL_STATES(state_no)
    {
        struct shift_header_type  sh;
        struct reduce_header_type red;

        sh = shift[statset[state_no].shift_number];
        as_size[state_no] = sh.size;
        for (i = 1; i <= sh.size; i++)
        {
            if (table_opt == OPTIMIZE_TIME)
                symbol = original[SHIFT_SYMBOL(sh, i)];
            else
                symbol = SHIFT_SYMBOL(sh, i);
            SET_BIT_IN(action_symbols, state_no, symbol);
        }

        red = reduce[state_no];
        as_size[state_no] += red.size;
        for (i = 1; i <= red.size; i++)
        {
            if (table_opt == OPTIMIZE_TIME)
                symbol = original[REDUCE_SYMBOL(red, i)];
            else
                symbol = REDUCE_SYMBOL(red, i);
            SET_BIT_IN(action_symbols, state_no, symbol);
        }
    }

    partset(action_symbols, as_size, state_list, state_start,
            state_stack, num_terminals);

    ffree(action_symbols);

    /*********************************************************************/
    /* We now write the starting location for each state in the domain   */
    /* of the ACTION_SYMBOL map.                                         */
    /* The starting locations are contained in the STATE_START vector.   */
    /*********************************************************************/
    offset = state_start[num_states + 1];
    k = 0;
    for ALL_STATES(i)
    {
        field(ABS(state_start[state_list[i]]), 6);
        k++;
        if (k == 12)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
            k = 0;
        }
    }
    field(offset, 6);
    k++;
    *output_ptr++ = '\n';
    BUFFER_CHECK(systab);

    /**************************************************************/
    /* Compute and write out the range of the ACTION_SYMBOLS map. */
    /**************************************************************/
    action_symbols_range = Allocate_short_array(offset);

    compute_action_symbols_range(state_start, state_stack,
                                 state_list, action_symbols_range);

    k = 0;
    for (i = 0; i < (offset - 1); i++)
    {
        field(action_symbols_range[i], 4);
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
    /* Compute and list space required for ACTION_SYMBOLS map.   */
    /*************************************************************/
    num_bytes = 2 * (num_states + offset);
    if (byte_bit)
    {
        if (offset <= 255)
            num_bytes -= (num_states + 1);
        if (((table_opt == OPTIMIZE_TIME) && (last_terminal <= 255)) ||
            ((table_opt != OPTIMIZE_TIME) && (num_terminals <= 255)))
            num_bytes -= (offset - 1);
    }
    sprintf(msg_line,
            "    Storage required for ACTION_SYMBOLS map: "
            "%ld Bytes", num_bytes);
    PRNT(msg_line);

    ffree(action_symbols_range);

/***********************************************************************/
/* We now repeat the same process for the domain of the GOTO table.    */
/***********************************************************************/
    for ALL_STATES(state_no)
    {
        as_size[state_no] = gd_index[state_no + 1] - gd_index[state_no];
        for (i = gd_index[state_no]; i < gd_index[state_no + 1]; i++)
        {
            symbol = gd_range[i] - num_terminals;
            NTSET_BIT_IN(naction_symbols, state_no, symbol);
        }
    }

    partset(naction_symbols, as_size, state_list, state_start,
            state_stack, num_non_terminals);

    ffree(as_size);
    ffree(naction_symbols);

    for (i = 1; i <= gotodom_size; i++)  /* Remap non-terminals */
    {
        if (table_opt == OPTIMIZE_TIME)
            gd_range[i] = symbol_map[gd_range[i]];
        else
            gd_range[i] = symbol_map[gd_range[i]] - num_terminals;
    }

    /*****************************************************************/
    /* We now write the starting location for each state in the      */
    /* domain of the NACTION_SYMBOLS map. The starting locations are */
    /* contained in the STATE_START vector.                          */
    /*****************************************************************/
    offset = state_start[num_states + 1];

    k = 0;
    for ALL_STATES(i)
    {
        field(ABS(state_start[state_list[i]]), 6);
        k++;
        if (k == 12)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
            k = 0;
        }
    }
    field(offset, 6);
    k++;
    *output_ptr++ = '\n';
    BUFFER_CHECK(systab);

    /**************************************************************/
    /* Compute and write out the range of the NACTION_SYMBOLS map.*/
    /**************************************************************/
    naction_symbols_range = Allocate_short_array(offset);

    compute_naction_symbols_range(state_start, state_stack,
                                  state_list, naction_symbols_range);

    k = 0;
    for (i = 0; i < (offset - 1); i++)
    {
        field(naction_symbols_range[i], 4);
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
    /* Compute and list space required for NACTION_SYMBOLS map.  */
    /*************************************************************/
    num_bytes = 2 * (num_states + offset);
    if (byte_bit)
    {
        if (offset <= 255)
            num_bytes -= (num_states + 1);
        if (((table_opt == OPTIMIZE_TIME) && (last_non_terminal <= 255)) ||
            ((table_opt != OPTIMIZE_TIME) && (num_non_terminals <= 255)))
            num_bytes -= (offset - 1);
    }
    sprintf(msg_line,
            "    Storage required for NACTION_SYMBOLS map: "
            "%ld Bytes", num_bytes);
    PRNT(msg_line);

    ffree(naction_symbols_range);

    /***********************************************************************/
    /* Compute map from each symbol to state into which that symbol can    */
    /* cause a transition: TRANSITION_STATES                               */
    /* TRANSITION_STATES is also written as two vectors like the FOLLOW    */
    /* map and the ACTION_DOMAIN map.                                      */
    /* The first vector contains the starting location in the second       */
    /* vector for each symbol.                                             */
    /*   Construct the TRANSITION_STATES map using an array SYMBOL_ROOT    */
    /* (indexable by each symbol) whose elements are the root of a linked  */
    /* stack built in STATE_STACK. Another array SYMBOL_COUNT is used to   */
    /* keep count of the number of states associated with each symbol.     */
    /* For space tables, the TRANSITION_STATES map is written as two       */
    /* separate tables: SHIFT_STATES and GOTO_STATES.                      */
    /***********************************************************************/
    for ALL_SYMBOLS(symbol)
    {
        symbol_root[symbol] = NIL;
        symbol_count[symbol] = 0;
    }

    for (state_no = 2; state_no <= (int) num_states; state_no++)
    {
        struct node *q;

        q = statset[state_no].kernel_items;
        if (q == NULL) /* is the state a single production state? */
        {
            q = statset[state_no].complete_items; /* pick arbitrary item */
        }

        item_no = q -> value - 1;
        i = item_table[item_no].symbol;
        symbol = symbol_map[i];
        state_stack[state_no] = symbol_root[symbol];
        symbol_root[symbol] = state_no;
        symbol_count[symbol]++;
    }

  /***************************************************************************/
  /* We now compute and write the starting location for each terminal symbol */
  /***************************************************************************/
    offset = 1;     /* Offset of the first state */
    field(offset, 6);
    k = 1;
    for (symbol = 1; symbol <= terminal_ubound; symbol++)
    {
        offset += symbol_count[symbol];
        field(offset, 6);
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

    /*********************************************************************/
    /* We now write out the range elements of SHIFT_STATES for space     */
    /* tables or TRANSITION_STATES for time tables.                      */
    /*********************************************************************/
    k = 0;
    for (symbol = 1; symbol <= terminal_ubound; symbol++)
    {
        for (state_no = symbol_root[symbol];
             state_no != NIL; state_no = state_stack[state_no])
        {
            field(state_index[state_no] + num_rules, 6);
            k++;
            if (k == 12)
            {
                *output_ptr++ = '\n';
                BUFFER_CHECK(systab);
                k = 0;
            }
        }
    }
    if (k != 0)
    {
        *output_ptr++ = '\n';
        BUFFER_CHECK(systab);
    }

    /*********************************************************************/
    /* If space tables are requested we compute and list space required  */
    /* for SHIFT_STATES map. The base vector contains NUM_TERMINALS+ 1   */
    /* elements,  and the vector containing the range elements has size  */
    /* OFFSET - 1. If time tables are requested we compute and list space*/
    /* requirements for TRANSITION_STATES map.  The base vector has      */
    /* NUM_SYMBOLS + 1 elements, and the range elements vector contains  */
    /* OFFSET - 1 elements.                                              */
    /*********************************************************************/
    if (table_opt == OPTIMIZE_TIME)
    {
        num_bytes = 2 * (num_symbols + offset);
        sprintf(msg_line,
                "    Storage required for TRANSITION_STATES map: %d Bytes",
                num_bytes);
        PRNT(msg_line);
    }
    else
    {
        num_bytes = 2 * (num_terminals + offset);
        sprintf(msg_line,
                "    Storage required for SHIFT_STATES map: %d Bytes",
                num_bytes);
        PRNT(msg_line);

        /************************************************************/
        /* We now compute and write the starting location for each  */
        /* non-terminal symbol...                                   */
        /************************************************************/
        offset = 1;
        field(offset, 6);     /* Offset of the first state */
        k = 1;
        for ALL_NON_TERMINALS(symbol)
        {
            offset += symbol_count[symbol];
            field(offset, 6);
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

     /*********************************************************************/
     /* We now write out the range elements of GOTO_STATES whose domain   */
     /* is a non-terminal symbol.                                         */
     /*********************************************************************/
        k = 0;
        for ALL_NON_TERMINALS(symbol)
        {
            for (state_no = symbol_root[symbol];
                 state_no != NIL; state_no = state_stack[state_no])
            {
                field(state_index[state_no] + num_rules, 6);
                k++;
                if (k == 12)
                {
                    *output_ptr++ = '\n';
                    BUFFER_CHECK(systab);
                    k = 0;
                }
            }
        }
        if (k != 0)
        {
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);
        }

       /*********************************************************************/
       /* We compute and list space required for GOTO_STATES map. The       */
       /* base vector contains NUM_NON_TERMINALS+ 1 elements, and the vector*/
       /* containing the range elements has size OFFSET - 1                 */
       /*********************************************************************/
       num_bytes = 2 * (num_non_terminals + offset);
       sprintf(msg_line,"    Storage required for GOTO_STATES map: %d Bytes",
               num_bytes);
       PRNT(msg_line);
    }

    /*********************************************************************/
    /* Write the number associated with the ERROR symbol.                */
    /*********************************************************************/

    field(error_image, 4);
    field(eolt_image, 4);
    field(num_names, 4);
    field(num_scopes, 4);
    field(scope_rhs_size, 4);
    field(scope_state_size, 4);
    if (table_opt == OPTIMIZE_SPACE)
        field(num_error_rules, 4);
    *output_ptr++ = '\n';
    BUFFER_CHECK(systab);

    /*********************************************************************/
    /* We write out the names map.                                       */
    /*********************************************************************/
    num_bytes = 0;
    max_len = 0;
    for (i = 1; i <= num_names; i++)
    {
        int name_len;

        strcpy(tok, RETRIEVE_NAME(i));
        if (tok[0] == '\n')  /* we're dealing with special symbol?  */
            tok[0] = escape; /* replace initial marker with escape. */
        name_len = strlen(tok);
        num_bytes += name_len;
        if (max_len < name_len)
            max_len = name_len;

        field(name_len, 4);

        if (name_len <= 68)
            strcpy(output_ptr, tok);
        else
        {
            memcpy(output_ptr, tok, 68);
            output_ptr+= 68;
            *output_ptr++ = '\n';
            BUFFER_CHECK(systab);;
            strcpy(tok, tok+68);

            for (name_len = strlen(tok); name_len > 72; name_len = strlen(tok))
            {
                memcpy(output_ptr, tok, 72);
                output_ptr+= 72;
                *output_ptr++ = '\n';
                BUFFER_CHECK(systab);
                strcpy(tok, tok+72);
            }
            memcpy(output_ptr, tok, name_len);
        }
        output_ptr += name_len;
        *output_ptr++ = '\n';
        BUFFER_CHECK(systab);
    }

    /*********************************************************************/
    /* We write the name_index of each terminal symbol.  The array TEMP  */
    /* is used to remap the NAME_INDEX values based on the new symbol    */
    /* numberings. If time tables are requested, the terminals and non-  */
    /* terminals are mixed together.                                     */
    /*********************************************************************/
    temp = Allocate_short_array(num_symbols + 1);

    if (table_opt == OPTIMIZE_TIME)
    {
        for ALL_SYMBOLS(symbol)
            temp[symbol_map[symbol]] = symno[symbol].name_index;

        k = 0;
        for ALL_SYMBOLS(symbol)
        {
            field(temp[symbol], 4);
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
    }
    else
    {
        for ALL_TERMINALS(symbol)
            temp[symbol_map[symbol]] = symno[symbol].name_index;

        k = 0;
        for ALL_TERMINALS(symbol)
        {
            field(temp[symbol], 4);
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

       /****************************************************************/
       /* We write the name_index of each non_terminal symbol. The     */
       /* array TEMP is used to remap the NAME_INDEX values based on   */
       /* the new symbol numberings.                                   */
       /****************************************************************/
        for ALL_NON_TERMINALS(symbol)
            temp[symbol_map[symbol]] = symno[symbol].name_index;

        k = 0;
        for ALL_NON_TERMINALS(symbol)
        {
            field(temp[symbol], 4);
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
    }

    /*****************************************************/
    /* Compute and list space requirements for NAME map. */
    /*****************************************************/
    if (max_len > 255)
        offset = 2 * num_symbols;
    else
        offset = num_symbols;

    if (num_bytes > 255)
        offset += (2 * num_symbols);
    else
        offset += num_symbols;

    sprintf(msg_line,
            "    Storage required for direct NAME map: %d Bytes",
            num_bytes + offset);
    PRNT(msg_line);

    if (max_len > 255)
        offset = 2 * num_names;
    else
        offset = num_names;

    if (num_bytes > 255)
        offset += (2 * num_names);
    else
        offset += num_names;

    if (num_names > 255)
        offset += (2 * num_symbols);
    else
        offset += num_symbols;

    sprintf(msg_line,
            "    Storage required for indirect NAME map: %d Bytes",
            num_bytes + offset);
    PRNT(msg_line);

    if (scopes_bit)
    {
        for (i = 1; i <= scope_rhs_size; i++)
        {
            if (scope_right_side[i] != 0)
                scope_right_side[i] = symbol_map[scope_right_side[i]];
        }

        for (i = 1; i <= num_scopes; i++)
        {
            scope[i].look_ahead = symbol_map[scope[i].look_ahead];
            if (table_opt == OPTIMIZE_TIME)
                scope[i].lhs_symbol = symbol_map[scope[i].lhs_symbol];
            else
                scope[i].lhs_symbol = symbol_map[scope[i].lhs_symbol]
                                      - num_terminals;
        }

        k = 0;
        for (i = 1; i <= num_scopes; i++)
        {
            field(scope[i].prefix, 4);
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
        for (i = 1; i <= num_scopes; i++)
        {
            field(scope[i].suffix, 4);
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
        for (i = 1; i <= num_scopes; i++)
        {
            field(scope[i].lhs_symbol, 4);
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
        for (i = 1; i <= num_scopes; i++)
        {
            field(scope[i].look_ahead, 4);
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
        for (i = 1; i <= num_scopes; i++)
        {
            field(scope[i].state_set, 4);
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
        for (i = 1; i <= scope_rhs_size; i++)
        {
            field(scope_right_side[i], 4);
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
        for (i = 1; i <= scope_state_size; i++)
        {
            if (scope_state[i] == 0)
                field(0, 6);
            else
                field(state_index[scope_state[i]] + num_rules, 6);
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

        if (table_opt == OPTIMIZE_TIME)
        {
            num_bytes = 5 * num_scopes +
                        scope_rhs_size+ scope_state_size;
            if (num_symbols > 255)
                num_bytes += (2 * num_scopes + scope_rhs_size);
        }
        else
        {
            num_bytes = 5 * num_scopes +
                        scope_rhs_size+ 2 * scope_state_size;
            if (num_non_terminals > 255)
                num_bytes += num_scopes;
            if (num_terminals > 255)
                num_bytes += num_scopes;
            if (num_symbols > 255)
                num_bytes += scope_rhs_size;
        }
        if (scope_rhs_size > 255)
            num_bytes += (2 * num_scopes);
        if (scope_state_size > 255)
            num_bytes += num_scopes;
        sprintf(msg_line,
                "    Storage required for SCOPE map: %d Bytes",
                num_bytes);
        PRNT(msg_line);
    }

    if (original != NULL)
    {
        ffree(original);
    }
    ffree(symbol_root);
    ffree(symbol_count);
    ffree(temp);
    ffree(state_start);
    ffree(state_stack);
    ffree(term_list);

    return;
}


/*********************************************************************/
/*                   COMPUTE_ACTION_SYMBOLS_RANGE:                   */
/*********************************************************************/
/*                                                                   */
/* This procedure computes the range of the ACTION_SYMBOLS map after */
/* Optimal Partitioning has been used to compress that map.  Its     */
/* first argument is an array, STATE_START, that indicates the       */
/* starting location in the compressed vector for each state.  When  */
/* a value of STATE_START is negative it indicates that the state in */
/* question shares its elements with another state.  Its second      */
/* argument, STATE_STACK, is an array that contains the elements of  */
/* the partition created by PARTSET.  Each element of the partition  */
/* is organized as a circular list where the smallest sets appear    */
/* first in the list.                                                */
/*                                                                   */
/*********************************************************************/
void compute_action_symbols_range(short *state_start,
                                  short *state_stack,
                                  short *state_list,
                                  short *action_symbols_range)
{
    int i,
        j,
        k,
        state_no,
        state,
        symbol,
        symbol_root;

    BOOLEAN end_node;

    short *symbol_list;

    symbol_list = Allocate_short_array(num_symbols + 1);

    /*********************************************************************/
    /* We now write out the range elements of the ACTION_SYMBOLS map.    */
    /* Recall that if STATE_START has a negative value, then the set in  */
    /* question is sharing elements and does not need to be processed.   */
    /*********************************************************************/
    k = 0;
    for ALL_SYMBOLS(j)
        symbol_list[j] = OMEGA;     /* Initialize all links to OMEGA */

    for ALL_STATES(i)
    {
        state_no = state_list[i];
        if (state_start[state_no] > 0)
        {
            symbol_root = 0;       /* Add "fence" element: 0 to list */
            symbol_list[symbol_root] = NIL;

            /*************************************************************/
            /* Pop a state from the stack,  and add each of its elements */
            /* that has not yet been processed into the list.            */
            /* Continue until stack is empty...                          */
            /* Recall that the stack is represented by a circular queue. */
            /*************************************************************/
            for (end_node = ((state = state_no) == NIL);
                 ! end_node; end_node = (state == state_no))
            {
                struct shift_header_type  sh;
                struct reduce_header_type red;

                state = state_stack[state];
                sh = shift[statset[state].shift_number];
                for (j = 1; j <= sh.size; j++)
                {
                    symbol = SHIFT_SYMBOL(sh, j);
                    if (symbol_list[symbol] == OMEGA)
                    {
                        symbol_list[symbol] = symbol_root;
                        symbol_root = symbol;
                    }
                }

                red = reduce[state];
                for (j = 1; j <= red.size; j++)
                {
                    symbol = REDUCE_SYMBOL(red, j);
                    if (symbol_list[symbol] == OMEGA)
                    {
                        symbol_list[symbol] = symbol_root;
                        symbol_root = symbol;
                    }
                }
            }

            /*************************************************************/
            /* Write the list out.                                       */
            /*************************************************************/
            for (symbol = symbol_root; symbol != NIL; symbol = symbol_root)
            {
                symbol_root = symbol_list[symbol];

                symbol_list[symbol] = OMEGA;
                action_symbols_range[k++] = symbol;
            }
        }
    }

    ffree(symbol_list);

    return;
}


/*********************************************************************/
/*                   COMPUTE_NACTION_SYMBOLS_RANGE:                  */
/*********************************************************************/
/* This procedure computes the range of the NACTION_SYMBOLS map. It  */
/* organization is analoguous to COMPUTE_ACTION_SYMBOLS_RANGE.       */
/*********************************************************************/
void compute_naction_symbols_range(short *state_start,
                                   short *state_stack,
                                   short *state_list,
                                   short *naction_symbols_range)
{
    int i,
        j,
        k,
        state_no,
        state,
        symbol,
        symbol_root;

    BOOLEAN end_node;

    short *symbol_list;

    symbol_list = Allocate_short_array(num_symbols + 1);

    /*********************************************************************/
    /* We now write out the range elements of the NACTION_SYMBOLS map.   */
    /* Recall that if STATE_START has a negative value, then the set in  */
    /* question is sharing elements and does not need to be processed.   */
    /*********************************************************************/
    k = 0;
    for ALL_SYMBOLS(j)
        symbol_list[j] = OMEGA;     /* Initialize all links to OMEGA */

    for ALL_STATES(i)
    {
        state_no = state_list[i];
        if (state_start[state_no] > 0)
        {
            symbol_root = 0;       /* Add "fence" element: 0 to list */
            symbol_list[symbol_root] = NIL;

           /*************************************************************/
           /* Pop a state from the stack,  and add each of its elements */
           /* that has not yet been processed into the list.            */
           /* Continue until stack is empty...                          */
           /* Recall that the stack is represented by a circular queue. */
           /*************************************************************/
            for (end_node =  ((state = state_no) == NIL);
                 ! end_node; end_node = (state == state_no))
            {
                state = state_stack[state];
                for (j = gd_index[state];
                     j <= gd_index[state + 1] - 1; j++)
                {
                    symbol = gd_range[j];
                    if (symbol_list[symbol] == OMEGA)
                    {
                        symbol_list[symbol] = symbol_root;
                        symbol_root = symbol;
                    }
                }
            }

            /**********************************************************/
            /* Write the list out.                                    */
            /**********************************************************/
            for (symbol = symbol_root; symbol != NIL; symbol = symbol_root)
            {
                symbol_root = symbol_list[symbol];

                symbol_list[symbol] = OMEGA;
                naction_symbols_range[k++] = symbol;
            }
        }
    }

    ffree(symbol_list);

    return;
}
