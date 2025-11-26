// $Id: AstNumber.java,v 1.3 1999/11/04 14:02:18 shields Exp $
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://www.ibm.com/research/jikes.
// Copyright (C) 1983, 1999, International Business Machines Corporation
// and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.

class AstNumber extends Ast
{
    int token;

    public int Value() { return value; }

    public String toString(LexStream lex_stream)
    {
        return lex_stream.Name(token);
    }
}
