// $Id: LexStream.java,v 1.3 1999/11/04 14:02:18 shields Exp $
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://www.ibm.com/research/jikes.
// Copyright (C) 1983, 1999, International Business Machines Corporation
// and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.

import java.util.Vector;
import java.io.InputStream;

//
// LexStream holds a stream of tokens generated from an input and 
// provides methods to retrieve information from the stream.
//
public class LexStream implements exprsym
{
    private int index;
    final static int INFINITY = Integer.MAX_VALUE;

    int Next(int i) { return (++i < tokens.size() ? i : tokens.size() - 1); }

    int Previous(int i) { return (i <= 0 ? 0 : i - 1); }

    int Peek() { return Next(index); }

    void Reset(int i) { index = Previous(i); }
    void Reset() { index = 0; }

    int Gettoken() { return index = Next(index); }

    int Gettoken(int end_token)
         { return index = (index < end_token ? Next(index) : tokens.size() - 1); }

    int Badtoken() { return 0; }

    int Kind(int i)   { return ((Token) tokens.elementAt(i)).kind; }

    String Name(int i) { return ((Token) tokens.elementAt(i)).name; }

    void dump()
    {
        for (int i = 1; i < tokens.size(); i++)
        {
            System.out.print(" (");
            switch(((Token) tokens.elementAt(i)).kind)
            {
                case TK_PLUS:      System.out.print("PLUS"); break;
                case TK_MINUS:     System.out.print("MINUS"); break;
                case TK_STAR:      System.out.print("STAR"); break;
                case TK_SLASH:     System.out.print("SLASH"); break;
                case TK_LPAREN:    System.out.print("LPAREN"); break;
                case TK_RPAREN:    System.out.print("RPAREN"); break;
                case TK_NUMBER:    System.out.print("NUMBER"); break;
                case TK_ERROR:     System.out.print("ERROR"); break;
                case TK_EOF:       System.out.print("EOF"); break;
                default:           System.out.print(((Token) tokens.elementAt(i)).kind);
            }
            System.out.print(") : \"");
            System.out.print(((Token) tokens.elementAt(i)).name);
            System.out.println("\"");
        }
    }

    Vector tokens = new Vector();
}
