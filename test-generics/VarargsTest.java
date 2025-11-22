// Test varargs support

public class VarargsTest {
    // Simple varargs method
    public void printStrings(String... args) {
        for (int i = 0; i < args.length; i++) {
            // Process each argument
        }
    }

    // Varargs with regular parameters
    public void printf(String format, Object... args) {
        // Format string with varargs
    }

    // Test calling varargs methods
    public void testCalls() {
        // Call with zero arguments
        printStrings();

        // Call with one argument
        printStrings("hello");

        // Call with multiple arguments
        printStrings("foo", "bar", "baz");

        // Call with explicit array
        String[] arr = {"a", "b", "c"};
        printStrings(arr);

        // Mixed parameters
        printf("Value: %d", 42);
    }
}
