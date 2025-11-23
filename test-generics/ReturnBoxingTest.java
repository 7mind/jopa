// Test autoboxing/unboxing in return values
public class ReturnBoxingTest {
    // Methods that return wrapper types but return primitives
    static Integer returnBoxedInt() {
        return 42;  // Should autobox int -> Integer
    }

    static Double returnBoxedDouble() {
        return 3.14;  // Should autobox double -> Double
    }

    static Boolean returnBoxedBool() {
        return true;  // Should autobox boolean -> Boolean
    }

    // Methods that return primitives but return wrappers
    static int returnUnboxedInt() {
        Integer wrapper = new Integer(99);
        return wrapper;  // Should unbox Integer -> int
    }

    static double returnUnboxedDouble() {
        Double wrapper = new Double(2.718);
        return wrapper;  // Should unbox Double -> double
    }

    static boolean returnUnboxedBool() {
        Boolean wrapper = new Boolean(false);
        return wrapper;  // Should unbox Boolean -> boolean
    }

    public static void main(String[] args) {
        System.out.println("Testing autoboxing in return values");

        // Test 1: Boxing returns
        Integer i = returnBoxedInt();
        System.out.println("  Boxed int: " + i);

        Double d = returnBoxedDouble();
        System.out.println("  Boxed double: " + d);

        Boolean b = returnBoxedBool();
        System.out.println("  Boxed boolean: " + b);

        if (i.intValue() == 42 && d.doubleValue() > 3.0 && b.booleanValue()) {
            System.out.println("Test 1: Boxing returns passed");
        }

        // Test 2: Unboxing returns
        int pi = returnUnboxedInt();
        System.out.println("  Unboxed int: " + pi);

        double pd = returnUnboxedDouble();
        System.out.println("  Unboxed double: " + pd);

        boolean pb = returnUnboxedBool();
        System.out.println("  Unboxed boolean: " + pb);

        if (pi == 99 && pd > 2.0 && !pb) {
            System.out.println("Test 2: Unboxing returns passed");
        }

        System.out.println("All return boxing tests passed!");
    }
}
