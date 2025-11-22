// Test varargs with runtime execution
public class VarargsTest {
    private static int callCount = 0;
    private static int totalArgs = 0;

    public static void testMethod(String... args) {
        callCount++;
        totalArgs += args.length;
    }

    public static void testWithFixed(int fixed, String... args) {
        if (fixed == 42 && args.length == 2) {
            System.out.println("✓ Fixed + varargs works");
        } else {
            System.out.println("✗ FAIL: Fixed + varargs");
            System.exit(1);
        }
    }

    public static void main(String[] args) {
        // Test empty varargs
        testMethod();
        if (callCount == 1 && totalArgs == 0) {
            System.out.println("✓ Empty varargs works");
        } else {
            System.out.println("✗ FAIL: Empty varargs");
            System.exit(1);
        }

        // Test single argument
        testMethod("hello");
        if (callCount == 2 && totalArgs == 1) {
            System.out.println("✓ Single vararg works");
        } else {
            System.out.println("✗ FAIL: Single vararg");
            System.exit(1);
        }

        // Test multiple arguments
        testMethod("a", "b", "c");
        if (callCount == 3 && totalArgs == 4) {
            System.out.println("✓ Multiple varargs work");
        } else {
            System.out.println("✗ FAIL: Multiple varargs");
            System.exit(1);
        }

        // Test fixed + varargs
        testWithFixed(42, "x", "y");

        System.out.println("✓ All varargs tests passed!");
    }
}
