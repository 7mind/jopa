package com.sun.tools.javac.api;

import javax.tools.JavaCompiler;

public abstract class JavacTask implements JavaCompiler.CompilationTask {
    public abstract Iterable<? extends com.sun.source.tree.CompilationUnitTree> parse() throws java.io.IOException;
    public abstract Iterable<? extends javax.lang.model.element.Element> analyze() throws java.io.IOException;
    public abstract Iterable<? extends javax.tools.JavaFileObject> generate() throws java.io.IOException;
    public abstract javax.lang.model.util.Types getTypes();
    public abstract javax.lang.model.util.Elements getElements();
}
