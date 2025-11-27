// StackMapTable verification test - exercises control flow patterns that require
// proper StackMapTable generation for Java 7+ bytecode (class version 51.0+).
//
// These patterns MUST pass with strict JVM verification (no -noverify).
// Target 1.7 generates StackMapTable attributes; targets 1.5/1.6 don't need them.

public class StackMapTableTest {
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

    // === Basic if/else branches ===

    static int simpleIfElse(int x) {
        if (x > 0) {
            return 1;
        } else {
            return -1;
        }
    }

    static int nestedIfElse(int x, int y) {
        if (x > 0) {
            if (y > 0) {
                return 1;
            } else {
                return 2;
            }
        } else {
            if (y > 0) {
                return 3;
            } else {
                return 4;
            }
        }
    }

    static int ifElseChain(int x) {
        if (x == 1) {
            return 10;
        } else if (x == 2) {
            return 20;
        } else if (x == 3) {
            return 30;
        } else {
            return 0;
        }
    }

    // === While loops ===

    static int whileLoop(int n) {
        int sum = 0;
        int i = 0;
        while (i < n) {
            sum += i;
            i++;
        }
        return sum;
    }

    static int nestedWhileLoop(int n, int m) {
        int sum = 0;
        int i = 0;
        while (i < n) {
            int j = 0;
            while (j < m) {
                sum += 1;
                j++;
            }
            i++;
        }
        return sum;
    }

    static int whileWithBreak(int n) {
        int sum = 0;
        int i = 0;
        while (i < n * 2) {
            if (i >= n) {
                break;
            }
            sum += i;
            i++;
        }
        return sum;
    }

    static int whileWithContinue(int n) {
        int sum = 0;
        int i = 0;
        while (i < n) {
            i++;
            if (i % 2 == 0) {
                continue;
            }
            sum += i;
        }
        return sum;
    }

    // === For loops ===

    static int forLoop(int n) {
        int sum = 0;
        for (int i = 0; i < n; i++) {
            sum += i;
        }
        return sum;
    }

