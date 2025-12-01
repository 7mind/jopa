package java.lang;

// Stub System class for testing
public final class System {
    // Standard streams - not final so they can be redirected
    public static java.io.PrintStream out = null;
    public static java.io.PrintStream err = null;
    public static java.io.InputStream in = null;

    public static void setOut(java.io.PrintStream out) {}
    public static void setErr(java.io.PrintStream err) {}
    public static void setIn(java.io.InputStream in) {}

    // Prevent instantiation
    private System() {}

    // Exit method
    public static void exit(int status) {
        // Native implementation
    }

    // Utility methods
    public static long currentTimeMillis() {
        return 0L;
    }

    public static native void arraycopy(Object src, int srcPos, Object dest, int destPos, int length);
    public static native void gc();
    public static native long nanoTime();


    public static String getProperty(String key) {
        return null;
    }

    public static String getProperty(String key, String def) {
        return def;
    }

    public static String setProperty(String key, String value) {
        return null;
    }

    public static String getenv(String name) {
        return null;
    }

    public static int identityHashCode(Object x) {
        return 0;
    }

    public static String lineSeparator() {
        return "\n";
    }

    public static SecurityManager getSecurityManager() {
        return null;
    }
}
