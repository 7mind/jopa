// Test autoboxing/unboxing in method arguments
public class MethodBoxingTest {
    // Methods that take wrapper types
    static void takeInteger(Integer i) {
        System.out.println("  takeInteger got: " + i);
    }

    static void takeDouble(Double d) {
        System.out.println("  takeDouble got: " + d);
    }

    static void takeBool(Boolean b) {
        System.out.println("  takeBool got: " + b);
    }

    // Methods that take primitive types
    static void takePrimitiveInt(int i) {
        System.out.println("  takePrimitiveInt got: " + i);
    }

    static void takePrimitiveDouble(double d) {
        System.out.println("  takePrimitiveDouble got: " + d);
    }

    public static void main(String[] args) {
        System.out.println("Testing autoboxing in method arguments");

        // Test 1: Boxing primitive to wrapper parameter
        int primitiveInt = 42;
        takeInteger(primitiveInt);  // Should autobox int -> Integer

        double primitiveDouble = 3.14;
        takeDouble(primitiveDouble);  // Should autobox double -> Double

        boolean primitiveBool = true;
        takeBool(primitiveBool);  // Should autobox boolean -> Boolean

        System.out.println("Test 1: Boxing primitives passed");

        // Test 2: Unboxing wrapper to primitive parameter
        Integer wrapperInt = new Integer(99);
        takePrimitiveInt(wrapperInt);  // Should unbox Integer -> int

        Double wrapperDouble = new Double(2.718);
        takePrimitiveDouble(wrapperDouble);  // Should unbox Double -> double

        System.out.println("Test 2: Unboxing wrappers passed");

        // Test 3: Direct literals
        takeInteger(123);  // int literal -> Integer
        takeDouble(4.56);  // double literal -> Double
        takeBool(false);   // boolean literal -> Boolean

        System.out.println("Test 3: Direct literals passed");

        System.out.println("All method boxing tests passed!");
    }
}
