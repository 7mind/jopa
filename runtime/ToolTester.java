import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Arrays;
import javax.tools.*;

import static javax.tools.StandardLocation.CLASS_PATH;
import static javax.tools.StandardLocation.SOURCE_PATH;
import static javax.tools.StandardLocation.CLASS_OUTPUT;

public class ToolTester {
    public final File test_src = new File(System.getProperty("test.src", "."));
    public final File test_classes = new File(System.getProperty("test.classes", "."));
    public final JavaCompiler tool = ToolProvider.getSystemJavaCompiler();
    public final StandardJavaFileManager fm = getFileManager(tool, null, null);
    public JavaCompiler.CompilationTask task = null;

    public final StandardJavaFileManager getFileManager(JavaCompiler tool,
                                                        DiagnosticListener<JavaFileObject> dl,
                                                        Charset encoding) {
        StandardJavaFileManager fm = tool.getStandardFileManager(dl, null, encoding);
        try {
            fm.setLocation(SOURCE_PATH, Arrays.asList(test_src));
            fm.setLocation(CLASS_PATH, Arrays.asList(test_classes));
            fm.setLocation(CLASS_OUTPUT, Arrays.asList(test_classes));
        } catch (IOException e) {
            throw new AssertionError(e);
        }
        return fm;
    }
}
