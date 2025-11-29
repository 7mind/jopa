package com.sun.javadoc;

public interface Parameter {
    Type type();
    String name();
    String typeName();
    AnnotationDesc[] annotations();
}
