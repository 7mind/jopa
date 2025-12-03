package com.sun.tools.javac.util;

public class Names {
    public static Names instance(Context context) { return new Names(); }

    public Name fromString(String s) { return new Name(s); }
}
