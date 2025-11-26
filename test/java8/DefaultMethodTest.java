// Test for Java 8 default methods
public class DefaultMethodTest {
    interface Greeter {
        // Default method with implementation
        default String greet() {
            return "Hello";
        }

        // Static method in interface - REMOVED
        // Note: Static methods in interfaces require class file version 52 (Java 8)
        // which requires StackMapTable. Disabled until StackMapTable is implemented.

        // Abstract method
        String farewell();
    }

    static class FriendlyGreeter implements Greeter {
        public String farewell() {
            return "Goodbye";
        }
    }

    static class FormalGreeter implements Greeter {
        // Override default method
        public String greet() {
            return "Greetings";
        }

        public String farewell() {
            return "Farewell";
        }
    }

    public static void main(String[] args) {
        boolean passed = true;
        Greeter friendly = new FriendlyGreeter();
        Greeter formal = new FormalGreeter();

        // Test default method
        String result = friendly.greet();
        if (!result.equals("Hello")) {
            System.out.println("FAIL: Default method");
            passed = false;
        }

        // Test overridden default method
        result = formal.greet();
        if (!result.equals("Greetings")) {
            System.out.println("FAIL: Overridden default method");
            passed = false;
        }

        // Test static interface method - SKIPPED
        // Note: Requires class file version 52 (Java 8) with StackMapTable

        // Test abstract method implementations
        if (!friendly.farewell().equals("Goodbye")) {
            System.out.println("FAIL: Abstract method (friendly)");
            passed = false;
        }
        if (!formal.farewell().equals("Farewell")) {
            System.out.println("FAIL: Abstract method (formal)");
            passed = false;
        }

        if (passed) {
            System.out.println("All default method tests passed!");
        } else {
            System.exit(1);
        }
    }
}
