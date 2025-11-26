// $Id: Option.java,v 1.3 1999/11/04 14:02:18 shields Exp $
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://www.ibm.com/research/jikes.
// Copyright (C) 1983, 1999, International Business Machines Corporation
// and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.

class Option
{
    String filename;
    boolean non_validating = true,
            dump = false;

    Option(String [] args)
    {
        for (int i = 0; i < args.length; i++)
        {
            if (args[i].charAt(0) == '-')
            {
                if (args[i].equals("-d"))
                    dump = true;
            }
            else
            {
                filename = args[i];
                break;
            }
        }

        return;
    }
}
