package com.sun.javadoc;
public interface SerialFieldTag extends Tag {
    String fieldName();
    String fieldType();
    ClassDoc fieldTypeDoc();
    String description();
    int compareTo(Object obj);
}