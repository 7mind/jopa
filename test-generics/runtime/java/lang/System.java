package java.lang;

// Stub System class for testing
public final class System {
    // Standard streams
    public static final java.io.PrintStream out = null;
    public static final java.io.PrintStream err = null;

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

    public static void arraycopy(Object src, int srcPos, Object dest, int destPos, int length) {
        // Native implementation
    }
}
