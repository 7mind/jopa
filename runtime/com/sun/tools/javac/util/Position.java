package com.sun.tools.javac.util;

public class Position {
    public static final int NOPOS = -1;
    public static final int MAXPOS = Integer.MAX_VALUE;
    public static final int LINESHIFT = 10;
    public static final int MAXCOLUMN = (1 << LINESHIFT) - 1;
    public static final int MAXLINE = (1 << (Integer.SIZE - LINESHIFT - 1)) - 1;

    public static int encodePosition(int line, int col) {
        return 0;
    }
}
