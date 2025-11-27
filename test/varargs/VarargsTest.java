// Comprehensive varargs test - covers all corner cases

public class VarargsTest {
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

    // === Basic varargs methods ===

    static String simpleVarargs(String... args) {
        return "count=" + args.length;
    }

    static String oneFixed(String first, String... rest) {
        return "first=" + first + ",rest=" + rest.length;
    }

    static String twoFixed(String a, String b, String... rest) {
        return "a=" + a + ",b=" + b + ",rest=" + rest.length;
    }

    static String threeFixed(String a, String b, String c, String... rest) {
        return "a=" + a + ",b=" + b + ",c=" + c + ",rest=" + rest.length;
    }

    // === Varargs with primitives ===

    static int sumInts(int... nums) {
        int sum = 0;
        for (int i = 0; i < nums.length; i++) {
            sum += nums[i];
        }
        return sum;
    }

    static int mixedPrimitive(int first, int second, int... rest) {
        int sum = first + second;
        for (int i = 0; i < rest.length; i++) {
            sum += rest[i];
        }
        return sum;
    }

    // === Varargs with Object type ===

    static String objectVarargs(Object... args) {
        StringBuilder sb = new StringBuilder();
        sb.append("[");
        for (int i = 0; i < args.length; i++) {
            if (i > 0) sb.append(",");
            sb.append(args[i]);
        }
        sb.append("]");
        return sb.toString();
    }

    // === Varargs with generic array return ===

    static String[] returnArray(String... args) {
        return args;
    }

    // === Varargs with different fixed args (no ambiguous overloads) ===

    static String varargsOnly(int x, int... rest) {
        return "int,varargs:" + x + "," + rest.length;
    }

    // === Varargs with Object type accepting subtypes ===

    static String acceptObject(Object... args) {
        return "objects:" + args.length;
    }

    // === Test methods ===

    static void testBasicVarargs() {
        System.out.println("\n--- Basic Varargs ---");

        // No arguments
        test("simpleVarargs()", simpleVarargs().equals("count=0"));

        // Single argument
        test("simpleVarargs(one)", simpleVarargs("one").equals("count=1"));

        // Multiple arguments
        test("simpleVarargs(a,b,c)", simpleVarargs("a", "b", "c").equals("count=3"));

        // Explicit array
        test("simpleVarargs(array)", simpleVarargs(new String[]{"x", "y"}).equals("count=2"));

        // Empty explicit array
        test("simpleVarargs(empty array)", simpleVarargs(new String[0]).equals("count=0"));
    }

    static void testOneFixed() {
        System.out.println("\n--- One Fixed + Varargs ---");

        // Just the fixed argument
        test("oneFixed(first)", oneFixed("first").equals("first=first,rest=0"));

        // Fixed + one vararg
        test("oneFixed(first,second)", oneFixed("first", "second").equals("first=first,rest=1"));

        // Fixed + multiple varargs
        test("oneFixed(a,b,c,d)", oneFixed("a", "b", "c", "d").equals("first=a,rest=3"));

        // Fixed + explicit array
        test("oneFixed(first,array)", oneFixed("first", new String[]{"x", "y"}).equals("first=first,rest=2"));
    }

    static void testTwoFixed() {
        System.out.println("\n--- Two Fixed + Varargs ---");

        // Just the fixed arguments
        test("twoFixed(a,b)", twoFixed("a", "b").equals("a=a,b=b,rest=0"));

        // Fixed + one vararg
        test("twoFixed(a,b,c)", twoFixed("a", "b", "c").equals("a=a,b=b,rest=1"));

        // Fixed + multiple varargs
        test("twoFixed(a,b,c,d,e)", twoFixed("a", "b", "c", "d", "e").equals("a=a,b=b,rest=3"));

        // Fixed + explicit array
        test("twoFixed(a,b,array)", twoFixed("a", "b", new String[]{"c", "d"}).equals("a=a,b=b,rest=2"));
    }

