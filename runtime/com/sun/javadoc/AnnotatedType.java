package com.sun.javadoc;
public interface AnnotatedType extends Type {
    AnnotationDesc[] annotations();
    Type underlyingType();
}
