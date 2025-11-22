// Test enum with runtime execution
public class ColorTest {
    public static void main(String[] args) {
        // Test enum constant access
        Color red = Color.RED;
        Color green = Color.GREEN;
        Color blue = Color.BLUE;

        // Test enum comparisons
        if (red == Color.RED) {
            System.out.println("✓ Enum constant equality works");
        } else {
            System.out.println("✗ FAIL: Enum constant equality");
            System.exit(1);
        }

        // Test enum is not null
        if (red != null && green != null && blue != null) {
            System.out.println("✓ Enum constants are not null");
        } else {
            System.out.println("✗ FAIL: Enum constant is null");
            System.exit(1);
        }

        // Test different enum constants are different
        if (red != green && green != blue && red != blue) {
            System.out.println("✓ Enum constants are distinct");
        } else {
            System.out.println("✗ FAIL: Enum constants not distinct");
            System.exit(1);
        }

        System.out.println("✓ All enum tests passed!");
    }
}
