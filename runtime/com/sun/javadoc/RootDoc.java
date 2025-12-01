package com.sun.javadoc;

public interface RootDoc extends Doc, DocErrorReporter {
    ClassDoc[] classes();
        ClassDoc[] specifiedClasses();
        PackageDoc[] specifiedPackages();
        String[][] options();
        ClassDoc classNamed(String qualifiedName);
    }
    