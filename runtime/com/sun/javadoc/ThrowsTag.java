package com.sun.javadoc;
public interface ThrowsTag extends Tag {
    String exceptionName();
    String exceptionComment();
    ClassDoc exception();
    Type exceptionType();
}