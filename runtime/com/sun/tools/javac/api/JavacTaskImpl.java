package com.sun.tools.javac.api;

import javax.tools.*;
import javax.annotation.processing.Processor;
import java.util.Locale;

public class JavacTaskImpl extends JavacTask {
    public Boolean call() { return true; }
    public void setProcessors(Iterable<? extends Processor> processors) {}
    public void setLocale(Locale locale) {}
    public Iterable<? extends javax.lang.model.element.Element> analyze() { return null; }
    public Iterable<? extends JavaFileObject> generate() { return null; }
    public Iterable<? extends javax.lang.model.element.TypeElement> enter() { return null; }
    public Iterable<? extends com.sun.source.tree.CompilationUnitTree> parse() { return null; }
    public javax.lang.model.util.Types getTypes() { return null; }
    public javax.lang.model.util.Elements getElements() { return null; }
}
