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

        // Test 4: Exception suppression - try body throws, close also throws
        // The close exception should be suppressed and added to the primary exception
        try {
            try (ThrowingResource res4 = new ThrowingResource()) {
                throw new RuntimeException("Primary exception");
            }
        } catch (RuntimeException e) {
            if (!e.getMessage().equals("Primary exception")) {
                System.out.println("FAIL: Wrong primary exception: " + e.getMessage());
                passed = false;
            } else {
                Throwable[] suppressed = e.getSuppressed();
                if (suppressed.length != 1) {
                    System.out.println("FAIL: Expected 1 suppressed exception, got " + suppressed.length);
                    passed = false;
                } else if (!suppressed[0].getMessage().equals("Close exception")) {
                    System.out.println("FAIL: Wrong suppressed exception: " + suppressed[0].getMessage());
                    passed = false;
                }
            }
        } catch (Exception e) {
            System.out.println("FAIL: Unexpected exception type: " + e.getClass().getName());
            passed = false;
        }

        // Test 5: Multiple resources, both close() throw
        // All close exceptions should be suppressed on the primary
        try {
            try (ThrowingResource res5a = new ThrowingResource();
                 ThrowingResource res5b = new ThrowingResource()) {
                throw new RuntimeException("Primary from multi");
            }
        } catch (RuntimeException e) {
            if (!e.getMessage().equals("Primary from multi")) {
                System.out.println("FAIL: Wrong primary exception in multi test: " + e.getMessage());
                passed = false;
            } else {
                Throwable[] suppressed = e.getSuppressed();
                if (suppressed.length != 2) {
                    System.out.println("FAIL: Expected 2 suppressed exceptions, got " + suppressed.length);
                    passed = false;
                }
            }
        } catch (Exception e) {
            System.out.println("FAIL: Unexpected exception type in multi test: " + e.getClass().getName());
            passed = false;
        }

        // Test 6: Normal path close exception (no try body exception)
        // close() throws, should propagate as main exception
        try {
            try (ThrowingResource res6 = new ThrowingResource()) {
                // Normal completion, but close() will throw
            }
            System.out.println("FAIL: Expected exception from close()");
            passed = false;
        } catch (Exception e) {
            if (!e.getMessage().equals("Close exception")) {
                System.out.println("FAIL: Wrong exception from normal path close: " + e.getMessage());
                passed = false;
            }
        }

        // Test 7: Multiple close() throw in normal path
        // First close exception becomes main, second gets suppressed
        try {
            try (ThrowingResource res7a = new ThrowingResource();
                 ThrowingResource res7b = new ThrowingResource()) {
                // Normal completion
            }
            System.out.println("FAIL: Expected exception from close() in multi normal");
            passed = false;
        } catch (Exception e) {
            if (!e.getMessage().equals("Close exception")) {
                System.out.println("FAIL: Wrong exception in multi normal: " + e.getMessage());
                passed = false;
            }
            Throwable[] suppressed = e.getSuppressed();
            if (suppressed.length != 1) {
                System.out.println("FAIL: Expected 1 suppressed in multi normal, got " + suppressed.length);
                passed = false;
            }
        }

        if (passed) {
            System.out.println("Try-with-resources test passed!");
        } else {
            System.exit(1);
        }
    }
}
