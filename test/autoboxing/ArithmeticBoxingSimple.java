// Test autoboxing/unboxing in arithmetic operations
public class ArithmeticBoxingSimple {
    public static void main(String[] args) {
        // Test 1: Unboxing for arithmetic operators
        Integer a = new Integer(10);
        Integer b = new Integer(20);
        int sum = a + b;  // Should unbox both and add

        if (sum != 30) {
            throw new RuntimeException("Test 1 failed");
        }

        // Test 2: Mixed primitive and wrapper
        Integer x = new Integer(50);
        int y = 25;
        int result = x - y;  // Should unbox x

        if (result != 25) {
            throw new RuntimeException("Test 2 failed");
        }

        // Test 3: Multiplication
        Integer m = new Integer(7);
        Integer n = new Integer(6);
        int product = m * n;  // Should unbox both

        if (product != 42) {
            throw new RuntimeException("Test 3 failed");
        }

        // Test 4: Division
        Integer dividend = new Integer(100);
        Integer divisor = new Integer(4);
        int quotient = dividend / divisor;  // Should unbox both

        if (quotient != 25) {
            throw new RuntimeException("Test 4 failed");
        }

        // Test 5: Comparison operators
        Integer val1 = new Integer(15);
        Integer val2 = new Integer(10);
        boolean greater = val1 > val2;  // Should unbox both

        if (!greater) {
            throw new RuntimeException("Test 5 failed");
        }
    }
}
