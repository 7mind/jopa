// Test autoboxing/unboxing in method arguments without string concatenation
public class MethodBoxingSimple {
    // Methods that take wrapper types
    static void takeInteger(Integer i) {
        int x = i.intValue();
    }

    static void takeDouble(Double d) {
        double x = d.doubleValue();
    }

    static void takeBool(Boolean b) {
        boolean x = b.booleanValue();
    }

    // Methods that take primitive types
    static void takePrimitiveInt(int i) {
    }

    static void takePrimitiveDouble(double d) {
    }

    public static void main(String[] args) {
        // Test 1: Boxing primitive to wrapper parameter
        int primitiveInt = 42;
        takeInteger(primitiveInt);  // Should autobox int -> Integer

        double primitiveDouble = 3.14;
        takeDouble(primitiveDouble);  // Should autobox double -> Double

        boolean primitiveBool = true;
        takeBool(primitiveBool);  // Should autobox boolean -> Boolean

        // Test 2: Unboxing wrapper to primitive parameter
        Integer wrapperInt = new Integer(99);
        takePrimitiveInt(wrapperInt);  // Should unbox Integer -> int

        Double wrapperDouble = new Double(2.718);
        takePrimitiveDouble(wrapperDouble);  // Should unbox Double -> double

        // Test 3: Direct literals
        takeInteger(123);  // int literal -> Integer
        takeDouble(4.56);  // double literal -> Double
        takeBool(false);   // boolean literal -> Boolean
    }
}
