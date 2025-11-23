// Test enum synthetic methods (values(), valueOf())
public class EnumMethodsTest {
    public static void main(String[] args) {
        int failures = 0;

        // Test values() method
        System.out.println("Testing Color.values()...");
        try {
            Color[] values = Color.values();

            if (values == null) {
                System.out.println("✗ FAIL: values() returned null");
                failures++;
            } else if (values.length != 3) {
                System.out.println("✗ FAIL: values() returned array of length " + values.length + ", expected 3");
                failures++;
            } else {
                boolean hasRed = false;
                boolean hasGreen = false;
                boolean hasBlue = false;

                for (int i = 0; i < values.length; i++) {
                    Color c = values[i];
                    if (c == Color.RED) hasRed = true;
                    if (c == Color.GREEN) hasGreen = true;
                    if (c == Color.BLUE) hasBlue = true;
                }

                if (hasRed && hasGreen && hasBlue) {
                    System.out.println("✓ values() method works");
                } else {
                    System.out.println("✗ FAIL: values() missing some constants");
                    failures++;
                }
            }
        } catch (Exception e) {
            System.out.println("✗ FAIL: values() threw exception: " + e);
            failures++;
        }

        // Test valueOf(String) method
        System.out.println("Testing Color.valueOf(String)...");
        try {
            Color red = Color.valueOf("RED");
            if (red == Color.RED) {
                System.out.println("✓ valueOf(\"RED\") works");
            } else {
                System.out.println("✗ FAIL: valueOf(\"RED\") returned wrong constant");
                failures++;
            }
        } catch (Exception e) {
            System.out.println("✗ FAIL: valueOf(\"RED\") threw exception: " + e);
            failures++;
        }

        // Print summary
        if (failures == 0) {
            System.out.println("✓ All enum method tests passed!");
            System.exit(0);
        } else {
            System.out.println("✗ " + failures + " test(s) failed");
            System.exit(1);
        }
    }
}
