package com.sun.tools.javac.code;

import com.sun.tools.javac.util.List;

public class Type {
    public static final Type noType = new Type();
    public Symbol.TypeSymbol tsym;

    public Type() {}

    public boolean isPrimitive() { return false; }
    public Type constType(Object value) { return this; }
    public List<Type> getTypeArguments() { return List.nil(); }

    public static class ClassType extends Type {
        public Type supertype_field;
        public List<Type> interfaces_field;
        private List<Type> typarams_field;

        public ClassType(Type outer, List<Type> typarams, Symbol.TypeSymbol tsym) {
            this.tsym = tsym;
            this.typarams_field = typarams;
        }

        public List<Type> getTypeArguments() { return typarams_field != null ? typarams_field : List.<Type>nil(); }
    }

    public static class ArrayType extends Type {
        public ArrayType(Type elemtype, Symbol.TypeSymbol arrayClass) {}
    }

    public static class TypeVar extends Type {
        public Type bound;

        public TypeVar(Symbol.TypeSymbol tsym, Type bound, Type lower) {
            this.tsym = tsym;
            this.bound = bound;
        }
    }

    public static class WildcardType extends Type {
        public WildcardType(Type type, BoundKind kind, Symbol.TypeSymbol bound) {}
    }

    public static class CapturedType extends Type {
        public CapturedType(com.sun.tools.javac.util.Name name, Symbol owner, Type upper, Type lower, WildcardType wildcard) {}
    }
}
