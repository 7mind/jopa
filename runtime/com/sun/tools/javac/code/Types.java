package com.sun.tools.javac.code;

import com.sun.tools.javac.util.Context;
import com.sun.tools.javac.util.List;

public class Types {
    public static Types instance(Context context) { return new Types(); }

    public boolean isSubtype(Type t, Type s) { return false; }
    public boolean isSameType(Type t, Type s) { return false; }
    public boolean isCastable(Type t, Type s) { return false; }
    public boolean isConvertible(Type t, Type s) { return false; }
    public boolean isAssignable(Type t, Type s) { return false; }
    public Type erasure(Type t) { return t; }
    public Type capture(Type t) { return t; }
    public Symbol.ClassSymbol boxedClass(Type t) { return null; }
    public Type unboxedType(Type t) { return t; }
    public Type subst(Type t, List<Type> from, List<Type> to) { return t; }
}
