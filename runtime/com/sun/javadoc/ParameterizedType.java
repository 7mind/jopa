package com.sun.javadoc;

public interface ParameterizedType extends Type {
    ClassDoc asClassDoc();
    Type[] typeArguments();
    Type superclassType();
    Type[] interfaceTypes();
    Type containingType();
}
