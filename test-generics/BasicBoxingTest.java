// Test basic boxing: primitive → wrapper
public class BasicBoxingTest {
    public static void main(String[] args) {
        int failures = 0;

        // Test Integer boxing
        System.out.println("Testing Integer boxing...");
        try {
            Integer i = 42;  // Should box int → Integer
            if (i == null) {
                System.out.println("✗ FAIL: Boxing produced null");
                failures++;
            } else if (i.intValue() != 42) {
                System.out.println("✗ FAIL: Boxing produced wrong value: " + i.intValue());
                failures++;
            } else {
                System.out.println("✓ Integer boxing works");
            }
        } catch (Exception e) {
            System.out.println("✗ FAIL: Boxing threw exception: " + e);
            failures++;
        }

        // Print summary
        if (failures == 0) {
            System.out.println("✓ All boxing tests passed!");
            System.exit(0);
        } else {
            System.out.println("✗ " + failures + " test(s) failed");
            System.exit(1);
        }
    }
}
