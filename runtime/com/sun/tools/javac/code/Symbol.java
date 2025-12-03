package com.sun.tools.javac.code;

import com.sun.tools.javac.util.Name;

public class Symbol {
    public Type type;
    public Name name;

    public void complete() {}

    public static class TypeSymbol extends Symbol {
        public TypeSymbol(long flags, Name name, Type type, Symbol owner) {
            this.name = name;
        }
    }

    public static class ClassSymbol extends TypeSymbol {
        public ClassSymbol(long flags, Name name, Symbol owner) {
            super(flags, name, null, owner);
        }
    }
}
