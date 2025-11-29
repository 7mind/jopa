package com.sun.tools.javac.main;

import com.sun.tools.javac.util.Context;

public class JavaCompiler {
    public static JavaCompiler instance(Context context) { return null; }
    public void compile(java.util.List<?> sourceFileObjects) {}
    public void close() {}
    public void close(boolean disposeNames) {}
    public int errorCount() { return 0; }
    public int warningCount() { return 0; }

    public static final Context.Key<JavaCompiler> compilerKey = new Context.Key<JavaCompiler>();
}
