// $Id: Parser.java,v 1.3 1999/11/04 14:02:16 shields Exp $
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://www.ibm.com/research/jikes.
// Copyright (C) 1996, 1998, International Business Machines Corporation
// and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.
class Parser extends bnfprs implements bnfsym
{
    final static int STACK_INCREMENT = 128;

    LexStream lex_stream;

    bnfhdr actions = new bnfhdr(this);

    int state_stack_top,
        stack[],
        location_stack[];
    int parse_stack[];

    //
    // Given a rule of the form     A ::= x1 x2 ... xn     n > 0
    //
    // the function TOKEN(i) yields the symbol xi, if xi is a terminal
    // or ti, if xi is a nonterminal that produced a string of the form
    // xi => ti w.
    //
    final int TOKEN(int i)
    {
        return location_stack[state_stack_top + (i - 1)];
    }

    //
    // Given a rule of the form     A ::= x1 x2 ... xn     n > 0
    //
    // The function SYM(i) yields the AST subtree associated with symbol
    // xi. NOTE that if xi is a terminal, SYM(i) is undefined ! (However,
    // see token_action below.)
    //
    // setSYM1(int INFO) is a function that allows us to assign an info
    // to SYM(1).
    //
    final int SYM(int i) { return parse_stack[state_stack_top + (i - 1)]; }
    final void setSYM1(int info) { parse_stack[state_stack_top] = info; }

    //
    // When a token is shifted, we may wish to perform an action on 
    // it. One possibility is to invoke "setSYM(null)" to associate
    // a null subtree with this terminal symbol.
    //
    void token_action(int tok)
    {
        setSYM1(0);
        System.out.println("Shifting token " + lex_stream.Name(tok));
    }

    Parser(LexStream lex_stream)
    {
        this.lex_stream = lex_stream;
    }

    void reallocate_stacks()
    {
        int old_stack[] = stack; 
        stack = new int[(old_stack == null ? 0 : old_stack.length) + STACK_INCREMENT];
        if (old_stack != null)
            System.arraycopy(old_stack, 0, stack, 0, old_stack.length);

        old_stack = location_stack; 
        location_stack = new int[(old_stack == null ? 0 : old_stack.length) + STACK_INCREMENT];
        if (old_stack != null)
            System.arraycopy(old_stack, 0, location_stack, 0, old_stack.length);

        int old_parse_stack[] = parse_stack; 
        parse_stack = new int[(old_parse_stack == null ? 0 : old_parse_stack.length) + STACK_INCREMENT];
        if (old_parse_stack != null)
            System.arraycopy(old_parse_stack, 0, parse_stack, 0, old_parse_stack.length);

        return;
    }
 
    void parse()
    {
        lex_stream.Reset();
        int curtok = lex_stream.Gettoken(),
            act = START_STATE,
            current_kind = lex_stream.Kind(curtok);
 
    /*****************************************************************/
    /* Start parsing.                                                */
    /*****************************************************************/
        state_stack_top = -1;

        ProcessTerminals: for (;;)
        {
            if (++state_stack_top >= (stack == null ? 0 : stack.length))
                reallocate_stacks();
 
            stack[state_stack_top] = act;

            location_stack[state_stack_top] = curtok;

            act = t_action(act, current_kind, lex_stream);
 
            if (act <= NUM_RULES)
                state_stack_top--; // make reduction look like a shift-reduce
            else if (act > ERROR_ACTION)
            {
                token_action(curtok);
                curtok = lex_stream.Gettoken();
                current_kind = lex_stream.Kind(curtok);

                act -= ERROR_ACTION;
            }
            else if (act < ACCEPT_ACTION)
            {
                token_action(curtok);
                curtok = lex_stream.Gettoken();
                current_kind = lex_stream.Kind(curtok);

                continue ProcessTerminals;
            }
            else break ProcessTerminals;

            ProcessNonTerminals: do
            {
                state_stack_top -= (rhs[act] - 1);
                actions.rule_action[act].action();
                act = nt_action(stack[state_stack_top], lhs[act]);
            } while(act <= NUM_RULES);
        }

        if (act == ERROR_ACTION)
        {
            //
            // Recover or Scream or Whatever !!!
            //
            System.out.println("Error detected on token " + curtok);
            return;
        }

        System.out.println("Input parsed successfully");
        return;
    }
}
