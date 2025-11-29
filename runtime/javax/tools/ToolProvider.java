package javax.tools;

public class ToolProvider {
    private ToolProvider() {}
    public static JavaCompiler getSystemJavaCompiler() { return null; }
    public static DocumentationTool getSystemDocumentationTool() { return null; }
}
