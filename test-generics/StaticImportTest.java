// Test static imports
import static util.MathUtils.PI;
import static util.MathUtils.MAX_VALUE;
import static util.MathUtils.max;

public class StaticImportTest {
    public static void main(String[] args) {
        int failures = 0;

        // Test 1: Static field import
        System.out.println("Test 1: Static field import...");
        if (PI != 3.14159) {
            System.out.println("✗ FAIL: PI value incorrect");
            failures++;
        } else {
            System.out.println("✓ Static field import works");
        }

        // Test 2: Another static field import
        System.out.println("Test 2: Another static field import...");
        if (MAX_VALUE != 100) {
            System.out.println("✗ FAIL: MAX_VALUE incorrect");
            failures++;
        } else {
            System.out.println("✓ Second static field import works");
        }

        // Test 3: Static method import
        System.out.println("Test 3: Static method import...");
        int result = max(10, 20);
        if (result != 20) {
            System.out.println("✗ FAIL: max(10, 20) returned " + result);
            failures++;
        } else {
            System.out.println("✓ Static method import works");
        }

        // Print summary
        if (failures == 0) {
            System.out.println("✓ All static import tests passed!");
            System.exit(0);
        } else {
            System.out.println("✗ " + failures + " test(s) failed");
            System.exit(1);
        }
    }
}
