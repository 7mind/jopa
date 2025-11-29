package javax.tools;

import java.io.Writer;
import java.nio.charset.Charset;
import java.util.Locale;

public interface JavaCompiler extends Tool {
    CompilationTask getTask(Writer out, JavaFileManager fileManager,
        DiagnosticListener<? super JavaFileObject> diagnosticListener,
        Iterable<String> options, Iterable<String> classes,
        Iterable<? extends JavaFileObject> compilationUnits);
    StandardJavaFileManager getStandardFileManager(
        DiagnosticListener<? super JavaFileObject> diagnosticListener,
        Locale locale, Charset charset);

    interface CompilationTask {
        void setProcessors(Iterable<? extends javax.annotation.processing.Processor> processors);
        void setLocale(Locale locale);
        Boolean call();
    }
}
