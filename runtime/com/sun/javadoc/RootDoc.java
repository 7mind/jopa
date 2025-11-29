package com.sun.javadoc;

public interface RootDoc extends Doc {
    String[][] options();
    PackageDoc[] specifiedPackages();
    ClassDoc[] specifiedClasses();
    ClassDoc[] classes();
    PackageDoc packageNamed(String name);
    ClassDoc classNamed(String qualifiedName);
    void printError(String msg);
    void printError(SourcePosition pos, String msg);
    void printWarning(String msg);
    void printWarning(SourcePosition pos, String msg);
    void printNotice(String msg);
    void printNotice(SourcePosition pos, String msg);
}
