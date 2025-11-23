// Comprehensive autoboxing/unboxing test
public class AutoboxingTest {
    public static void main(String[] args) {
        int failures = 0;

        // Test 1: Basic boxing (primitive → wrapper)
        System.out.println("Test 1: Basic boxing...");
        Integer i = 42;
        if (i.intValue() != 42) {
            System.out.println("✗ FAIL: Boxing failed");
            failures++;
        } else {
            System.out.println("✓ Boxing works");
        }

        // Test 2: Basic unboxing (wrapper → primitive)
        System.out.println("Test 2: Basic unboxing...");
        Integer boxed = Integer.valueOf(100);
        int unboxed = boxed;
        if (unboxed != 100) {
            System.out.println("✗ FAIL: Unboxing failed");
            failures++;
        } else {
            System.out.println("✓ Unboxing works");
        }

        // Test 3: Multiple boxing/unboxing
        System.out.println("Test 3: Multiple conversions...");
        Integer a = 10;
        Integer b = 20;
        Integer c = 30;
        int sum = a.intValue() + b.intValue() + c.intValue();
        if (sum != 60) {
            System.out.println("✗ FAIL: Multiple conversions failed");
            failures++;
        } else {
            System.out.println("✓ Multiple conversions work");
        }

        // Print summary
        if (failures == 0) {
            System.out.println("✓ All autoboxing tests passed!");
            System.exit(0);
        } else {
            System.out.println("✗ " + failures + " test(s) failed");
            System.exit(1);
        }
    }

    static boolean testMethodBoxing(Integer value) {
        return value.intValue() == 55;
    }

    static boolean testMethodUnboxing(int value) {
        return value == 77;
    }
}
