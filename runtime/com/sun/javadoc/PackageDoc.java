package com.sun.javadoc;

public interface PackageDoc extends Doc {
    ClassDoc[] allClasses();
    ClassDoc[] allClasses(boolean filter);
    ClassDoc[] ordinaryClasses();
    ClassDoc[] exceptions();
    ClassDoc[] errors();
    ClassDoc[] enums();
    ClassDoc[] interfaces();
    AnnotationTypeDoc[] annotationTypes();
    AnnotationDesc[] annotations();
    ClassDoc findClass(String className);
}