// Test autoboxing/unboxing in return values (simple, no string concat)
public class ReturnBoxingSimple {
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
        // Test 1: Boxing returns
        Integer i = returnBoxedInt();
        Double d = returnBoxedDouble();
        Boolean b = returnBoxedBool();

        if (i.intValue() == 42 && d.doubleValue() > 3.0 && b.booleanValue()) {
            // pass
        }

        // Test 2: Unboxing returns
        int pi = returnUnboxedInt();
        double pd = returnUnboxedDouble();
        boolean pb = returnUnboxedBool();

        if (pi == 99 && pd > 2.0 && !pb) {
            // pass
        }
    }
}
