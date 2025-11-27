// $Id: Main.java,v 1.3 1999/11/04 14:02:18 shields Exp $
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://www.ibm.com/research/jikes.
// Copyright (C) 1983, 1999, International Business Machines Corporation
// and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.

public class Main
{
    public static void main(String[] args) throws java.io.IOException, java.io.FileNotFoundException
    {
        Option option = new Option(args);

        while (true)
        {
            System.out.print("Expression: ");

            LexStream lex_stream = new LexStream();
            Scanner scanner = new Scanner(option, lex_stream);
            scanner.scan();

            if (option.dump)
                lex_stream.dump();

            Parser parser = new Parser(lex_stream);
            Ast root = parser.parse();
            if (root != null)
                 System.out.println("    " + root.toString(lex_stream) + " = " + root.Value());
            else break;
        }

        return;
    }
}
