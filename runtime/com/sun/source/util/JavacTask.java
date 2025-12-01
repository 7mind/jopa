package com.sun.source.util;

import javax.tools.JavaCompiler;
import java.io.IOException;
import javax.lang.model.util.Elements;
import javax.lang.model.util.Types;

public abstract class JavacTask implements JavaCompiler.CompilationTask {
    public abstract void setTaskListener(TaskListener taskListener);
    public abstract void addTaskListener(TaskListener taskListener);
    public abstract void removeTaskListener(TaskListener taskListener);
    public abstract Elements getElements();
    public abstract Types getTypes();

    public abstract Iterable<? extends com.sun.source.tree.CompilationUnitTree> parse() throws IOException;
    public abstract Iterable<? extends javax.lang.model.element.Element> analyze() throws IOException;
    public abstract Iterable<? extends javax.tools.JavaFileObject> generate() throws IOException;
}
