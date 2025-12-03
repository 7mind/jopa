package com.sun.tools.javac.comp;

import com.sun.tools.javac.code.Type;
import com.sun.tools.javac.util.Context;

public class Check {
    public static Check instance(Context context) { return new Check(); }

    public boolean checkValidGenericType(Type t) { return true; }
}
