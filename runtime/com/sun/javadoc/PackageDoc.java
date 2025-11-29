package com.sun.javadoc;

public interface PackageDoc extends Doc {
    ClassDoc[] allClasses();
    ClassDoc[] allClasses(boolean filter);
    ClassDoc[] ordinaryClasses();
    ClassDoc[] exceptions();
    ClassDoc[] errors();
    ClassDoc[] enums();
    ClassDoc[] interfaces();
    ClassDoc[] annotationTypes();
    AnnotationTypeDoc[] annotationTypes(boolean filter);
    ClassDoc findClass(String className);
}