    static void testThreeFixed() {
        System.out.println("\n--- Three Fixed + Varargs ---");

        // Just the fixed arguments
        test("threeFixed(a,b,c)", threeFixed("a", "b", "c").equals("a=a,b=b,c=c,rest=0"));

        // Fixed + one vararg
        test("threeFixed(a,b,c,d)", threeFixed("a", "b", "c", "d").equals("a=a,b=b,c=c,rest=1"));

        // Fixed + multiple varargs
        test("threeFixed(a,b,c,d,e,f)", threeFixed("a", "b", "c", "d", "e", "f").equals("a=a,b=b,c=c,rest=3"));

        // Fixed + explicit array
        test("threeFixed(a,b,c,array)", threeFixed("a", "b", "c", new String[]{"d", "e"}).equals("a=a,b=b,c=c,rest=2"));
    }

    static void testPrimitiveVarargs() {
        System.out.println("\n--- Primitive Varargs ---");

        // No arguments
        test("sumInts()", sumInts() == 0);

        // Single argument
        test("sumInts(5)", sumInts(5) == 5);

        // Multiple arguments
        test("sumInts(1,2,3,4,5)", sumInts(1, 2, 3, 4, 5) == 15);

        // Explicit array
        test("sumInts(array)", sumInts(new int[]{10, 20, 30}) == 60);

        // Mixed primitive
        test("mixedPrimitive(1,2)", mixedPrimitive(1, 2) == 3);
        test("mixedPrimitive(1,2,3)", mixedPrimitive(1, 2, 3) == 6);
        test("mixedPrimitive(1,2,3,4,5)", mixedPrimitive(1, 2, 3, 4, 5) == 15);
        test("mixedPrimitive(1,2,array)", mixedPrimitive(1, 2, new int[]{3, 4}) == 10);
    }

    static void testObjectVarargs() {
        System.out.println("\n--- Object Varargs ---");

        // No arguments
        test("objectVarargs()", objectVarargs().equals("[]"));

        // Mixed types
        test("objectVarargs(mixed)", objectVarargs("str", Integer.valueOf(42), "end").equals("[str,42,end]"));

        // Explicit Object array
        test("objectVarargs(Object[])", objectVarargs(new Object[]{"a", "b"}).equals("[a,b]"));

        // String array passed as Object varargs (should work due to array covariance)
        String[] strArr = new String[]{"x", "y"};
        test("objectVarargs(String[])", objectVarargs((Object[])strArr).equals("[x,y]"));
    }

    static void testReturnArray() {
        System.out.println("\n--- Varargs Return ---");

        String[] result = returnArray("a", "b", "c");
        test("returnArray length", result.length == 3);
        test("returnArray[0]", result[0].equals("a"));
        test("returnArray[2]", result[2].equals("c"));

        // Pass through
        String[] input = new String[]{"x", "y"};
        String[] output = returnArray(input);
        test("returnArray passthrough", input == output);
    }

    static void testVarargsOnly() {
        System.out.println("\n--- Varargs Only (no overloads) ---");

        // Single fixed + varargs
        test("varargsOnly(1)", varargsOnly(1).equals("int,varargs:1,0"));
        test("varargsOnly(1,2)", varargsOnly(1, 2).equals("int,varargs:1,1"));
        test("varargsOnly(1,2,3)", varargsOnly(1, 2, 3).equals("int,varargs:1,2"));

        // Explicit array
        test("varargsOnly(1,array)", varargsOnly(1, new int[]{2, 3}).equals("int,varargs:1,2"));
    }

    static void testSubtypeArrays() {
        System.out.println("\n--- Subtype Arrays ---");

        // String array passed to Object varargs
        test("acceptObject(String...)", acceptObject("a", "b").equals("objects:2"));

        // Explicit String array passed to Object varargs
        test("acceptObject(String[])", acceptObject(new String[]{"x", "y", "z"}).equals("objects:3"));

        // Explicit Object array
        test("acceptObject(Object[])", acceptObject(new Object[]{"a", "b", "c", "d"}).equals("objects:4"));
    }

    public static void main(String[] args) {
        System.out.println("=== Varargs Comprehensive Test ===");

        testBasicVarargs();
        testOneFixed();
        testTwoFixed();
        testThreeFixed();
        testPrimitiveVarargs();
        testObjectVarargs();
        testReturnArray();
        testVarargsOnly();
        testSubtypeArrays();

        System.out.println("\n=== RESULTS ===");
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);

        if (failed > 0) {
            System.exit(1);
        }
    }
}
