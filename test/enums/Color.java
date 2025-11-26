// Test enum support with proper filename
public enum Color {
    RED,
    GREEN,
    BLUE;

    public static void main(String[] args) {
        // Test enum values
        Color[] colors = Color.values();
        if (colors.length != 3) {
            System.out.println("FAIL: expected 3 colors");
            System.exit(1);
        }

        // Test valueOf
        Color red = Color.valueOf("RED");
        if (red != Color.RED) {
            System.out.println("FAIL: valueOf failed");
            System.exit(1);
        }

        // Test ordinal
        if (Color.RED.ordinal() != 0 || Color.GREEN.ordinal() != 1 || Color.BLUE.ordinal() != 2) {
            System.out.println("FAIL: ordinal values wrong");
            System.exit(1);
        }

        System.out.println("PASS: Color enum tests passed");
    }
}
