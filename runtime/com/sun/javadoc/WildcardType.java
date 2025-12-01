package com.sun.javadoc;
public interface WildcardType extends Type {
    Type[] extendsBounds();
    Type[] superBounds();
}