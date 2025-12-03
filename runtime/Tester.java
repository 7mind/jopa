import java.io.*;
import java.util.*;
import com.sun.javadoc.*;

public class Tester {
    protected final String TEST_SRC = System.getProperty("test.src", ".");
    protected final String TEST_CLASSES = System.getProperty("test.classes", ".");

    private String docletName;
    private String[] args;
    private Writer out = null;

    public static abstract class Doclet extends com.sun.javadoc.Doclet {
        public static LanguageVersion languageVersion() {
            return LanguageVersion.JAVA_1_5;
        }
    }

    public Tester(String docletName) {
        this(docletName, new String[0]);
    }

    public Tester(String docletName, String... additionalArgs) {
        this.docletName = docletName;
        this.args = additionalArgs;
        this.out = new StringWriter();
    }

    public void run() throws IOException {}
    public void verify() throws IOException {}

    public void println(Object o) throws IOException {}
    public void println() throws IOException {}
    public void printPackage(PackageDoc p) throws IOException {}
    public void printClass(ClassDoc cd) throws IOException {}
    public void printAnnotationType(AnnotationTypeDoc at) throws IOException {}
    public void printField(FieldDoc f) throws IOException {}
    public void printParameter(Parameter p) throws IOException {}
    public void printMethod(MethodDoc m) throws IOException {}
    public void printAnnotationTypeElement(AnnotationTypeElementDoc e) throws IOException {}
    public void printConstructor(ConstructorDoc c) throws IOException {}
}
