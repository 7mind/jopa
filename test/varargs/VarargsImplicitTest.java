// Test varargs with implicit array creation

public class VarargsImplicitTest {
    private static int callCount = 0;
    private static int lastArgCount = -1;

    public static void printStrings(String... args) {
        callCount++;
        lastArgCount = args.length;
    }

    public static void main(String[] args) {
        // Test multiple arguments
        printStrings("a", "b", "c");
        if (callCount != 1 || lastArgCount != 3) {
            System.out.println("FAIL: multiple varargs");
            System.exit(1);
        }

        // Test single argument
        printStrings("single");
        if (callCount != 2 || lastArgCount != 1) {
            System.out.println("FAIL: single vararg");
            System.exit(1);
        }

        // Test empty varargs
        printStrings();
        if (callCount != 3 || lastArgCount != 0) {
            System.out.println("FAIL: empty varargs");
            System.exit(1);
        }

        System.out.println("PASS: VarargsImplicitTest passed");
    }
}
