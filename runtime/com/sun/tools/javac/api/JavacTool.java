package com.sun.tools.javac.api;

import javax.tools.*;
import java.io.Writer;
import java.nio.charset.Charset;
import java.util.Locale;
import java.util.Set;
import javax.lang.model.SourceVersion;
import java.io.InputStream;
import java.io.OutputStream;

public class JavacTool implements JavaCompiler {
    public static JavacTool create() { return new JavacTool(); }

    public CompilationTask getTask(Writer out, JavaFileManager fileManager,
        DiagnosticListener<? super JavaFileObject> diagnosticListener,
        Iterable<String> options, Iterable<String> classes,
        Iterable<? extends JavaFileObject> compilationUnits) {
        return new JavacTaskImpl();
    }

    public StandardJavaFileManager getStandardFileManager(
        DiagnosticListener<? super JavaFileObject> diagnosticListener,
        Locale locale, Charset charset) {
        return null;
    }

    public int run(InputStream in, OutputStream out, OutputStream err, String... arguments) { return 0; }
    public Set<SourceVersion> getSourceVersions() { return null; }
    public int isSupportedOption(String option) { return 0; }
}
