package com.sun.javadoc;
public interface TypeVariable extends Type {
    Type[] bounds();
    ProgramElementDoc owner();
    AnnotationDesc[] annotations();
}