package com.sun.javadoc;
public interface ParameterizedType extends Type {
    Type[] typeArguments();
    Type superclassType();
    Type[] interfaceTypes();
    Type containingType();
}