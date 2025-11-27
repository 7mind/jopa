static char hostfile[] = __FILE__;

#include <string.h>
#include "common.h"
#include "header.h"

/*****************************************************************************/
/*                                 PTSTATS:                                  */
/*****************************************************************************/
/*          PT_STATS prints all the states of the parser.                    */
/*****************************************************************************/
void ptstats(void)
{
    int max_size,
        state_no,
        symbol,
        number,
        i;

    struct goto_header_type   go_to;
    struct shift_header_type  sh;
    struct reduce_header_type red;

    char temp[SYMBOL_SIZE + 1],
    line[MAX_LINE_SIZE + 1];

    PR_HEADING;
    fprintf(syslis,"Shift STATES: ");

    for ALL_STATES(state_no)  /* iterate over the states */
    {
        print_state(state_no);

        max_size = 0;

        /****************************************************************/
        /* Compute the size of the largest symbol.  The MAX_SIZE cannot */
        /* be larger than PRINT_LINE_SIZE - 17 to allow for printing of */
        /* headers for actions to be taken on the symbols.              */
        /****************************************************************/
        sh = shift[statset[state_no].shift_number];
        for (i = 1; i <= sh.size; i++)
        {
            symbol = SHIFT_SYMBOL(sh, i);
            restore_symbol(temp, RETRIEVE_STRING(symbol));
            max_size = MAX(max_size, strlen(temp));
        }

        go_to = statset[state_no].go_to;
        for (i = 1; i <= go_to.size; i++)
        {
            symbol = GOTO_SYMBOL(go_to, i);
            restore_symbol(temp, RETRIEVE_STRING(symbol));
            max_size = MAX(max_size, strlen(temp));
        }

        red = reduce[state_no];
        for (i = 1; i <= red.size; i++)
        {
            symbol = REDUCE_SYMBOL(red, i);
            restore_symbol(temp, RETRIEVE_STRING(symbol));
            max_size = MAX(max_size, strlen(temp));
        }

        max_size = MIN(max_size, PRINT_LINE_SIZE - 17);

        /**************************************************************/
        /* 1) Print all Shift actions.                                */
        /* 2) Print all Goto actions.                                 */
        /* 3) Print all reduce actions.                               */
        /* 4) If there is a default then print it.                    */
        /**************************************************************/
        if (sh.size > 0)
        {
            fprintf(syslis, "\n");
            ENDPAGE_CHECK;
            for (i = 1; i <= sh.size; i++)
            {
                symbol = SHIFT_SYMBOL(sh, i);
                restore_symbol(temp, RETRIEVE_STRING(symbol));
                print_large_token(line, temp, "", max_size);
                number = ABS(SHIFT_ACTION(sh, i));
                if (SHIFT_ACTION(sh, i) > (short) num_states)
                {
                    fprintf(syslis,
                            "\n%-*s    La/Sh  %d",
                            max_size, line, number);
                    ENDPAGE_CHECK;
                }
                else if (SHIFT_ACTION(sh, i) > 0)
                {
                    fprintf(syslis,
                            "\n%-*s    Shift  %d",
                            max_size, line, number);
                    ENDPAGE_CHECK;
                }
                else
                {
                    fprintf(syslis,
                            "\n%-*s    Sh/Rd  %d",
                            max_size, line, number);
                    ENDPAGE_CHECK;
                }
            }
        }

        if (go_to.size > 0)
        {
            fprintf(syslis, "\n");
            ENDPAGE_CHECK;
            for (i = 1; i <= go_to.size; i++)
            {
                symbol = GOTO_SYMBOL(go_to, i);
                restore_symbol(temp, RETRIEVE_STRING(symbol));
                print_large_token(line, temp, "", max_size);
                number = ABS(GOTO_ACTION(go_to, i));
                if (GOTO_ACTION(go_to, i) > 0)
                {
                    fprintf(syslis,
                            "\n%-*s    Goto   %d",
                            max_size, line, number);
                    ENDPAGE_CHECK;
                }
                else
                {
                    fprintf(syslis,
                            "\n%-*s    Gt/Rd  %d",
                            max_size, line, number);
                    ENDPAGE_CHECK;
                }
            }
        }

        if (red.size != 0)
        {
            fprintf(syslis, "\n");
            ENDPAGE_CHECK;
            for (i = 1; i <= red.size; i++)
            {
                symbol = REDUCE_SYMBOL(red, i);
                restore_symbol(temp, RETRIEVE_STRING(symbol));
                print_large_token(line, temp, "", max_size);
                number = REDUCE_RULE_NO(red, i);
                if (rules[number].lhs != accept_image)
                {
                    fprintf(syslis,
                            "\n%-*s    Reduce %d",
                            max_size, line, number);
                    ENDPAGE_CHECK;
                }
                else
                {
                    fprintf(syslis, "\n%-*s    Accept",max_size, line);
                    ENDPAGE_CHECK;
                }
            }
        }

        if (default_opt > 0)
        {
            if (REDUCE_RULE_NO(red, 0) != OMEGA)
            {
                fprintf(syslis,
                        "\n\nDefault reduction to rule  %d",
                        REDUCE_RULE_NO(red, 0));
                output_line_no++;
                ENDPAGE_CHECK;
            }
        }
    }

    if (max_la_state > num_states)
    {
        PR_HEADING;
        fprintf(syslis,"Look-Ahead STATES:");
    }
    for ALL_LA_STATES(state_no)
    {
        char buffer[PRINT_LINE_SIZE + 1];

        i = number_len(state_no) + 8; /* 8 = length of "STATE" */
                                      /* + 2 spaces + newline  */
        fill_in(buffer, (PRINT_LINE_SIZE - i) ,'-');

        fprintf(syslis, "\n\n\nSTATE %d %s",state_no, buffer);
        output_line_no += 2;
        ENDPAGE_CHECK;

        /**************************************************************/
        /* Print the set of states that have transitions to STATE_NO. */
        /**************************************************************/
        if (lastats[state_no].in_state == state_no)
        {
            fprintf(syslis,"\n(Unreachable State)\n");
            output_line_no++;
            ENDPAGE_CHECK;
        }
        else
        {
            fprintf(syslis,"\n(%d)\n", lastats[state_no].in_state);
            output_line_no++;
            ENDPAGE_CHECK;

            max_size = 0;

            /*********************************************************/
            /* Compute the size of the largest symbol.  The MAX_SIZE */
            /* cannot be larger than PRINT_LINE_SIZE - 17 to allow   */
            /* for printing of headers for actions to be taken on    */
            /* the symbols.                                          */
            /*********************************************************/
            sh = shift[lastats[state_no].shift_number];
            for (i = 1; i <= sh.size; i++)
            {
                symbol = SHIFT_SYMBOL(sh, i);
                restore_symbol(temp, RETRIEVE_STRING(symbol));
                max_size = MAX(max_size, strlen(temp));
            }

            red = lastats[state_no].reduce;
            for (i = 1; i <= red.size; i++)
            {
                symbol = REDUCE_SYMBOL(red, i);
                restore_symbol(temp, RETRIEVE_STRING(symbol));
                max_size = MAX(max_size, strlen(temp));
            }

            max_size = MIN(max_size, PRINT_LINE_SIZE - 17);

            /**********************************************************/
            /* 1) Print all Shift actions.                            */
            /* 2) Print all Goto actions.                             */
            /* 3) Print all reduce actions.                           */
            /* 4) If there is a default then print it.                */
            /**********************************************************/
            fprintf(syslis, "\n");
            ENDPAGE_CHECK;
            for (i = 1; i <= sh.size; i++)
            {
                symbol = SHIFT_SYMBOL(sh, i);
                restore_symbol(temp, RETRIEVE_STRING(symbol));
                print_large_token(line, temp, "", max_size);
                number = ABS(SHIFT_ACTION(sh, i));
                if (SHIFT_ACTION(sh, i) > (short) num_states)
                {
                    fprintf(syslis,
                            "\n%-*s    La/Sh  %d",
                            max_size, line, number);
                    ENDPAGE_CHECK;
                }
                else if (SHIFT_ACTION(sh, i) > 0)
                {
                    fprintf(syslis,
                            "\n%-*s    Shift  %d",
                            max_size, line, number);
                    ENDPAGE_CHECK;
                }
                else
                {
                    fprintf(syslis,
                            "\n%-*s    Sh/Rd  %d",
                            max_size, line, number);
                    ENDPAGE_CHECK;
                }
            }

            fprintf(syslis, "\n");
            ENDPAGE_CHECK;
            for (i = 1; i <= red.size; i++)
            {
                symbol = REDUCE_SYMBOL(red, i);
                restore_symbol(temp, RETRIEVE_STRING(symbol));
                print_large_token(line, temp, "", max_size);
                number = REDUCE_RULE_NO(red, i);
                fprintf(syslis,
                        "\n%-*s    Reduce %d", max_size, line, number);
                ENDPAGE_CHECK;
            }

            if (default_opt > 0 && REDUCE_RULE_NO(red, 0) != OMEGA)
            {
                fprintf(syslis,
                        "\n\nDefault reduction to rule  %d",
                        REDUCE_RULE_NO(red, 0));
                output_line_no++;
                ENDPAGE_CHECK;
            }
        }
    }

    fprintf(syslis,"\n");
    ENDPAGE_CHECK;

    return;
}
