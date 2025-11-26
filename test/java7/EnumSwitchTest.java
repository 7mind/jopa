// Test enum switch (Java 5+ feature, should already work)
public class EnumSwitchTest {
    enum Color { RED, GREEN, BLUE }

    static String colorToName(Color c) {
        switch (c) {
            case RED: return "red";
            case GREEN: return "green";
            case BLUE: return "blue";
            default: return "unknown";
        }
    }

    public static void main(String[] args) {
        boolean passed = true;

        if (!"red".equals(colorToName(Color.RED))) {
            System.out.println("FAIL: RED");
            passed = false;
        }
        if (!"green".equals(colorToName(Color.GREEN))) {
            System.out.println("FAIL: GREEN");
            passed = false;
        }
        if (!"blue".equals(colorToName(Color.BLUE))) {
            System.out.println("FAIL: BLUE");
            passed = false;
        }

        if (passed) {
            System.out.println("Enum switch test passed!");
        } else {
            System.exit(1);
        }
    }
}
