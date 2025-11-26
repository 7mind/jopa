// $Id: Scanner.java,v 1.3 1999/11/04 14:02:16 shields Exp $
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://www.ibm.com/research/jikes.
// Copyright (C) 1983, 1999, International Business Machines Corporation
// and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.

//
// The Scanner object
//
class Scanner implements bnfsym
{
    int next_byte;
    Option option;
    LexStream lex_stream;

    Scanner(Option option, LexStream lex_stream)
    {
        this.lex_stream = lex_stream;
        this.option = option;
    }

    //
    //
    //
    void skip_spaces() throws java.io.IOException
    {
        while (next_byte >= 0 && Character.isSpace((char) next_byte))
            next_byte = lex_stream.srcfile.read();
        return;
    }

    //
    //
    //
    String scan_symbol() throws java.io.IOException
    {
        StringBuffer buffer = new StringBuffer();
        while (next_byte >= 0 && (! Character.isSpace((char) next_byte)))
        {
            buffer.append((char) next_byte);
            next_byte = lex_stream.srcfile.read();
        }

        return buffer.toString();
    }

    //
    //
    //
    void scan() throws java.io.IOException
    {
        //
        // Do not use token indexed at location 0.
        //
        Token start_token = new Token();
        start_token.kind = 0;
        start_token.name = "";
        lex_stream.tokens.addElement(start_token);

        next_byte = lex_stream.srcfile.read();
        for (skip_spaces(); next_byte >= 0; skip_spaces())
        {
            Token token = new Token();
            token.name = scan_symbol();
            lex_stream.tokens.addElement(token);

            if (token.name.equals("::="))
                 token.kind = TK_PRODUCES;
            else if (token.name.equals("|"))
                 token.kind = TK_OR;
            else token.kind = TK_SYMBOL;
        }

        Token end_token = new Token();
        end_token.kind = TK_EOF;
        end_token.name = "";
        lex_stream.tokens.addElement(end_token);

        return;
    }
}
