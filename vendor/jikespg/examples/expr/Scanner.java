// $Id: Scanner.java,v 1.3 1999/11/04 14:02:18 shields Exp $
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://www.ibm.com/research/jikes.
// Copyright (C) 1983, 1999, International Business Machines Corporation
// and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.

//
// The Scanner object
//
class Scanner implements exprsym
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
        while (next_byte != '\n' && Character.isSpace((char) next_byte))
            next_byte = System.in.read();
        return;
    }

    //
    //
    //
    String scan_symbol() throws java.io.IOException
    {
        StringBuffer buffer = new StringBuffer();
        while (next_byte != '\n' && (! Character.isSpace((char) next_byte)))
        {
            buffer.append((char) next_byte);
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

        next_byte = System.in.read();
        for (skip_spaces(); next_byte != '\n'; skip_spaces())
        {
            Token token = new Token();
            switch(next_byte)
            {
                case '+':
                     token.kind = TK_PLUS;
                     token.name = "+";
                     break;
                case '-':
                     token.kind = TK_MINUS;
                     token.name = "-";
                     break;
                case '*':
                     token.kind = TK_STAR;
                     token.name = "*";
                     break;
                case '/':
                     token.kind = TK_SLASH;
                     token.name = "/";
                     break;
                case '(':
                     token.kind = TK_LPAREN;
                     token.name = "(";
                     break;
                case ')':
                     token.kind = TK_RPAREN;
                     token.name = ")";
                     break;
                default:
                     StringBuffer buffer = new StringBuffer();
                     if (! Character.isDigit((char) next_byte))
                     {
                         token.kind = TK_ERROR;
                         while (next_byte != '\n' && (! Character.isSpace((char) next_byte)))
                         {
                             buffer.append((char) next_byte);
                             next_byte = System.in.read();
                         }
                         System.out.println("The token \"" + buffer.toString() + "\" is illegal"); 
                     }
                     else
                     {
                         token.kind = TK_NUMBER;
                         do
                         {
                             buffer.append((char) next_byte);
                             next_byte = System.in.read();
                         } while (next_byte != '\n' && Character.isDigit((char) next_byte));
                    }

                    token.name = buffer.toString();
	    }

            if (token.kind != TK_ERROR && token.kind != TK_NUMBER)
                next_byte = System.in.read();
    
            lex_stream.tokens.addElement(token);
        }

        Token end_token = new Token();
        end_token.kind = TK_EOF;
        end_token.name = "";
        lex_stream.tokens.addElement(end_token);

        return;
    }
}
