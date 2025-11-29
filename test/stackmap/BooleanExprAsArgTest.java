// Test for complex boolean expressions used directly as method arguments.
// This pattern is known to have StackMapTable verification issues with -target 1.7.
// See README: "test("name", a == b)" style calls.

public class BooleanExprAsArgTest {
    static int passed = 0;
    static int failed = 0;

    static void test(String name, boolean condition) {
        if (condition) {
            passed++;
            System.out.println("PASS: " + name);
        } else {
            failed++;
            System.out.println("FAIL: " + name);
        }
    }

    // Helper methods to prevent constant folding
    static int getInt(int x) { return x; }
    static boolean getBool(boolean b) { return b; }

    public static void main(String[] args) {
        System.out.println("=== Boolean Expression as Method Argument Test ===");
        System.out.println("Testing StackMapTable generation for complex boolean expressions");
        System.out.println("passed directly as method arguments.\n");

        int a = getInt(5);
        int b = getInt(5);
        int c = getInt(3);

        // Simple comparison as method argument
        test("simple equality a==b", a == b);
        test("simple inequality a!=c", a != c);
        test("simple less than c<a", c < a);
        test("simple greater than a>c", a > c);

        // Complex boolean expressions as method arguments (the problematic pattern)
        test("and expression (a==b && c<a)", a == b && c < a);
        test("or expression (a==c || b==5)", a == c || b == 5);
        test("complex (a==b && c<a) || c==3", (a == b && c < a) || c == 3);
        test("negated !(a != b)", !(a != b));
        test("double negation !!(a == b)", !!(a == b));

        // Nested complex expressions
        test("nested ((a==b) && (c<a || c==3))", (a == b) && (c < a || c == 3));
        test("triple and (a==5 && b==5 && c==3)", a == 5 && b == 5 && c == 3);
        test("triple or (a==1 || b==2 || c==3)", a == 1 || b == 2 || c == 3);

        // Boolean variable comparisons
        boolean x = getBool(true);
        boolean y = getBool(false);
        test("bool var equality x==true", x == true);
        test("bool var and (x && !y)", x && !y);
        test("mixed (a==b && x)", a == b && x);

        // Comparison chains
        test("chained (a >= c && c > 0)", a >= c && c > 0);
        test("range check (c > 0 && c < 10)", c > 0 && c < 10);

        System.out.println("\n=== RESULTS ===");
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);

        if (failed > 0) {
            System.exit(1);
        }
    }
}
