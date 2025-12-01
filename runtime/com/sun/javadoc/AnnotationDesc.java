package com.sun.javadoc;
public interface AnnotationDesc {
    AnnotationTypeDoc annotationType();
    ElementValuePair[] elementValues();
    boolean isSynthesized();
    public interface ElementValuePair {
        AnnotationTypeElementDoc element();
        AnnotationValue value();
    }
}