    static int nestedForLoop(int n, int m) {
        int sum = 0;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                sum += 1;
            }
        }
        return sum;
    }

    static int forWithBreak(int n) {
        int sum = 0;
        for (int i = 0; i < n * 2; i++) {
            if (i >= n) {
                break;
            }
            sum += i;
        }
        return sum;
    }

    static int forWithContinue(int n) {
        int sum = 0;
        for (int i = 0; i < n; i++) {
            if (i % 2 == 0) {
                continue;
            }
            sum += i;
        }
        return sum;
    }

    // === Enhanced for-loop ===

    static int enhancedForLoop(int[] arr) {
        int sum = 0;
        for (int x : arr) {
            sum += x;
        }
        return sum;
    }

    static int enhancedForWithBreak(int[] arr, int limit) {
        int sum = 0;
        for (int x : arr) {
            if (sum >= limit) {
                break;
            }
            sum += x;
        }
        return sum;
    }

    // === Try-catch-finally ===

    static int tryCatch(boolean throwException) {
        try {
            if (throwException) {
                throw new RuntimeException("test");
            }
            return 1;
        } catch (RuntimeException e) {
            return -1;
        }
    }

    static int tryCatchFinally(boolean throwException) {
        int result = 0;
        try {
            if (throwException) {
                throw new RuntimeException("test");
            }
            result = 1;
        } catch (RuntimeException e) {
            result = -1;
        } finally {
            result += 10;
        }
        return result;
    }

    static int nestedTryCatch(int level) {
        try {
            if (level == 1) {
                throw new IllegalArgumentException("level 1");
            }
            try {
                if (level == 2) {
                    throw new RuntimeException("level 2");
                }
                return 0;
            } catch (RuntimeException e) {
                return 2;
            }
        } catch (IllegalArgumentException e) {
            return 1;
        }
    }

    // === Switch statements ===

    static int intSwitch(int x) {
        switch (x) {
            case 1:
                return 10;
            case 2:
                return 20;
            case 3:
                return 30;
            default:
                return 0;
        }
    }

    static int intSwitchWithFallthrough(int x) {
        int result = 0;
        switch (x) {
            case 1:
                result += 1;
            case 2:
                result += 2;
                break;
            case 3:
                result += 3;
                break;
            default:
                result = -1;
        }
        return result;
    }

    static String charSwitch(char c) {
        switch (c) {
            case 'a':
                return "first";
            case 'b':
                return "second";
            case 'c':
            case 'd':
                return "third or fourth";
            default:
                return "other";
        }
    }

    // === Ternary operator ===

    static int ternary(int x) {
        return x > 0 ? x : -x;
    }

    static int nestedTernary(int x) {
        return x > 0 ? (x > 10 ? 2 : 1) : (x < -10 ? -2 : -1);
    }

    // === Boolean short-circuit ===

    static int and(boolean a, boolean b) {
        if (a && b) {
            return 1;
        }
        return 0;
    }

    static int or(boolean a, boolean b) {
        if (a || b) {
            return 1;
        }
        return 0;
    }

    static int complexBoolean(boolean a, boolean b, boolean c) {
        if ((a && b) || c) {
            return 1;
        }
        return 0;
    }

    // === Boolean as return value ===

    static boolean booleanReturn(int x) {
        return x > 0;
    }

    static boolean complexBooleanReturn(int x, int y) {
        return x > 0 && y > 0;
    }

    // === Boolean stored in variable (NOT as method arg - that's the known issue) ===

    static int booleanInVariable(int x) {
        boolean positive = x > 0;
        if (positive) {
            return 1;
        }
        return 0;
    }

    // === Mixed control flow ===

    static int mixedControlFlow(int x, int[] arr) {
        int sum = 0;
        if (x > 0) {
            for (int i = 0; i < arr.length; i++) {
                if (arr[i] < 0) {
                    continue;
                }
                try {
                    sum += arr[i];
                } catch (Exception e) {
                    break;
                }
            }
        } else {
            int i = 0;
            while (i < arr.length) {
                sum -= arr[i];
                i++;
            }
        }
        return sum;
    }

    // === Labeled break/continue ===

    static int labeledBreak(int n, int m) {
        int sum = 0;
        outer:
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                if (i * j >= 10) {
                    break outer;
                }
                sum += 1;
            }
        }
        return sum;
    }

    static int labeledContinue(int n, int m) {
        int sum = 0;
        outer:
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                if (j >= 2) {
                    continue outer;
                }
                sum += 1;
            }
        }
        return sum;
    }

    // === Tests ===

    static void testIfElse() {
        System.out.println("\n--- If/Else ---");
        test("simpleIfElse positive", simpleIfElse(5) == 1);
        test("simpleIfElse negative", simpleIfElse(-5) == -1);
        test("nestedIfElse +,+", nestedIfElse(1, 1) == 1);
        test("nestedIfElse +,-", nestedIfElse(1, -1) == 2);
        test("nestedIfElse -,+", nestedIfElse(-1, 1) == 3);
        test("nestedIfElse -,-", nestedIfElse(-1, -1) == 4);
        test("ifElseChain 1", ifElseChain(1) == 10);
        test("ifElseChain 2", ifElseChain(2) == 20);
        test("ifElseChain 3", ifElseChain(3) == 30);
        test("ifElseChain default", ifElseChain(99) == 0);
    }

    static void testWhileLoops() {
        System.out.println("\n--- While Loops ---");
        test("whileLoop 0", whileLoop(0) == 0);
        test("whileLoop 5", whileLoop(5) == 10);
        test("nestedWhileLoop 3x3", nestedWhileLoop(3, 3) == 9);
        test("whileWithBreak", whileWithBreak(5) == 10);
        test("whileWithContinue", whileWithContinue(5) == 9);
    }

    static void testForLoops() {
        System.out.println("\n--- For Loops ---");
        test("forLoop 0", forLoop(0) == 0);
        test("forLoop 5", forLoop(5) == 10);
        test("nestedForLoop 3x3", nestedForLoop(3, 3) == 9);
        test("forWithBreak", forWithBreak(5) == 10);
        test("forWithContinue", forWithContinue(6) == 9);
    }

    static void testEnhancedFor() {
        System.out.println("\n--- Enhanced For ---");
        int[] arr = {1, 2, 3, 4, 5};
        test("enhancedForLoop", enhancedForLoop(arr) == 15);
        test("enhancedForWithBreak", enhancedForWithBreak(arr, 6) == 6);
        test("enhancedForLoop empty", enhancedForLoop(new int[0]) == 0);
    }

    static void testTryCatch() {
        System.out.println("\n--- Try-Catch ---");
        test("tryCatch no throw", tryCatch(false) == 1);
        test("tryCatch throw", tryCatch(true) == -1);
        test("tryCatchFinally no throw", tryCatchFinally(false) == 11);
        test("tryCatchFinally throw", tryCatchFinally(true) == 9);
        test("nestedTryCatch level 0", nestedTryCatch(0) == 0);
        test("nestedTryCatch level 1", nestedTryCatch(1) == 1);
        test("nestedTryCatch level 2", nestedTryCatch(2) == 2);
    }

    static void testSwitch() {
        System.out.println("\n--- Switch ---");
        test("intSwitch 1", intSwitch(1) == 10);
        test("intSwitch 2", intSwitch(2) == 20);
        test("intSwitch 3", intSwitch(3) == 30);
        test("intSwitch default", intSwitch(99) == 0);
        test("intSwitchWithFallthrough 1", intSwitchWithFallthrough(1) == 3);
        test("intSwitchWithFallthrough 2", intSwitchWithFallthrough(2) == 2);
        test("intSwitchWithFallthrough 3", intSwitchWithFallthrough(3) == 3);
        test("charSwitch a", charSwitch('a').equals("first"));
        test("charSwitch b", charSwitch('b').equals("second"));
        test("charSwitch c", charSwitch('c').equals("third or fourth"));
        test("charSwitch x", charSwitch('x').equals("other"));
    }

    static void testTernary() {
        System.out.println("\n--- Ternary ---");
        test("ternary positive", ternary(5) == 5);
        test("ternary negative", ternary(-5) == 5);
        test("nestedTernary >10", nestedTernary(15) == 2);
        test("nestedTernary 0<x<=10", nestedTernary(5) == 1);
        test("nestedTernary -10<x<=0", nestedTernary(-5) == -1);
        test("nestedTernary <-10", nestedTernary(-15) == -2);
    }

    static void testBoolean() {
        System.out.println("\n--- Boolean ---");
        test("and true,true", and(true, true) == 1);
        test("and true,false", and(true, false) == 0);
        test("and false,true", and(false, true) == 0);
        test("and false,false", and(false, false) == 0);
        test("or true,true", or(true, true) == 1);
        test("or true,false", or(true, false) == 1);
        test("or false,true", or(false, true) == 1);
        test("or false,false", or(false, false) == 0);
        test("complexBoolean t,t,f", complexBoolean(true, true, false) == 1);
        test("complexBoolean f,f,t", complexBoolean(false, false, true) == 1);
        test("complexBoolean f,t,f", complexBoolean(false, true, false) == 0);
        test("booleanReturn positive", booleanReturn(5) == true);
        test("booleanReturn negative", booleanReturn(-5) == false);
        test("complexBooleanReturn +,+", complexBooleanReturn(1, 1) == true);
        test("complexBooleanReturn +,-", complexBooleanReturn(1, -1) == false);
        test("booleanInVariable positive", booleanInVariable(5) == 1);
        test("booleanInVariable negative", booleanInVariable(-5) == 0);
    }

    static void testMixed() {
        System.out.println("\n--- Mixed Control Flow ---");
        int[] arr = {1, 2, -1, 3, 4};
        test("mixedControlFlow positive", mixedControlFlow(1, arr) == 10);
        test("mixedControlFlow negative", mixedControlFlow(-1, arr) == -9);
    }

    static void testLabeled() {
        System.out.println("\n--- Labeled Break/Continue ---");
        // i=0: 5 iterations (0*j < 10 always)
        // i=1: 5 iterations (1*j <= 4)
        // i=2: 5 iterations (2*j <= 8)
        // i=3: 4 iterations (3*3=9 ok, but 3*4=12 >= 10, break)
        // Total: 5 + 5 + 5 + 4 = 19
        test("labeledBreak 5x5", labeledBreak(5, 5) == 19);
        test("labeledContinue 5x5", labeledContinue(5, 5) == 10);
    }

    public static void main(String[] args) {
        System.out.println("=== StackMapTable Verification Test ===");
        System.out.println("This test exercises control flow patterns that require");
        System.out.println("proper StackMapTable generation for Java 7+ bytecode.");

        testIfElse();
        testWhileLoops();
        testForLoops();
        testEnhancedFor();
        testTryCatch();
        testSwitch();
        testTernary();
        testBoolean();
        testMixed();
        testLabeled();

        System.out.println("\n=== RESULTS ===");
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);

        if (failed > 0) {
            System.exit(1);
        }
    }
}
