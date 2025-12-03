package com.sun.tools.javac.code;

import com.sun.tools.javac.util.Context;

public class Symtab {
    public static Symtab instance(Context context) { return new Symtab(); }

    public Symbol noSymbol = new Symbol();
    public Symbol.ClassSymbol arrayClass = null;
    public Symbol.TypeSymbol boundClass = null;

    public Type objectType = new Type();
    public Type byteType = new Type();
    public Type shortType = new Type();
    public Type intType = new Type();
    public Type longType = new Type();
    public Type floatType = new Type();
    public Type doubleType = new Type();
    public Type charType = new Type();
    public Type booleanType = new Type();
    public Type voidType = new Type();
    public Type stringType = new Type();
    public Type classType = new Type();
    public Type throwableType = new Type();
    public Type exceptionType = new Type();
    public Type errorType = new Type();
    public Type botType = new Type();
}
