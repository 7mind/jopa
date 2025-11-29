package com.sun.javadoc;

public abstract class Doclet {
    public static boolean start(RootDoc root) { return true; }
    public static int optionLength(String option) { return 0; }
    public static boolean validOptions(String[][] options, DocErrorReporter reporter) { return true; }
    public static LanguageVersion languageVersion() { return LanguageVersion.JAVA_1_1; }
}
