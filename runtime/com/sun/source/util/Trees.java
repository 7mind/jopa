package com.sun.source.util;

import javax.annotation.processing.ProcessingEnvironment;

public abstract class Trees {
    public static Trees instance(ProcessingEnvironment env) { return null; }
    public static Trees instance(javax.tools.JavaCompiler.CompilationTask task) { return null; }

    public abstract com.sun.source.util.TreePath getPath(javax.lang.model.element.Element e, javax.lang.model.element.AnnotationMirror a);
    public abstract com.sun.source.tree.Tree getTree(javax.lang.model.element.Element e, javax.lang.model.element.AnnotationMirror a);
}
