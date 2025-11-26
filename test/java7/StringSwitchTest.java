// Test for Java 7 strings in switch
public class StringSwitchTest {
    static int stringToInt(String s) {
        switch (s) {
            case "one":
                return 1;
            case "two":
                return 2;
            case "three":
                return 3;
            default:
                return -1;
        }
    }

    static String stringWithFallthrough(String s) {
        String result = "";
        switch (s) {
            case "a":
            case "b":
                result = "ab";
                break;
            case "c":
                result = "c";
                break;
            default:
                result = "other";
        }
        return result;
    }

    public static void main(String[] args) {
        boolean passed = true;

        // Test basic string switch
        if (stringToInt("one") != 1) {
            System.out.println("FAIL: stringToInt(\"one\")");
            passed = false;
        }
        if (stringToInt("two") != 2) {
            System.out.println("FAIL: stringToInt(\"two\")");
            passed = false;
        }
        if (stringToInt("three") != 3) {
            System.out.println("FAIL: stringToInt(\"three\")");
            passed = false;
        }
        if (stringToInt("unknown") != -1) {
            System.out.println("FAIL: stringToInt(\"unknown\")");
            passed = false;
        }

        // Test fallthrough
        if (!"ab".equals(stringWithFallthrough("a"))) {
            System.out.println("FAIL: stringWithFallthrough(\"a\")");
            passed = false;
        }
        if (!"ab".equals(stringWithFallthrough("b"))) {
            System.out.println("FAIL: stringWithFallthrough(\"b\")");
            passed = false;
        }
        if (!"c".equals(stringWithFallthrough("c"))) {
            System.out.println("FAIL: stringWithFallthrough(\"c\")");
            passed = false;
        }
        if (!"other".equals(stringWithFallthrough("x"))) {
            System.out.println("FAIL: stringWithFallthrough(\"x\")");
            passed = false;
        }

        if (passed) {
            System.out.println("String switch test passed!");
        } else {
            System.exit(1);
        }
    }
}
