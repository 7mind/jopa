import com.sun.javadoc.*;
import java.util.*;
import java.io.*;

public abstract class JavadocTester {
    protected static final String FS = System.getProperty("file.separator");
    protected static final String PS = System.getProperty("path.separator");
    protected static final String NL = System.getProperty("line.separator");
    protected static final String SRC_DIR = System.getProperty("test.src", ".");
    protected static final String JAVA_VERSION = System.getProperty("java.version");
    protected static final String[][] NO_TEST = new String[][] {};

    public static final String ERROR_OUTPUT = "ERROR_OUTPUT";
    public static final String NOTICE_OUTPUT = "NOTICE_OUTPUT";
    public static final String WARNING_OUTPUT = "WARNING_OUTPUT";
    public static final String STANDARD_OUTPUT = "STANDARD_OUTPUT";

    public static final String DEFAULT_DOCLET_CLASS = "com.sun.tools.doclets.formats.html.HtmlDoclet";
    public static final String DEFAULT_DOCLET_CLASS_OLD = "com.sun.tools.doclets.standard.Standard";

    public StringWriter errors;
    public StringWriter notices;
    public StringWriter warnings;
    public StringBuffer standardOut;

    protected boolean exactNewlineMatch = true;

    public JavadocTester() {}

    public abstract String getBugId();
    public abstract String getBugName();

    public static int run(JavadocTester tester, String[] args,
            String[][] testArray, String[][] negatedTestArray) {
        int returnCode = tester.runJavadoc(args);
        tester.runTestsOnHTML(testArray, negatedTestArray);
        return returnCode;
    }

    public int runJavadoc(String[] args) {
        return runJavadoc(DEFAULT_DOCLET_CLASS, args);
    }

    public int runJavadoc(String docletClass, String[] args) {
        errors = new StringWriter();
        notices = new StringWriter();
        warnings = new StringWriter();
        standardOut = new StringBuffer();
        return com.sun.tools.javadoc.Main.execute(
                getBugName(),
                new PrintWriter(errors, true),
                new PrintWriter(warnings, true),
                new PrintWriter(notices, true),
                docletClass,
                getClass().getClassLoader(),
                args);
    }

    public void runTestsOnHTML(String[][] testArray, String[][] negatedTestArray) {}

    public void runDiffs(String[][] filePairs) throws Error {}
    public void runDiffs(String[][] filePairs, boolean throwErrorIfNoMatch) throws Error {}

    public void checkExitCode(int expectedExitCode, int actualExitCode) {}

    protected void printSummary() {}

    public String readFileToString(String fileName) throws Error {
        return "";
    }

    public boolean diff(String file1, String file2, boolean throwErrorIFNoMatch) throws Error {
        return true;
    }

    public String getStandardOutput() { return standardOut != null ? standardOut.toString() : ""; }
    public String getErrorOutput() { return errors != null ? errors.getBuffer().toString() : ""; }
    public String getNoticeOutput() { return notices != null ? notices.getBuffer().toString() : ""; }
    public String getWarningOutput() { return warnings != null ? warnings.getBuffer().toString() : ""; }

    public static void copyDir(String targetDir, String destDir) {}
    public static void copyFile(File destfile, File srcfile) throws IOException {}
}
