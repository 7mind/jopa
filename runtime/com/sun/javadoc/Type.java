package com.sun.javadoc;

public interface Type {
    String typeName();
    String qualifiedTypeName();
    String simpleTypeName();
    String dimension();
    boolean isPrimitive();
    ClassDoc asClassDoc();
    TypeVariable asTypeVariable();
    WildcardType asWildcardType();
    ParameterizedType asParameterizedType();
    AnnotationTypeDoc asAnnotationTypeDoc();
}
