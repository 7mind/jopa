#include "diagnose.h"
#include "control.h"
#include "semantic.h"
#include "case.h"
#include "spell.h"
#include "option.h"
#include "stream.h"


namespace Jopa { // Open namespace Jopa block
void DiagnoseParser::ReallocateStacks()
{
    int old_stack_length = stack_length;

    stack_length += STACK_INCREMENT;

    assert(stack_length <= SHRT_MAX);

    int* old_stack = stack;
    stack = (int*) memcpy(new int[stack_length], stack,
                          old_stack_length * sizeof(int));
    delete [] old_stack;

    Location* old_location_stack = location_stack;
    location_stack = (Location*) memcpy(new Location[stack_length],
                                        location_stack,
                                        old_stack_length * sizeof(Location));
    delete [] old_location_stack;

    Ast** old_parse_stack = parse_stack;
    parse_stack = (Ast**) memcpy(new Ast*[stack_length], parse_stack,
                                 old_stack_length * sizeof(Ast*));
    delete [] old_parse_stack;

    int* old_temp_stack = temp_stack;
    temp_stack = (int*) memcpy(new int[stack_length], temp_stack,
                               old_stack_length * sizeof(int));
    delete [] old_temp_stack;

    int* old_next_stack = next_stack;
    next_stack = (int*) memcpy(new int[stack_length], next_stack,
                               old_stack_length * sizeof(int));
    delete [] old_next_stack;

    int* old_prev_stack = prev_stack;
    prev_stack = (int*) memcpy(new int[stack_length], prev_stack,
                               old_stack_length * sizeof(int));
    delete [] old_prev_stack;

    int* old_scope_index = scope_index;
    scope_index = (int*) memcpy(new int[stack_length], scope_index,
                                old_stack_length * sizeof(int));
    delete [] old_scope_index;

    int* old_scope_position = scope_position;
    scope_position = (int*) memcpy(new int[stack_length], scope_position,
                                   old_stack_length * sizeof(int));
    delete [] old_scope_position;
}


void DiagnoseParser::DiagnoseParse()
{
    lex_stream -> Reset();

    TokenObject curtok = lex_stream -> Gettoken();

    int prev_pos,
        pos,
        next_pos,
        i,
        tok,
        lhs_symbol,
        act = START_STATE;

    ReallocateStacks();

/*****************************************************************/
/* Start parsing.                                                */
/*****************************************************************/
    state_stack_top = 0;
    stack[state_stack_top] = act;

    tok = lex_stream -> Kind(curtok);
    location_stack[state_stack_top] = Loc(curtok);

    //
    // Process a terminal
    //
    do
    {
        /*************************************************************/
        /* Synchronize state stacks and update the location stack.   */
        /*************************************************************/
        prev_pos = -1;
        prev_stack_top = -1;

        next_pos = -1;
        next_stack_top = -1;

        pos = state_stack_top;
        temp_stack_top = state_stack_top - 1;
        for (i = 0; i <= state_stack_top; i++)
            temp_stack[i] = stack[i];

        act = t_action(act, tok, lex_stream);
/*****************************************************************/
/* When a reduce action is encountered, we compute all REDUCE    */
/* and associated goto actions induced by the current token.     */
/* Eventually, a SHIFT, SHIFT-REDUCE, ACCEPT or ERROR action is  */
/* computed...                                                   */
/*****************************************************************/
        while (act <= NUM_RULES)
        {
            do
            {
                temp_stack_top -= (rhs[act]-1);
                //
                // if no error detected?
                //     semantic_action(act);
                //
                act = nt_action(temp_stack[temp_stack_top], lhs[act]);
            } while (act <= NUM_RULES);
            /*************************************************/
            /* ... Update the maximum useful position of the */
            /* (STATE_)STACK, push goto state into stack, and*/
            /* compute next action on current symbol ...     */
            /*************************************************/
            if (temp_stack_top + 1 >= stack_length)
                ReallocateStacks();
            pos = Min(pos, temp_stack_top);
            temp_stack[temp_stack_top + 1] = act;
            act = t_action(act, tok, lex_stream);
        }

/*****************************************************************/
/* At this point, we have a shift, shift-reduce, accept or error */
/* action.  STACK contains the configuration of the state stack  */
/* prior to executing any action on curtok. next_stack contains  */
/* the configuration of the state stack after executing all      */
/* reduce actions induced by curtok.  The variable pos indicates */
/* the highest position in STACK that is still useful after the  */
/* reductions are executed.                                      */
/*****************************************************************/
        while (act > ERROR_ACTION ||       /* SHIFT-REDUCE action */
               act < ACCEPT_ACTION)               /* SHIFT action */
        {
            next_stack_top = temp_stack_top + 1;
            for (i = next_pos + 1; i <= next_stack_top; i++)
                next_stack[i] = temp_stack[i];

            for (i = pos + 1; i <= next_stack_top; i++)
                location_stack[i] = location_stack[state_stack_top];

            /*****************************************************/
            /* If we have a shift-reduce, process it as well as  */
            /* the goto-reduce actions that follow it.           */
            /*****************************************************/
            if (act > ERROR_ACTION)
            {
                act -= ERROR_ACTION;
                do
                {
                    next_stack_top -= (rhs[act]-1);
                    //
                    // if no error detected?
                    //     semantic_action(act);
                    //
                    act = nt_action(next_stack[next_stack_top], lhs[act]);
                } while (act <= NUM_RULES);
                pos = Min(pos, next_stack_top);
            }

            if (next_stack_top + 1 >= stack_length)
                ReallocateStacks();

            temp_stack_top = next_stack_top;
            next_stack[++next_stack_top] = act;
            next_pos = next_stack_top;

            /********************************************************/
            /* Simulate the parser through the next token without   */
            /* destroying STACK or next_stack.                      */
            /********************************************************/
            curtok = lex_stream -> Gettoken();
            tok = lex_stream -> Kind(curtok);
            act = t_action(act, tok, lex_stream);
            while (act <= NUM_RULES)
            {
                /*************************************************/
                /* ... Process all goto-reduce actions following */
                /* reduction, until a goto action is computed ...*/
                /*************************************************/
                do
                {
                    lhs_symbol = lhs[act];
                    temp_stack_top -= (rhs[act]-1);
                    //
                    // if no error detected?
                    //     semantic_action(act);
                    //
                    act = (temp_stack_top > next_pos
                               ? temp_stack[temp_stack_top]
                               : next_stack[temp_stack_top]);
                    act = nt_action(act, lhs_symbol);
                }   while (act <= NUM_RULES);

                /*************************************************/
                /* ... Update the maximum useful position of the */
                /* (STATE_)STACK, push GOTO state into stack, and*/
                /* compute next action on current symbol ...     */
                /*************************************************/
                if (temp_stack_top + 1 >= stack_length)
                    ReallocateStacks();

                next_pos = Min(next_pos, temp_stack_top);
                temp_stack[temp_stack_top + 1] = act;
                act = t_action(act, tok, lex_stream);
            }

            /*****************************************************/
            /* No error was detected, Read next token into       */
            /* PREVTOK element, advance CURTOK pointer and       */
            /* update stacks.                                    */
            /*****************************************************/
            if (act != ERROR_ACTION)
            {
                prev_stack_top = state_stack_top;
                for (i = prev_pos + 1; i <= prev_stack_top; i++)
                    prev_stack[i] = stack[i];
                prev_pos = pos;

                state_stack_top = next_stack_top;
                for (i = pos + 1; i <= state_stack_top; i++)
                    stack[i] = next_stack[i];
                location_stack[state_stack_top] = Loc(curtok);
                pos = next_pos;
            }
        }

        /*********************************************************/
        /* At this stage, either we have an ACCEPT or an ERROR   */
        /* action.                                               */
        /*********************************************************/
        if (act == ERROR_ACTION)
        {
            //
            // An error was detected.
            //

            RepairCandidate candidate = ErrorRecovery(curtok);

            act = stack[state_stack_top];

/*****************************************************************/
/* If the recovery was successful on a nonterminal candidate,    */
/* parse through that candidate and "read" the next token.       */
/*****************************************************************/
            if (!candidate.symbol)
                break;
            else if (candidate.symbol > NT_OFFSET)
            {
                lhs_symbol = candidate.symbol - NT_OFFSET;
                act = nt_action(act, lhs_symbol);
                while (act <= NUM_RULES)
                {
                    state_stack_top -= (rhs[act]-1);
                    act = nt_action(stack[state_stack_top], lhs[act]);
                }
                stack[++state_stack_top] = act;
                curtok = lex_stream -> Gettoken();
                tok = lex_stream -> Kind(curtok);
                location_stack[state_stack_top] = Loc(curtok);
            }
            else
            {
                tok = candidate.symbol;
                location_stack[state_stack_top] = candidate.location;
            }
        }
    } while (act != ACCEPT_ACTION);

    error.PrintMessages();
}


/*****************************************************************/
/*  This routine is invoked when an error is encountered.  It    */
/* tries to diagnose the error and recover from it.  If it is    */
/* successful, the state stack, the current token and the buffer */
/* are readjusted; i.e., after a successful recovery,            */
/* state_stack_top points to the location in the state stack     */
/* that contains the state on which to recover; curtok           */
/* identifies the symbol on which to recover.                    */
/*                                                               */
/* Up to three configurations may be available when this routine */
/* is invoked. PREV_STACK may contain the sequence of states     */
/* preceding any action on prevtok, STACK always contains the    */
/* sequence of states preceding any action on curtok, and        */
/* NEXT_STACK may contain the sequence of states preceding any   */
/* action on the successor of curtok.                            */
/*****************************************************************/
RepairCandidate DiagnoseParser::ErrorRecovery(TokenObject error_token)
{
    TokenObject prevtok = lex_stream -> Previous(error_token);

    RepairCandidate candidate;

/*****************************************************************/
/* Try primary phase recoveries. If not successful, try secondary*/
/* phase recoveries.  If not successful and we are at end of the */
/* file, we issue the end-of-file error and quit. Otherwise, ... */
/*****************************************************************/
    candidate = PrimaryPhase(error_token);
    if (candidate.symbol)
        return candidate;

    candidate = SecondaryPhase(error_token);
    if (candidate.symbol)
        return candidate;

    if (lex_stream -> Kind(error_token) == EOFT_SYMBOL)
    {
        error.Report(0, EOF_CODE, terminal_index[EOFT_SYMBOL],
                     Loc(prevtok), prevtok);
        candidate.symbol = 0;
        candidate.location = Loc(error_token);
        return candidate;
    }

/*****************************************************************/
/* At this point, primary and (initial attempt at) secondary     */
/* recovery did not work.  We will now get into "panic mode" and */
/* keep trying secondary phase recoveries until we either find   */
/* a successful recovery or have consumed the remaining input    */
/* tokens.                                                       */
/*****************************************************************/
    while (lex_stream -> Kind(buffer[BUFF_UBOUND]) != EOFT_SYMBOL)
    {
        candidate = SecondaryPhase(buffer[MAX_DISTANCE - MIN_DISTANCE + 2]);
        if (candidate.symbol)
            return candidate;
    }

/*****************************************************************/
/* We reached the end of the file while panicking. Delete all    */
/* remaining tokens in the input.                                */
/*****************************************************************/
    int i;
    for (i = BUFF_UBOUND; lex_stream -> Kind(buffer[i]) == EOFT_SYMBOL; i--)
        ;

    error.Report(0, DELETION_CODE, terminal_index[lex_stream -> Kind(prevtok)],
                 Loc(error_token), buffer[i]);

    candidate.symbol = 0;
    candidate.location = Loc(buffer[i]);

    return candidate;
}


/*****************************************************************/
/* This function tries primary and scope recovery on each        */
/* available configuration.  If a successful recovery is found   */
/* and no secondary phase recovery can do better, a diagnosis is */
/* issued, the configuration is updated and the function returns */
/* "true".  Otherwise, it returns "false".                       */
/*****************************************************************/
RepairCandidate DiagnoseParser::PrimaryPhase(TokenObject error_token)
{
    PrimaryRepairInfo repair,
                      new_repair;

    RepairCandidate candidate;

    repair.distance = 0;
    repair.misspell_index = 0;
    repair.code = ERROR_CODE;

    candidate.symbol = 0;

/*****************************************************************/
/* Initialize the buffer.                                        */
/*****************************************************************/
    int i = (next_stack_top >= 0 ? 3 : 2);
    buffer[i] = error_token;

    int k;
    for (k = i; k > 0; k--)
        buffer[k - 1] = lex_stream -> Previous(buffer[k]);

    for (k = i + 1; k < BUFF_SIZE; k++)
        buffer[k] = lex_stream -> Next(buffer[k - 1]);

/*****************************************************************/
/* If NEXT_STACK_TOP > 0 then the parse was successful on CURTOK */
/* and the error was detected on the successor of CURTOK. In     */
/* that case, first check whether or not primary recovery is     */
/* possible on next_stack ...                                    */
/*****************************************************************/
    if (next_stack_top >= 0)
    {
        repair.buffer_position = 3;
        repair = CheckPrimaryDistance(next_stack, next_stack_top, repair);
    }

/*****************************************************************/
/* ... Next, try primary recovery on the current token...        */
/*****************************************************************/
    new_repair = repair;

    new_repair.buffer_position = 2;
    new_repair = CheckPrimaryDistance(stack, state_stack_top, new_repair);
    if (new_repair.distance > repair.distance ||
        new_repair.misspell_index > repair.misspell_index)
        repair = new_repair;

/*****************************************************************/
/* Finally, if prev_stack_top >= 0 then try primary recovery on  */
/* the prev_stack configuration.                                 */
/*****************************************************************/
    if (prev_stack_top >= 0)
    {
        new_repair = repair;
        new_repair.buffer_position = 1;
        new_repair = CheckPrimaryDistance(prev_stack,
                                          prev_stack_top, new_repair);
        if (new_repair.distance > repair.distance ||
            new_repair.misspell_index > repair.misspell_index)
            repair = new_repair;
    }

/*****************************************************************/
/* Before accepting the best primary phase recovery obtained,    */
/* ensure that we cannot do better with a similar secondary      */
/* phase recovery.                                               */
/*****************************************************************/
    if (next_stack_top >= 0)             /* next_stack available */
    {
        if (SecondaryCheck(next_stack, next_stack_top, 3, repair.distance))
              return candidate;
    }
    else if (SecondaryCheck(stack, state_stack_top, 2, repair.distance))
             return candidate;

/*****************************************************************/
/* First, adjust distance if the recovery is on the error token; */
/* it is important that the adjustment be made here and not at   */
/* each primary trial to prevent the distance tests from being   */
/* biased in favor of deferred recoveries which have access to   */
/* more input tokens...                                          */
/*****************************************************************/
    repair.distance = repair.distance - repair.buffer_position + 1;

/*****************************************************************/
/* ...Next, adjust the distance if the recovery is a deletion or */
/* (some form of) substitution...                                */
/*****************************************************************/
    if (repair.code == INVALID_CODE      ||
        repair.code == DELETION_CODE     ||
        repair.code == SUBSTITUTION_CODE ||
        repair.code == MERGE_CODE)
         repair.distance--;

/*****************************************************************/
/* ... After adjustment, check if the most successful primary    */
/* recovery can be applied.  If not, continue with more radical  */
/* recoveries...                                                 */
/*****************************************************************/
    if (repair.distance < MIN_DISTANCE)
        return candidate;

/*****************************************************************/
/* When processing an insertion error, if the token preceeding   */
/* the error token is not available, we change the repair code   */
/* into a BEFORE_CODE to instruct the reporting routine that it  */
/* indicates that the repair symbol should be inserted before    */
/* the error token.                                              */
/*****************************************************************/
    if (repair.code == INSERTION_CODE)
    {
        if (! lex_stream -> Kind(buffer[repair.buffer_position - 1]))
            repair.code = BEFORE_CODE;
    }

/*****************************************************************/
/* Select the proper sequence of states on which to recover,     */
/* update stack accordingly and call diagnostic routine.         */
/*****************************************************************/
    if (repair.buffer_position == 1)
    {
        state_stack_top = prev_stack_top;
        for (i = 0; i <= state_stack_top; i++)
            stack[i] = prev_stack[i];
    }
    else if (next_stack_top >= 0 && repair.buffer_position >= 3)
    {
        state_stack_top = next_stack_top;
        for (i = 0; i <= state_stack_top; i++)
            stack[i] = next_stack[i];
        location_stack[state_stack_top] = Loc(buffer[3]);
    }

    return PrimaryDiagnosis(repair);
}


/*****************************************************************/
/*     This function checks whether or not a given state has a   */
/* candidate, whose string representaion is a merging of the two */
/* tokens at positions buffer_position and buffer_position+1 in  */
/* the buffer.  If so, it returns the candidate in question;     */
/* otherwise it returns 0.                                       */
/*****************************************************************/
int DiagnoseParser::MergeCandidate(int state, int buffer_position)
{
    int i;
    int k;
    int len;
    int len1;
    int len2;

    const wchar_t* p1;
    const wchar_t* p2;

    wchar_t str[MAX_TERM_LENGTH + 1];

    len1 = 0;
    for (p1 = lex_stream -> NameString(buffer[buffer_position]); *p1; p1++)
        len1++;
    len2 = 0;
    for (p2 = lex_stream -> NameString(buffer[buffer_position + 1]); *p2; p2++)
        len2++;
    len  = len1 + len2;

    p1 = lex_stream -> NameString(buffer[buffer_position]);
    p2 = lex_stream -> NameString(buffer[buffer_position + 1]);

    if (len <= MAX_TERM_LENGTH)
    {
        wchar_t* p0;

        p0 = &str[0];
        for (i = 0; i < len1; i++)
            *(p0++) = p1[i];
        for (i = 0; i < len2; i++)
            *(p0++) = p2[i];
        *p0 = U_NULL;

        for (i = asi(state); asr[i] != 0; i++)
        {
            k = terminal_index[asr[i]];
            if (len == name_length[k])
            {
                const char* p3 = &string_buffer[name_start[k]];

                p0 = &str[0];
                while (*p0 != U_NULL)
                {
                    wchar_t c = *(p3++);

                    if (Case::ToAsciiLower(*p0) != Case::ToAsciiLower(c))
                        break;
                    p0++;
                }
                if (*p0 == U_NULL)
                    return asr[i];
            }
        }
    }

    return 0;
}


/*****************************************************************/
/* This procedure takes as arguments a parsing configuration     */
/* consisting of a state stack (stack and stack_top) and a fixed */
/* number of input tokens (starting at buffer_position) in the   */
/* input BUFFER; and some reference arguments: repair_code,      */
/* distance, misspell_index, candidate, and stack_position       */
/* which it sets based on the best possible recovery that it     */
/* finds in the given configuration.  The effectiveness of a     */
/* a repair is judged based on two criteria:                     */
/*                                                               */
/*   1) the number of tokens that can be parsed after the repair */
/*      is applied: distance.                                    */
/*   2) how close to perfection is the candidate that is chosen: */
/*      misspell_index.                                          */
/* When this procedure is entered, distance, misspell_index and  */
/* repair_code are assumed to be initialized.                    */
/*****************************************************************/
PrimaryRepairInfo DiagnoseParser::CheckPrimaryDistance
       (int stck[], int stack_top, PrimaryRepairInfo repair)
{
    PrimaryRepairInfo scope_repair;
    int i,
        j,
        k,
        next_state,
        max_pos,
        act,
        root,
        symbol,
        tok;

/*****************************************************************/
/*  First, try scope and manual recovery.                        */
/*****************************************************************/
#if defined(SCOPE_REPAIR)
    scope_repair = ScopeTrial(stck, stack_top, repair);
    if (scope_repair.distance > repair.distance)
        repair = scope_repair;
#endif

/*****************************************************************/
/*  Next, try merging the error token with its successor.        */
/*****************************************************************/
    symbol = MergeCandidate(stck[stack_top], repair.buffer_position);
    if (symbol != 0)
    {
        j = ParseCheck(stck, stack_top,
                       symbol, repair.buffer_position+2);
        if ((j > repair.distance) ||
            (j == repair.distance && repair.misspell_index < 10))
        {
            repair.misspell_index = 10;
            repair.symbol = symbol;
            repair.distance = j;
            repair.code = MERGE_CODE;
        }
    }

/*****************************************************************/
/* Next, try deletion of the error token.                        */
/*****************************************************************/
    j = ParseCheck(stck, stack_top,
                   lex_stream -> Kind(buffer[repair.buffer_position+1]),
                   repair.buffer_position+2);
    if (lex_stream -> Kind(buffer[repair.buffer_position]) == EOLT_SYMBOL &&
        lex_stream -> AfterEol(buffer[repair.buffer_position+1]))
         k = 10;
    else k = 0;
    if (j > repair.distance ||
        (j == repair.distance && k > repair.misspell_index))
    {
        repair.misspell_index = k;
        repair.code = DELETION_CODE;
        repair.distance = j;
    }

/*****************************************************************/
/* Update the error configuration by simulating all reduce and   */
/* goto actions induced by the error token. Then assign the top  */
/* most state of the new configuration to next_state.            */
/*****************************************************************/
    next_state = stck[stack_top];
    max_pos = stack_top;
    temp_stack_top = stack_top - 1;

    tok = lex_stream -> Kind(buffer[repair.buffer_position]);
    lex_stream -> Reset(buffer[repair.buffer_position + 1]);
    act = t_action(next_state, tok, lex_stream);
    while (act <= NUM_RULES)
    {
        do
        {
            temp_stack_top -= (rhs[act]-1);
            symbol = lhs[act];
            act = (temp_stack_top > max_pos
                                  ? temp_stack[temp_stack_top]
                                  : stck[temp_stack_top]);
            act = nt_action(act, symbol);
        }   while (act <= NUM_RULES);
        max_pos = Min(max_pos, temp_stack_top);
        temp_stack[temp_stack_top + 1] = act;
        next_state = act;
        act = t_action(next_state, tok, lex_stream);
    }

/*****************************************************************/
/*  Next, place the list of candidates in proper order.          */
/*****************************************************************/
    root = 0;
    for (i = asi(next_state); asr[i] != 0; i++)
    {
        symbol = asr[i];
        if (symbol != EOFT_SYMBOL && symbol != ERROR_SYMBOL)
        {
            if (root == 0)
                list[symbol] = symbol;
            else
            {
                list[symbol] = list[root];
                list[root] = symbol;
            }
            root = symbol;
        }
    }

    if (stck[stack_top] != next_state)
    {
        for (i = asi(stck[stack_top]); asr[i] != 0; i++)
        {
            symbol = asr[i];
            if (symbol != EOFT_SYMBOL && symbol != ERROR_SYMBOL &&
                                         list[symbol] == 0)
            {
                if (root == 0)
                    list[symbol] = symbol;
                else
                {
                    list[symbol] = list[root];
                    list[root] = symbol;
                }
                root = symbol;
            }
        }
    }

    i = list[root];
    list[root] = 0;
    root = i;

/*****************************************************************/
/*  Next, try insertion for each possible candidate available in */
/* the current state, except EOFT and ERROR_SYMBOL.              */
/*****************************************************************/
    symbol = root;
    while (symbol != 0)
    {
        if (symbol == EOLT_SYMBOL &&
            lex_stream -> AfterEol(buffer[repair.buffer_position]))
             k = 10;
        else k = 0;
        j = ParseCheck(stck, stack_top,
                       symbol, repair.buffer_position);
        if (j > repair.distance)
        {
            repair.misspell_index = k;
            repair.distance = j;
            repair.symbol = symbol;
            repair.code = INSERTION_CODE;
        }
        else if (j == repair.distance && k > repair.misspell_index)
        {
            repair.misspell_index = k;
            repair.distance = j;
            repair.symbol = symbol;
            repair.code = INSERTION_CODE;
        }

        symbol = list[symbol];
    }

/*****************************************************************/
/*  Next, Try substitution for each possible candidate available */
/* in the current state, except EOFT and ERROR_SYMBOL.           */
/*****************************************************************/
    symbol = root;
    while (symbol != 0)
    {
        if (symbol == EOLT_SYMBOL &&
            lex_stream -> AfterEol(buffer[repair.buffer_position+1]))
             k = 10;
        else k = Misspell(symbol, buffer[repair.buffer_position]);
        j = ParseCheck(stck, stack_top,
                       symbol, repair.buffer_position+1);
        if (j > repair.distance)
        {
            repair.misspell_index = k;
            repair.distance = j;
            repair.symbol = symbol;
            repair.code = SUBSTITUTION_CODE;
        }
        else if (j == repair.distance && k > repair.misspell_index)
        {
            repair.misspell_index = k;
            repair.symbol = symbol;
            repair.code = SUBSTITUTION_CODE;
        }
        i = symbol;
        symbol = list[symbol];
        list[i] = 0;                             /* reset element */
    }


/*****************************************************************/
/* Next, we try to insert a nonterminal candidate in front of the*/
/* error token, or substituting a nonterminal candidate for the  */
/* error token. Precedence is given to insertion.                */
/*****************************************************************/
     for (i = nasi(stck[stack_top]); nasr[i] != 0; i++)
     {
         symbol = nasr[i] + NT_OFFSET;
         j = ParseCheck(stck, stack_top,
                        symbol, repair.buffer_position+1);
         if (j > repair.distance)
         {
             repair.misspell_index = 0;
             repair.distance = j;
             repair.symbol = symbol;
             repair.code = INVALID_CODE;
         }

         j = ParseCheck(stck, stack_top, symbol, repair.buffer_position);
         if ((j > repair.distance) ||
             (j == repair.distance && repair.code == INVALID_CODE))
         {
             repair.misspell_index = 0;
             repair.distance = j;
             repair.symbol = symbol;
             repair.code = INSERTION_CODE;
         }
     }

    return repair;
}


/*****************************************************************/
/* This procedure is invoked to issue a diagnostic message and   */
/* adjust the input buffer.  The recovery in question is either  */
/* the insertion of one or more scopes, the merging of the error */
/* token with its successor, the deletion of the error token,    */
/* the insertion of a single token in front of the error token   */
/* or the substitution of another token for the error token.     */
/*****************************************************************/
RepairCandidate DiagnoseParser::PrimaryDiagnosis(PrimaryRepairInfo repair)
{
    RepairCandidate candidate;

    int name_index;

/*****************************************************************/
/*  Issue diagnostic.                                            */
/*****************************************************************/
    TokenObject prevtok = buffer[repair.buffer_position - 1],
                curtok  = buffer[repair.buffer_position];

    switch (repair.code)
    {
        case INSERTION_CODE: case BEFORE_CODE:
        {
            if (repair.symbol > NT_OFFSET)
                 name_index = GetNtermIndex(stack[state_stack_top],
                                            repair.symbol,
                                            repair.buffer_position);
            else name_index = GetTermIndex(stack,
                                           state_stack_top,
                                           repair.symbol,
                                           repair.buffer_position);
            TokenObject t = (repair.code == INSERTION_CODE ? prevtok : curtok);
            error.Report(0, repair.code, name_index, Loc(t), t);
            break;
        }
        case INVALID_CODE:
        {
            name_index = GetNtermIndex(stack[state_stack_top],
                                       repair.symbol,
                                       repair.buffer_position + 1);
            error.Report(0, repair.code, name_index, Loc(curtok), curtok);
            break;
        }
        case SUBSTITUTION_CODE:
        {
            if (repair.misspell_index >= 6)
                name_index = terminal_index[repair.symbol];
            else
            {
                name_index = GetTermIndex(stack, state_stack_top,
                                          repair.symbol,
                                          repair.buffer_position + 1);
                if (name_index != terminal_index[repair.symbol])
                    repair.code = INVALID_CODE;
            }
            error.Report(0, repair.code, name_index, Loc(curtok), curtok);
            break;
        }
        case MERGE_CODE:
        {
            error.Report(0, repair.code, terminal_index[repair.symbol],
                         Loc(curtok), lex_stream -> Next(curtok));
            break;
        }
#if defined(SCOPE_REPAIR)
        case SCOPE_CODE:
        {
            for (int i = 0; i < scope_stack_top; i++)
            {
                error.Report(0, repair.code,
                                -scope_index[i],
                                location_stack[scope_position[i]],
                                prevtok,
                                non_terminal_index[scope_lhs[scope_index[i]]]);
            }

            repair.symbol = scope_lhs[scope_index[scope_stack_top]] + NT_OFFSET;
            state_stack_top = scope_position[scope_stack_top];
            error.Report(0, repair.code,
                            -scope_index[scope_stack_top],
                            location_stack[scope_position[scope_stack_top]],
                            prevtok,
                            GetNtermIndex(stack[state_stack_top],
                                          repair.symbol,
                                          repair.buffer_position)
                           );
            break;
        }
#endif
        default: /* deletion */
        {
            error.Report(0, repair.code, terminal_index[ERROR_SYMBOL],
                                         Loc(curtok), curtok);
        }
    }

/*****************************************************************/
/*  Update buffer.                                               */
/*****************************************************************/
    switch (repair.code)
    {
        case INSERTION_CODE: case BEFORE_CODE:
#if defined(SCOPE_REPAIR)
        case SCOPE_CODE:
#endif
        {
            candidate.symbol = repair.symbol;
            candidate.location = Loc(buffer[repair.buffer_position]);
            lex_stream -> Reset(buffer[repair.buffer_position]);

            break;
        }

        case INVALID_CODE: case SUBSTITUTION_CODE:
        {
            candidate.symbol = repair.symbol;
            candidate.location = Loc(buffer[repair.buffer_position]);
            lex_stream -> Reset(buffer[repair.buffer_position + 1]);

            break;
        }

        case MERGE_CODE:
        {
            candidate.symbol = repair.symbol;
            candidate.location = Loc(buffer[repair.buffer_position]);
            lex_stream -> Reset(buffer[repair.buffer_position + 2]);

            break;
        }

        default: /* deletion */
        {
            candidate.location = Loc(buffer[repair.buffer_position + 1]);
            candidate.symbol =
                      lex_stream -> Kind(buffer[repair.buffer_position + 1]);
            lex_stream -> Reset(buffer[repair.buffer_position + 2]);

            break;
        }
    }

    return candidate;
}


/*****************************************************************/
/* This function takes as parameter an integer STACK_TOP that    */
/* points to a STACK element containing the state on which a     */
/* primary recovery will be made; the terminal candidate on which*/
/* to recover; and an integer: buffer_position, which points to  */
/* the position of the next input token in the BUFFER.  The      */
/* parser is simulated until a shift (or shift-reduce) action    */
/* is computed on the candidate.  Then we proceed to compute the */
/* the name index of the highest level nonterminal that can      */
/* directly or indirectly produce the candidate.                 */
/*****************************************************************/
int DiagnoseParser::GetTermIndex(int stck[], int stack_top,
                                 int tok, int buffer_position)
{
/*****************************************************************/
/* Initialize stack index of temp_stack and initialize maximum   */
/* position of state stack that is still useful.                 */
/*****************************************************************/
    int act = stck[stack_top],
        max_pos = stack_top,
        highest_symbol = tok;

    temp_stack_top = stack_top - 1;

/*****************************************************************/
/* Compute all reduce and associated actions induced by the      */
/* candidate until a SHIFT or SHIFT-REDUCE is computed. ERROR    */
/* and ACCEPT actions cannot be computed on the candidate in     */
/* this context, since we know that it is suitable for recovery. */
/*****************************************************************/
    lex_stream -> Reset(buffer[buffer_position]);
    act = t_action(act, tok, lex_stream);
    while (act <= NUM_RULES)
    {
        /*********************************************************/
        /* Process all goto-reduce actions following reduction,  */
        /* until a goto action is computed ...                   */
        /*********************************************************/
        do
        {
            temp_stack_top -= (rhs[act]-1);
            int lhs_symbol = lhs[act];
            act = (temp_stack_top > max_pos
                                  ? temp_stack[temp_stack_top]
                                  : stck[temp_stack_top]);
            act = nt_action(act, lhs_symbol);
        } while (act <= NUM_RULES);
        /*********************************************************/
        /* Compute new maximum useful position of (STATE_)stack, */
        /* push goto state into the stack, and compute next      */
        /* action on candidate ...                               */
        /*********************************************************/
        max_pos = Min(max_pos, temp_stack_top);
        temp_stack[temp_stack_top + 1] = act;
        act = t_action(act, tok, lex_stream);
    }

/********************************************************************/
/* At this stage, we have simulated all actions induced by the      */
/* candidate and we are ready to shift or shift-reduce it. First,   */
/* set tok and next_ptr appropriately and identify the candidate    */
/* as the initial highest_symbol. If a shift action was computed    */
/* on the candidate, update the stack and compute the next          */
/* action. Next, simulate all actions possible on the next input    */
/* token until we either have to shift it or are about to reduce    */
/* below the initial starting point in the stack (indicated by      */
/* max_pos as computed in the previous loop).  At that point,       */
/* return the highest_symbol computed.                              */
/********************************************************************/
    temp_stack_top++;   /* adjust top of stack to reflect last goto */
                        /* next move is shift or shift-reduce.      */
    int threshold = temp_stack_top;

    tok = lex_stream -> Kind(buffer[buffer_position]);
    lex_stream -> Reset(buffer[buffer_position + 1]);

    if (act > ERROR_ACTION)   /* shift-reduce on candidate? */
        act -= ERROR_ACTION;
    else                      /* shift on candidate ! */
    {
        temp_stack[temp_stack_top + 1] = act;
        act = t_action(act, tok, lex_stream);
    }

    while (act <= NUM_RULES)
    {
        /*********************************************************/
        /* Process all goto-reduce actions following reduction,  */
        /* until a goto action is computed ...                   */
        /*********************************************************/
        do
        {
            temp_stack_top -= (rhs[act]-1);

            if (temp_stack_top < threshold)
                goto quit;

            int lhs_symbol = lhs[act];
            if (temp_stack_top == threshold)
                highest_symbol = lhs_symbol + NT_OFFSET;
            act = (temp_stack_top > max_pos
                                  ? temp_stack[temp_stack_top]
                                  : stck[temp_stack_top]);
            act = nt_action(act, lhs_symbol);
        } while (act <= NUM_RULES);

        temp_stack[temp_stack_top + 1] = act;
        act = t_action(act, tok, lex_stream);
    }

 quit:
    return (highest_symbol > NT_OFFSET
                           ? non_terminal_index[highest_symbol - NT_OFFSET]
                           : terminal_index[highest_symbol]);
}

/*****************************************************************/
/* This function takes as parameter a starting state number:     */
/* start, a nonterminal symbol, A (candidate), and an integer,   */
/* buffer_position,  which points to the position of the next    */
/* input token in the BUFFER.                                    */
/* It returns the highest level non-terminal B such that         */
/* B =>*rm A.  I.e., there does not exists a nonterminal C such  */
/* that C =>+rm B. (Recall that for an LALR(k) grammar if        */
/* C =>+rm B, it cannot be the case that B =>+rm C)              */
/*****************************************************************/
int DiagnoseParser::GetNtermIndex(int start, int sym, int buffer_position)
{
    int act,
        tok,
        highest_symbol;

    highest_symbol = sym - NT_OFFSET;

    tok = lex_stream -> Kind(buffer[buffer_position]);
    lex_stream -> Reset(buffer[buffer_position + 1]);

/*****************************************************************/
/* Initialize stack index of temp_stack and initialize maximum   */
/* position of state stack that is still useful.                 */
/*****************************************************************/
    temp_stack_top = 0;
    temp_stack[temp_stack_top] = start;

    act = nt_action(start, highest_symbol);
    if (act > NUM_RULES) /* goto action? */
    {
        temp_stack[temp_stack_top + 1] = act;
        act = t_action(act, tok, lex_stream);
    }

    while (act <= NUM_RULES)
    {
        /*********************************************************/
        /* Process all goto-reduce actions following reduction,  */
        /* until a goto action is computed ...                   */
        /*********************************************************/
        do
        {
            temp_stack_top -= (rhs[act]-1);
            if (temp_stack_top < 0)
                return non_terminal_index[highest_symbol];
            if (temp_stack_top == 0)
                highest_symbol = lhs[act];
            act = nt_action(temp_stack[temp_stack_top], lhs[act]);
        } while (act <= NUM_RULES);
        temp_stack[temp_stack_top + 1] = act;
        act = t_action(act, tok, lex_stream);
    }

    return non_terminal_index[highest_symbol];
}


/*****************************************************************/
/*     Check whether or not there is a high probability that a   */
/* given string is a misspelling of another.                     */
/* Certain singleton symbols (such as ":" and ";") are also      */
/* considered to be misspelling of each other.                   */
/*****************************************************************/
int DiagnoseParser::Misspell(int sym, TokenObject tok)
{
    int len = name_length[terminal_index[sym]];

    wchar_t* keyword = new wchar_t[len + 1];
    for (int i = name_start[terminal_index[sym]], j = 0; j < len; i++, j++)
    {
        wchar_t c = string_buffer[i];
        keyword[j] = c;
    }
    keyword[len] = U_NULL;

    int index = Spell::Index(lex_stream -> NameString(tok), keyword);

    delete [] keyword;

    return index;
}


/*****************************************************************/
/*****************************************************************/
PrimaryRepairInfo DiagnoseParser::ScopeTrial(int stck[], int stack_top,
                                             PrimaryRepairInfo repair)
{
    state_seen = new int[stack_length];
    for (int i = 0; i < stack_length; i++)
        state_seen[i] = NIL;

    ScopeTrialCheck(stck, stack_top, repair, 0);

    delete [] state_seen;
    state_pool.Reset();

    repair.code = SCOPE_CODE;
    repair.misspell_index = 10;

    return repair;
}


/*****************************************************************/
/* SCOPE_TRIAL_CHECK is a recursive procedure that takes as arguments  */
/* a state configuration: STACK and STACK_TOP, and an integer:   */
/* INDX. In addition, a global variable SCOPE_DISTANCE indicates */
/* the distance to "beat" and SCOPE_BUFFER_POSITION identifies   */
/* the starting position of the next input tokens in BUFFER.     */
/* SCOPE_TRIAL determines whether or not scope recovery is       */
/* possible.  If so, it uses two global arrays: SCOPE_INDEX and  */
/* SCOPE_LOCATION to store the necessary information about the   */
/* scopes to be closed. A global variable, scope_stack_top, identifies */
/* the highest element used in these arrays.                     */
/* The global variable SCOPE_DISTANCE is also updated to reflect */
/* the new result.                                               */
/*****************************************************************/
void DiagnoseParser::ScopeTrialCheck(int stck[], int stack_top,
                                     PrimaryRepairInfo& repair, int indx)
{
    int max_pos,
        marked_pos,
        stack_position,
        distance,
        previous_distance,
        i,
        j,
        k,
        act,
        tok,
        lhs_symbol;

    act = stck[stack_top];

    for (i = state_seen[stack_top]; i != NIL; i = state_pool[i].next)
        if (state_pool[i].state == act)
            return;

    i = state_pool.NextIndex();
    state_pool[i].state = act;
    state_pool[i].next  = state_seen[stack_top];
    state_seen[stack_top] = i;

    for (i = 0; i < SCOPE_SIZE; i++)
    {
        /*********************************************************/
        /* Use the scope lookahead symbol to force all reductions*/
        /* inducible by that symbol.                             */
        /*********************************************************/
        act = stck[stack_top];
        temp_stack_top = stack_top - 1;
        max_pos = stack_top;
        tok = scope_la[i];
        lex_stream -> Reset(buffer[repair.buffer_position]);
        act = t_action(act, tok, lex_stream);
        while (act <= NUM_RULES)
        {
            /*************************************************/
            /* ... Process all goto-reduce actions following */
            /* reduction, until a goto action is computed ...*/
            /*************************************************/
            do
            {
                temp_stack_top -= (rhs[act]-1);
                lhs_symbol = lhs[act];
                act =  (temp_stack_top > max_pos
                            ?  temp_stack[temp_stack_top]
                            :  stck[temp_stack_top]);
                act = nt_action(act, lhs_symbol);
            }  while (act <= NUM_RULES);
            if (temp_stack_top + 1 >= stack_length)
                return;
            max_pos = Min(max_pos, temp_stack_top);
            temp_stack[temp_stack_top + 1] = act;
            act = t_action(act, tok, lex_stream);
        }

        /*********************************************************/
        /* If the lookahead symbol is parsable, then we check    */
        /* whether or not we have a match between the scope      */
        /* prefix and the transition symbols corresponding to    */
        /* the states on top of the stack.                       */
        /*********************************************************/
        if (act != ERROR_ACTION)
        {
            k = scope_prefix[i];
            for (j = temp_stack_top + 1;
                 j >= (max_pos + 1) &&
                 in_symbol(temp_stack[j]) == scope_rhs[k]; j--)
                 k++;
            if (j == max_pos)
                for (j = max_pos;
                     j >= 1 && in_symbol(stck[j]) == scope_rhs[k];
                     j--)
                    k++;
            /*****************************************************/
            /* If the prefix matches, check whether the state    */
            /* newly exposed on top of the stack, (after the     */
            /* corresponding prefix states are popped from the   */
            /* stack), is in the set of "source states" for the  */
            /* scope in question and that it is at a position    */
            /* below the threshold indicated by MARKED_POS.      */
            /*****************************************************/
            marked_pos = (max_pos < stack_top ? max_pos + 1
                                              : stack_top);
            if (scope_rhs[k] == 0 && j < marked_pos)   /* match? */
            {
                stack_position = j;
                for (j = scope_state_set[i];
                     stck[stack_position] != scope_state[j] &&
                     scope_state[j] != 0;
                     j++);
                /*************************************************/
                /* If the top state is valid for scope recovery, */
                /* the left-hand side of the scope is used as    */
                /* starting symbol and we calculate how far the  */
                /* parser can advance within the forward context */
                /* after parsing the left-hand symbol.           */
                /*************************************************/
                if (scope_state[j] != 0)      /* state was found */
                {
                    previous_distance = repair.distance;
                    distance = ParseCheck(stck,
                                          stack_position,
                                          scope_lhs[i]+NT_OFFSET,
                                          repair.buffer_position);
                    /*********************************************/
                    /* if the recovery is not successful, we     */
                    /* update the stack with all actions induced */
                    /* by the left-hand symbol, and recursively  */
                    /* call SCOPE_TRIAL_CHECK to try again.      */
                    /* Otherwise, the recovery is successful. If */
                    /* the new distance is greater than the      */
                    /* initial SCOPE_DISTANCE, we update         */
                    /* SCOPE_DISTANCE and set scope_stack_top to INDX  */
                    /* to indicate the number of scopes that are */
                    /* to be applied for a succesful  recovery.  */
                    /* NOTE that this procedure cannot get into  */
                    /* an infinite loop, since each prefix match */
                    /* is guaranteed to take us to a lower point */
                    /* within the stack.                         */
                    /*********************************************/
                    if ((distance - repair.buffer_position + 1) <
                        MIN_DISTANCE)
                    {
                        int top = stack_position;
                        act = nt_action(stck[top], scope_lhs[i]);
                        while (act <= NUM_RULES)
                        {
                            top -= (rhs[act]-1);
                            act = nt_action(stck[top], lhs[act]);
                        }
                        top++;

                        j = act;
                        act = stck[top];  /* save */
                        stck[top] = j;    /* swap */
                        ScopeTrialCheck(stck, top, repair, indx+1);
                        stck[top] = act; /* restore */
                    }
                    else if (distance > repair.distance)
                    {
                        scope_stack_top = indx;
                        repair.distance = distance;
                    }

                    if (lex_stream -> Kind(buffer[repair.buffer_position]) ==
                        EOFT_SYMBOL &&
                        repair.distance == previous_distance)
                    {
                        scope_stack_top = indx;
                        repair.distance = MAX_DISTANCE;
                    }

                    /*********************************************/
                    /* If this scope recovery has beaten the     */
                    /* previous distance, then we have found a   */
                    /* better recovery (or this recovery is one  */
                    /* of a list of scope recoveries). Record    */
                    /* its information at the proper location    */
                    /* (INDX) in SCOPE_INDEX and SCOPE_STACK.    */
                    /*********************************************/
                    if (repair.distance > previous_distance)
                    {
                        scope_index[indx] = i;
                        scope_position[indx] = stack_position;
                        return;
                    }
                }
            }
        }
    }
}

/*****************************************************************/
/* This function computes the ParseCheck distance for the best   */
/* possible secondary recovery for a given configuration that    */
/* either deletes none or only one symbol in the forward context.*/
/* If the recovery found is more effective than the best primary */
/* recovery previously computed, then the function returns true. */
/* Only misplacement, scope and manual recoveries are attempted; */
/* simple insertion or substitution of a nonterminal are tried   */
/* in CHECK_PRIMARY_DISTANCE as part of primary recovery.        */
/*****************************************************************/
bool DiagnoseParser::SecondaryCheck(int stck[], int stack_top,
                                    int buffer_position, int distance)
{
    PrimaryRepairInfo repair;

    int top,
        j;

    for (top = stack_top - 1; top >= 0; top--)
    {
        j = ParseCheck(stck, top,
                       lex_stream -> Kind(buffer[buffer_position]),
                       buffer_position + 1);
        if (((j - buffer_position + 1) > MIN_DISTANCE) &&
            (j > distance))
            return true;
    }

#if defined(SCOPE_REPAIR)
    repair.buffer_position = buffer_position + 1;
    repair.distance = distance;
    repair = ScopeTrial(stck, stack_top, repair);
    if ((repair.distance - buffer_position) > MIN_DISTANCE &&
        repair.distance > distance)
         return true;
//#elif !defined(SCOPE_REPAIR)
//    manual_buffer_position = buffer_position + 1;
//    manual_distance = distance;
//    manual_trial(stck, stack_top);
//    if ((manual_distance - repair.buffer_position) > MIN_DISTANCE &&
//        manual_distance > distance)
//         return true;
#endif

    return false;
}


/*****************************************************************/
/* Secondary_phase is a boolean function that checks whether or  */
/* not some form of secondary recovery is applicable to one of   */
/* the error configurations. First, if "next_stack" is available,*/
/* misplacement and secondary recoveries are attempted on it.    */
/* Then, in any case, these recoveries are attempted on "stack". */
/* If a successful recovery is found, a diagnosis is issued, the */
/* configuration is updated and the function returns "true".     */
/* Otherwise, the function returns false.                        */
/*****************************************************************/
RepairCandidate DiagnoseParser::SecondaryPhase(TokenObject error_token)
{
    SecondaryRepairInfo repair,
                          misplaced;

    RepairCandidate candidate;

    int i,
        j,
        k,
        top,
        next_last_index = 0,
        last_index;

    candidate.symbol = 0;

    repair.code = ERROR_CODE;
    repair.distance = 0;
    repair.recovery_on_next_stack = false;

    misplaced.distance = 0;
    misplaced.recovery_on_next_stack = false;

/*****************************************************************/
/* If the next_stack is available, try misplaced and secondary   */
/* recovery on it first.                                         */
/*****************************************************************/
    if (next_stack_top >= 0)
    {
        Location  save_location;

        buffer[2] = error_token;
        buffer[1] = lex_stream -> Previous(buffer[2]);
        buffer[0] = lex_stream -> Previous(buffer[1]);

        for (k = 3; k < BUFF_UBOUND; k++)
            buffer[k] = lex_stream -> Next(buffer[k - 1]);

        buffer[BUFF_UBOUND] = BAD_TOKEN; /* elmt not available */

        /*********************************************************/
        /* If we are at the end of the input stream, compute the */
        /* index position of the first EOFT symbol (last useful  */
        /* index).                                               */
        /*********************************************************/
        for (next_last_index = MAX_DISTANCE - 1;
             next_last_index >= 1 &&
             lex_stream -> Kind(buffer[next_last_index]) == EOFT_SYMBOL;
             next_last_index--);
        next_last_index = next_last_index + 1;

        save_location = location_stack[next_stack_top];
        location_stack[next_stack_top] = Loc(buffer[2]);
        misplaced.num_deletions = next_stack_top;
        misplaced = MisplacementRecovery(next_stack, next_stack_top,
                                         next_last_index,
                                         misplaced, true);
        if (misplaced.recovery_on_next_stack)
            misplaced.distance++;

        repair.num_deletions = next_stack_top + BUFF_UBOUND;
        repair = SecondaryRecovery(next_stack, next_stack_top,
                                   next_last_index,
                                   repair, true);
        if (repair.recovery_on_next_stack)
            repair.distance++;

        location_stack[next_stack_top] = save_location;
    }
    else             /* next_stack not available, initialize ... */
    {
        misplaced.num_deletions = state_stack_top;
        repair.num_deletions = state_stack_top + BUFF_UBOUND;
    }

/*****************************************************************/
/* Try secondary recovery on the "stack" configuration.          */
/*****************************************************************/
    buffer[3] = error_token;

    buffer[2] = lex_stream -> Previous(buffer[3]);
    buffer[1] = lex_stream -> Previous(buffer[2]);
    buffer[0] = lex_stream -> Previous(buffer[1]);

    for (k = 4; k < BUFF_SIZE; k++)
        buffer[k] = lex_stream -> Next(buffer[k - 1]);

    for (last_index = MAX_DISTANCE - 1;
         last_index >= 1 &&
             lex_stream -> Kind(buffer[last_index]) == EOFT_SYMBOL;
         last_index--);
    last_index++;

    misplaced = MisplacementRecovery(stack, state_stack_top,
                                     last_index,
                                     misplaced, false);

    repair = SecondaryRecovery(stack, state_stack_top,
                               last_index, repair, false);

/*****************************************************************/
/* If a successful misplaced recovery was found, compare it with */
/* the most successful secondary recovery.  If the misplaced     */
/* recovery either deletes fewer symbols or parse-checks further */
/* then it is chosen.                                            */
/*****************************************************************/
    if (misplaced.distance > MIN_DISTANCE)
    {
        if (misplaced.num_deletions <= repair.num_deletions ||
           (misplaced.distance - misplaced.num_deletions) >=
           (repair.distance - repair.num_deletions))
        {
            repair.code = MISPLACED_CODE;
            repair.stack_position = misplaced.stack_position;
            repair.buffer_position = 2;
            repair.num_deletions = misplaced.num_deletions;
            repair.distance = misplaced.distance;
            repair.recovery_on_next_stack = misplaced.recovery_on_next_stack;
        }
    }

/*****************************************************************/
/* If the successful recovery was on next_stack, update: stack,  */
/* buffer, location_stack and last_index.                        */
/*****************************************************************/
    if (repair.recovery_on_next_stack)
    {
        state_stack_top = next_stack_top;
        for (i = 0; i <= state_stack_top; i++)
            stack[i] = next_stack[i];

        buffer[2] = error_token;
        buffer[1] = lex_stream -> Previous(buffer[2]);
        buffer[0] = lex_stream -> Previous(buffer[1]);

        for (k = 3; k < BUFF_UBOUND; k++)
            buffer[k] = lex_stream -> Next(buffer[k - 1]);

        buffer[BUFF_UBOUND] = BAD_TOKEN; /* elmt not available */

        location_stack[next_stack_top] = Loc(buffer[2]);
        last_index = next_last_index;
    }

#if defined(SCOPE_REPAIR)
/*****************************************************************/
/* Next, try scope recoveries after deletion of one, two, three, */
/* four ... buffer_position tokens from the input stream.        */
/*****************************************************************/
    if (repair.code == SECONDARY_CODE ||
        repair.code == DELETION_CODE)
    {
        PrimaryRepairInfo scope_repair;

        scope_repair.distance = 0;
        for (scope_repair.buffer_position = 2;
             scope_repair.buffer_position <= repair.buffer_position &&
             repair.code != SCOPE_CODE; scope_repair.buffer_position++)
        {
            scope_repair = ScopeTrial(stack, state_stack_top, scope_repair);
            j = (scope_repair.distance == MAX_DISTANCE
                                        ? last_index
                                        : scope_repair.distance);
            k = scope_repair.buffer_position - 1;
            if ((j - k) > MIN_DISTANCE &&
                (j - k) > (repair.distance - repair.num_deletions))
            {
                repair.code = SCOPE_CODE;
                i = scope_index[scope_stack_top];       /* upper bound */
                repair.symbol = scope_lhs[i] + NT_OFFSET;
                repair.stack_position = state_stack_top;
                repair.buffer_position = scope_repair.buffer_position;
            }
        }
    }

/*****************************************************************/
/* If no successful recovery is found and we have reached the    */
/* end of the file, check whether or not scope recovery is       */
/* applicable at the end of the file after discarding some       */
/* states.                                                       */
/*****************************************************************/
    if (repair.code == 0 &&
        lex_stream -> Kind(buffer[last_index]) == EOFT_SYMBOL)
    {
        PrimaryRepairInfo scope_repair;

        scope_repair.buffer_position = last_index;
        scope_repair.distance = 0;
        for (top = state_stack_top;
             top >= 0 && repair.code == 0; top--)
        {
            scope_repair = ScopeTrial(stack, top, scope_repair);
            if (scope_repair.distance > 0)
            {
                repair.code = SCOPE_CODE;
                i = scope_index[scope_stack_top];    /* upper bound */
                repair.symbol = scope_lhs[i] + NT_OFFSET;
                repair.stack_position = top;
                repair.buffer_position = scope_repair.buffer_position;
            }
        }
    }

#endif

/*****************************************************************/
/* If a successful repair was not found, quit!  Otherwise, issue */
/* diagnosis and adjust configuration...                         */
/*****************************************************************/
    if (repair.code == 0)
        return candidate;

    SecondaryDiagnosis(repair);

/*****************************************************************/
/* Update buffer based on number of elements that are deleted.   */
/*****************************************************************/
    switch (repair.code)
    {
        case MANUAL_CODE: case MISPLACED_CODE:
             candidate.location = Loc(buffer[2]);
             candidate.symbol = lex_stream -> Kind(buffer[2]);
             lex_stream -> Reset(lex_stream -> Next(buffer[2]));

             break;

        case DELETION_CODE:
             candidate.location = Loc(buffer[repair.buffer_position]);
             candidate.symbol =
                       lex_stream -> Kind(buffer[repair.buffer_position]);
             lex_stream -> Reset(lex_stream ->
                                 Next(buffer[repair.buffer_position]));

             break;

        default: /* SCOPE_CODE || SECONDARY_CODE */
             candidate.symbol = repair.symbol;
             candidate.location = Loc(buffer[repair.buffer_position]);
             lex_stream -> Reset(buffer[repair.buffer_position]);

             break;
    }

    return candidate;
}


/*****************************************************************/
/* This boolean function checks whether or not a given           */
/* configuration yields a better misplacement recovery than      */
/* the best misplacement recovery computed previously.           */
/*****************************************************************/
SecondaryRepairInfo DiagnoseParser::MisplacementRecovery
         (int stck[], int stack_top, int last_index,
          SecondaryRepairInfo repair, bool stack_flag)
{
    Location  previous_loc = Loc(buffer[2]);
    int stack_deletions = 0;

    for (int top = stack_top - 1; top >= 0; top--)
    {
        if (location_stack[top] < previous_loc)
            stack_deletions++;
        previous_loc = location_stack[top];

        int j = ParseCheck(stck, top, lex_stream -> Kind(buffer[2]), 3);
        if (j == MAX_DISTANCE)
             j = last_index;
        if ((j > MIN_DISTANCE) &&
            (j - stack_deletions) >
            (repair.distance - repair.num_deletions))
        {
            repair.stack_position = top;
            repair.distance = j;
            repair.num_deletions = stack_deletions;
            repair.recovery_on_next_stack = stack_flag;
        }
    }

    return repair;
}


/*****************************************************************/
/* This boolean function checks whether or not a given           */
/* configuration yields a better secondary recovery than the     */
/* best misplacement recovery computed previously.               */
/*****************************************************************/
SecondaryRepairInfo DiagnoseParser::SecondaryRecovery
         (int stck[], int stack_top,
          int last_index, SecondaryRepairInfo repair, bool stack_flag)
{
    int i,
        j,
        k,
        l,
        symbol,
        stack_deletions,
        top;

    Location  previous_loc;

    stack_deletions = 0;
    previous_loc = Loc(buffer[2]);
    for (top = stack_top;
         top >= 0 && repair.num_deletions >= stack_deletions; top--)
    {
        if (location_stack[top] < previous_loc)
            stack_deletions++;
        previous_loc = location_stack[top];

        for (i = 2;
             i <= (last_index - MIN_DISTANCE + 1) &&
             (repair.num_deletions >= (stack_deletions + i - 1)); i++)
        {
            j = ParseCheck(stck, top, lex_stream -> Kind(buffer[i]), i + 1);

            if (j == MAX_DISTANCE)
                 j = last_index;
            if ((j - i + 1) > MIN_DISTANCE)
            {
                k = stack_deletions + i - 1;
                if ((k < repair.num_deletions) ||
                    (j - k) > (repair.distance - repair.num_deletions) ||
                    ((repair.code == SECONDARY_CODE) &&
                     (j - k) == (repair.distance -
                                 repair.num_deletions)))
                {
                    repair.code = DELETION_CODE;
                    repair.distance = j;
                    repair.stack_position = top;
                    repair.buffer_position = i;
                    repair.num_deletions = k;
                    repair.recovery_on_next_stack = stack_flag;
                }
            }

            for (l = nasi(stck[top]); l >= 0 && nasr[l] != 0; l++)
            {
                symbol = nasr[l] + NT_OFFSET;
                j = ParseCheck(stck, top, symbol, i);
                if (j == MAX_DISTANCE)
                     j = last_index;
                if ((j - i + 1) > MIN_DISTANCE)
                {
                    k = stack_deletions + i - 1;
                    if (k < repair.num_deletions ||
                       (j - k) > (repair.distance -
                                  repair.num_deletions))
                    {
                        repair.code = SECONDARY_CODE;
                        repair.symbol = symbol;
                        repair.distance = j;
                        repair.stack_position = top;
                        repair.buffer_position = i;
                        repair.num_deletions = k;
                        repair.recovery_on_next_stack = stack_flag;
                    }
                }
            }
        }
    }

    return repair;
}


/*****************************************************************/
/* This procedure is invoked to issue a secondary diagnosis and  */
/* adjust the input buffer.  The recovery in question is either  */
/* an automatic scope recovery, a manual scope recovery, a       */
/* secondary substitution or a secondary deletion.               */
/*****************************************************************/
void DiagnoseParser::SecondaryDiagnosis(SecondaryRepairInfo repair)
{
/*****************************************************************/
/*  Issue diagnostic.                                            */
/*****************************************************************/
    switch (repair.code)
#if defined(SCOPE_REPAIR)
    {
        case SCOPE_CODE:
        {
            if (repair.stack_position < state_stack_top)
            {
                error.Report(0, DELETION_CODE,
                                terminal_index[ERROR_SYMBOL],
                                location_stack[repair.stack_position],
                                buffer[1]);
//                set_location(buffer[1],
//                             location_stack[repair.stack_position]);
            }
            for (int i = 0; i < scope_stack_top; i++)
            {
                error.Report(0, SCOPE_CODE,
                                -scope_index[i],
                                location_stack[scope_position[i]],
                                buffer[1],
                                non_terminal_index[scope_lhs[scope_index[i]]]);
            }

            repair.symbol = scope_lhs[scope_index[scope_stack_top]] + NT_OFFSET;
            state_stack_top = scope_position[scope_stack_top];
            error.Report(0, SCOPE_CODE,
                            -scope_index[scope_stack_top],
                            location_stack[scope_position[scope_stack_top]],
                            buffer[1],
                            GetNtermIndex(stack[state_stack_top],
                                          repair.symbol,
                                          repair.buffer_position)
                           );
            break;
        }
#endif
        default:
        {
            error.Report(0, repair.code,
                         (repair.code == SECONDARY_CODE
                          ? GetNtermIndex(stack[repair.stack_position],
                                          repair.symbol,
                                          repair.buffer_position)
                          : terminal_index[ERROR_SYMBOL]),
                         location_stack[repair.stack_position],
                         buffer[repair.buffer_position - 1]);
            state_stack_top = repair.stack_position;
        }
    }
}


ParseError::ParseError(Control& control_, LexStream* lex_stream_)
    : control(control_),
      lex_stream(lex_stream_),
      errors(256)
{
    ParseErrorInfo::emacs_style_report=!control_.option.errors;
}

//
// This procedure uses a  quick sort algorithm to sort the ERRORS
// by the left_line_no and left_column_no fields.
//
void ParseError::SortMessages()
{
     int lower,
         upper,
         lostack[32],
         histack[32];

     int top,
         i,
         j;
     ParseErrorInfo pivot, temp;

     top = 0;
     lostack[top] = 0;
     histack[top] = errors.Length() - 1;

     while (top >= 0)
     {
         lower = lostack[top];
         upper = histack[top];
         top--;

         while (upper > lower)
         {
             /*********************************************************/
             /* The array is most-likely almost sorted. Therefore,    */
             /* we use the middle element as the pivot element.       */
             /*********************************************************/
             i = (lower + upper) / 2;
             pivot = errors[i];
             errors[i] = errors[lower];

             /*********************************************************/
             /* Split the array section indicated by LOWER and UPPER  */
             /* using ARRAY(LOWER) as the pivot.                      */
             /*********************************************************/
             i = lower;
             for (j = lower + 1; j <= upper; j++)
                 if ((errors[j].left_token < pivot.left_token) ||
                 /*****************************************************/
                 /* When two error messages start in the same location*/
                 /* and one is nested inside the other, the outer one */
                 /* is placed first so that it can be printed last.   */
                 /* Recall that its right-span location is reached    */
                 /* after the inner one has been completely processed.*/
                 /*****************************************************/
                     (errors[j].left_token == pivot.left_token &&
                      errors[j].right_token > pivot.right_token) ||
                 /*****************************************************/
                 /* When two error messages are at the same location  */
                 /* span, check the NUM field to keep the sort stable.*/
                 /* When the location spans only a single symbol,     */
                 /* the one with the lowest "num" is placed first.    */
                 /*****************************************************/
                     (errors[j].left_token  == pivot.left_token  &&
                      errors[j].right_token == pivot.right_token &&
                      pivot.left_token == pivot.right_token     &&
                      errors[j].num < pivot.num)                       ||
                 /*****************************************************/
                 /* When two error messages are at the same location  */
                 /* which spans more than one symbol in the source,   */
                 /* the first message is treated as been nested into  */
                 /* the second message and (just like the nested case */
                 /* above) it is placed last in the sorted sequence.  */
                 /*****************************************************/
                     (errors[j].left_token  == pivot.left_token  &&
                      errors[j].right_token == pivot.right_token &&
                      pivot.left_token < pivot.right_token      &&
                      errors[j].num > pivot.num))
                 {
                     temp = errors[++i];
                     errors[i] = errors[j];
                     errors[j] = temp;
                 }
             errors[lower] = errors[i];
             errors[i] = pivot;

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
}


//
// This procedure is invoked by a JIKES PARSER or a semantic
// routine to process an error message.  The JIKES parser always
// passes the value 0 to msg_level to indicate an error.
// This routine simply stores all necessary information about
// the message into an array: error.
//
void ParseError::Report(int msg_level, ParseErrorCode msg_code,
                        int name_index, TokenIndex left_token,
                        TokenIndex right_token, int scope_name_index)
{
    int i = errors.NextIndex();

    errors[i].msg_level = msg_level;
    errors[i].msg_code = msg_code;
    errors[i].name_index = name_index;
    errors[i].num = i;
    errors[i].left_token = (left_token > Loc(right_token) ? Loc(right_token)
                            : left_token);
    errors[i].right_token = Loc(right_token);
    errors[i].scope_name_index = scope_name_index;

    if (control.option.dump_errors)
    {
        lex_stream -> RereadInput();
        PrintMessage(i);
        errors.Reset(1);
        // we only need to indicate that at least one error was detected...
        // See print_messages
    }
}

void ParseErrorInfo::Initialize(LexStream* l)
{
    lex_stream = l;

    left_line_no = lex_stream -> Line(left_token);
    left_column_no = lex_stream -> Column(left_token);
    right_line_no = lex_stream -> Line(right_token);
    right_column_no = lex_stream -> RightColumn(right_token);
}

int ParseErrorInfo::getLeftLineNo() { return left_line_no; }
int ParseErrorInfo::getLeftColumnNo() { return left_column_no; }
int ParseErrorInfo::getRightLineNo() { return right_line_no; }
int ParseErrorInfo::getRightColumnNo() { return right_column_no; }

JopaError::JopaErrorSeverity ParseErrorInfo::getSeverity()
{
    return JopaError::JOPA_ERROR;
}

const char* ParseErrorInfo::getFileName()
{
    assert(lex_stream);
    return lex_stream -> FileName();
}

const wchar_t* ParseErrorInfo::getErrorMessage()
{
    ErrorString s;
    const char* name = NULL;
    int i, len = 0;

#if defined(FULL_DIAGNOSIS)
    if (name_index >= 0)
    {
        len  = ParseError::name_length[name_index];
        name = &ParseError::string_buffer[ParseError::name_start[name_index]];
    }
#endif

    switch (msg_code)
    {
    case ERROR_CODE:
        s << "Parsing terminated at this token";
        break;

    case BEFORE_CODE:
        for (i = 0; i < len; i++)
            s << name[i];
        s << " inserted before this token";
        break;

    case INSERTION_CODE:
        for (i = 0; i < len; i++)
            s << name[i];
        s << " expected after this token";
        break;

    case DELETION_CODE:
        if (left_token == right_token)
            s << "Unexpected symbol ignored";
        else
            s << "Unexpected symbols ignored";
        break;

    case INVALID_CODE:
        if (len == 0)
            s << "Unexpected input discarded";
        else
        {
            s << "Invalid ";
            for (i = 0; i < len; i++)
                s << name[i];
        }
        break;

    case SUBSTITUTION_CODE:
        for (i = 0; i < len; i++)
            s << name[i];
        s << " expected instead of this token";
        break;

#if defined(SCOPE_REPAIR)
    case SCOPE_CODE:
        s << '\"';
        for (i = ParseError::scope_suffix[- (int)name_index];
             ParseError::scope_rhs[i] != 0; i++)
        {
            len  = ParseError::name_length[ParseError::scope_rhs[i]];
            name = &ParseError::string_buffer[ParseError::name_start[ParseError::scope_rhs[i]]];
            for (int j = 0; j < len; j++)
                s << name[j];
            if (ParseError::scope_rhs[i+1]) // any more symbols to print?
                s << ' ';
        }
        s << '\"';
        s << " inserted to complete ";
        //
        // TODO: This should not be an option
        //
        if (scope_name_index)
        {
            len  = ParseError::name_length[scope_name_index];
            name = &ParseError::string_buffer[ParseError::name_start[scope_name_index]];
            for (int j = 0; j < len; j++) // any more symbols to print?
                s << name[j];
        }
        else
            s << "scope";
        break;
#endif

    case MANUAL_CODE:
        s << '\"';
        for (i = 0; i < len; i++)
            s << name[i];
        s << "\" inserted to complete structure";
        break;

    case MERGE_CODE:
        s << "symbols merged to form ";
        for (i = 0; i < len; i++)
            s << name[i];
        break;

    case EOF_CODE:
        for (i = 0; i < len; i++)
            s << name[i];
        s << " reached after this token";
        break;

    case MISPLACED_CODE:
        s << "misplaced construct(s)";
        break;

    default:
        if (len == 0)
        {
            s << "unexpected input discarded";
        }
        else
        {
            for (i = 0; i < len; i++)
                s << name[i];
            s << " expected instead";
        }
        break;
    }

    return s.Array();
}

bool ParseErrorInfo::emacs_style_report = false;

const wchar_t* ParseErrorInfo::getErrorReport()
{
    return emacs_style_report ? emacsErrorString() : regularErrorString();
}

const wchar_t* ParseErrorInfo::regularErrorString()
{
    ErrorString s;

    lex_stream -> OutputSource(this, s);

    s << endl << "*** Syntax " << getSeverityString() << ": "
      << getErrorMessage();

    return s.Array();
}

const wchar_t* ParseErrorInfo::emacsErrorString()
{
    ErrorString s;

    s << getFileName()
      << ':' << left_line_no  << ':' << left_column_no
      << ':' << right_line_no << ':' << right_column_no
      << ": Syntax " << getSeverityString() << ": "
      << getErrorMessage();

    return s.Array();
}


/*****************************************************************/
/* This procedure is invoked to print JIKES error messages that  */
/* have been stored in the array: error.                         */
/*****************************************************************/
void ParseError::PrintMessages()
{
    if (control.option.dump_errors || errors.Length() == 0)
    {
        Coutput.flush();
        return;
    }

    if (control.option.errors)
    {
        Coutput << endl << "Found " << errors.Length() << " syntax error"
                << (errors.Length() == 1 ? "" : "s") << " in \""
                << lex_stream -> FileName() << "\":";
    }

    lex_stream -> RereadInput();

    if (! lex_stream -> InputBuffer())
    {
        char* file_name = lex_stream -> FileName();
        int length = lex_stream -> FileNameLength();
        wchar_t* name = new wchar_t[length + 1];
        for (int i = 0; i < length; i++)
            name[i] = file_name[i];
        name[length] = U_NULL;
        control.system_semantic ->
            ReportSemError(SemanticError::CANNOT_REOPEN_FILE, BAD_TOKEN, name);
        delete [] name;
        return;
    }

    SortMessages();

    int stack_top = -1;
    int* error_stack = new int[errors.Length()];

    for (unsigned k = 0; k < errors.Length(); k++)
    {
        for ( ; stack_top >= 0 &&
                  (errors[error_stack[stack_top]].right_token <=
                   errors[k].left_token);
              stack_top--)
        {
             if (stack_top == 0)
             {
                 PrintMessage(error_stack[stack_top]);
             }
             else
             {
                 int i = error_stack[stack_top],
                     j = error_stack[stack_top - 1];

                 if ((errors[i].msg_code != SECONDARY_CODE &&
                      errors[i].msg_code != MISPLACED_CODE &&
                      errors[i].msg_code != DELETION_CODE) ||
                     (errors[j].msg_code != DELETION_CODE  &&
                      errors[j].msg_code != MISPLACED_CODE &&
                      errors[j].msg_code != SECONDARY_CODE))
                 {
                     PrintMessage(i);
                 }
             }
        }

        if (errors[k].left_token < errors[k].right_token)
        {
            stack_top++;
            error_stack[stack_top] = k;
        }
        else if (!(stack_top >= 0 &&
                   (errors[error_stack[stack_top]].msg_code == SECONDARY_CODE ||
                    errors[error_stack[stack_top]].msg_code == DELETION_CODE) &&
                   (errors[k].left_token == errors[error_stack[stack_top]].left_token ||
                    errors[k].right_token == errors[error_stack[stack_top]].right_token)))
        {
            /* NOTE that input_line already contains a '\n' character */
            PrintMessage(k);
        }
    }

    for ( ; stack_top > 0; stack_top--)
    {
        int i = error_stack[stack_top],
            j = error_stack[stack_top - 1];

        if ((errors[i].msg_code != SECONDARY_CODE &&
             errors[i].msg_code != DELETION_CODE) ||
            (errors[j].msg_code != DELETION_CODE  &&
             errors[j].msg_code != SECONDARY_CODE))
        {
           PrintMessage(i);
        }
    }
    if (stack_top == 0)
        PrintMessage(error_stack[stack_top]);

    Coutput.flush();

    delete [] error_stack;

    if (control.option.errors)
        lex_stream -> DestroyInput();
}

void ParseError::PrintMessage(int k)
{
    errors[k].Initialize(lex_stream);
    JopaAPI::getInstance() -> reportError(&errors[k]);
}


} // Close namespace Jopa block

