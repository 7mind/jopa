// Simple test for Java 7 try-finally without resources
public class SimpleTryFinallyTest {
    static int count = 0;

    public static void main(String[] args) {
        // Test 1: Basic try-finally, normal completion
        count = 0;
        try {
            count = 1;
        } finally {
            count = count + 10;
        }
        if (count != 11) {
            System.out.println("FAIL: Test 1, expected 11, got " + count);
            System.exit(1);
        }

        // Test 2: Try-finally with exception
        count = 0;
        try {
            try {
                count = 1;
                throw new RuntimeException("test");
            } finally {
                count = count + 10;
            }
        } catch (RuntimeException e) {
            // Expected
        }
        if (count != 11) {
            System.out.println("FAIL: Test 2, expected 11, got " + count);
            System.exit(1);
        }

        // Test 3: Nested try-finally
        count = 0;
        try {
            count = 1;
            try {
                count = count + 2;
            } finally {
                count = count + 10;
            }
        } finally {
            count = count + 100;
        }
        if (count != 113) {
            System.out.println("FAIL: Test 3, expected 113, got " + count);
            System.exit(1);
        }

        System.out.println("SimpleTryFinallyTest passed!");
    }
}
