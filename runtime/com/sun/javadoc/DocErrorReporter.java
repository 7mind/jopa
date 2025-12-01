package com.sun.javadoc;

public interface DocErrorReporter {
    void printError(String msg);
    void printError(SourcePosition pos, String msg);
    void printWarning(String msg);
    void printWarning(SourcePosition pos, String msg);
    void printNotice(String msg);
    void printNotice(SourcePosition pos, String msg);
}