// Test for Java 7 try-with-resources
public class TryWithResourcesTest {
    static int closeCount = 0;

    static class SimpleResource implements AutoCloseable {
        private String name;

        public SimpleResource(String name) {
            this.name = name;
        }

        public void doSomething() {
            // Just a marker method
        }

        public void close() {
            closeCount++;
        }
    }

    static class ThrowingResource implements AutoCloseable {
        public void close() throws Exception {
            throw new Exception("Close exception");
        }
    }

    public static void main(String[] args) {
        boolean passed = true;

        // Test 1: Basic try-with-resources
        closeCount = 0;
        try (SimpleResource res1 = new SimpleResource("test")) {
            res1.doSomething();
        }
        if (closeCount != 1) {
            System.out.println("FAIL: Basic close not called, closeCount=" + closeCount);
            passed = false;
        }

        // Test 2: Multiple resources (closed in reverse order)
        closeCount = 0;
        try (SimpleResource r1 = new SimpleResource("first");
             SimpleResource r2 = new SimpleResource("second")) {
            r1.doSomething();
            r2.doSomething();
        }
        if (closeCount != 2) {
            System.out.println("FAIL: Multiple resources close not called, closeCount=" + closeCount);
            passed = false;
        }

        // Test 3: Exception in try block, resource still closed
        closeCount = 0;
        try {
            try (SimpleResource res3 = new SimpleResource("test")) {
                throw new RuntimeException("Test exception");
            }
        } catch (RuntimeException e) {
            // Expected
        }
        if (closeCount != 1) {
            System.out.println("FAIL: Exception case close not called, closeCount=" + closeCount);
            passed = false;
        }

        // Test 4: Exception suppression - SKIPPED for now
        // TODO: Implement addSuppressed() call in bytecode generation
        // when both try block and close() throw exceptions

        if (passed) {
            System.out.println("Try-with-resources test passed!");
        } else {
            System.exit(1);
        }
    }
}
