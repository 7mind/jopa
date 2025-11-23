// Test autoboxing/unboxing in arithmetic operations
public class ArithmeticBoxingTest {
    public static void main(String[] args) {
        System.out.println("Testing autoboxing in arithmetic operations");

        // Test 1: Unboxing for arithmetic operators
        Integer a = new Integer(10);
        Integer b = new Integer(20);
        int sum = a + b;  // Should unbox both and add
        System.out.println("  10 + 20 = " + sum);

        if (sum == 30) {
            System.out.println("Test 1: Addition passed");
        }

        // Test 2: Mixed primitive and wrapper
        Integer x = new Integer(50);
        int y = 25;
        int result = x - y;  // Should unbox x
        System.out.println("  50 - 25 = " + result);

        if (result == 25) {
            System.out.println("Test 2: Subtraction passed");
        }

        // Test 3: Multiplication
        Integer m = new Integer(7);
        Integer n = new Integer(6);
        int product = m * n;  // Should unbox both
        System.out.println("  7 * 6 = " + product);

        if (product == 42) {
            System.out.println("Test 3: Multiplication passed");
        }

        // Test 4: Division
        Integer dividend = new Integer(100);
        Integer divisor = new Integer(4);
        int quotient = dividend / divisor;  // Should unbox both
        System.out.println("  100 / 4 = " + quotient);

        if (quotient == 25) {
            System.out.println("Test 4: Division passed");
        }

        // Test 5: Comparison operators
        Integer val1 = new Integer(15);
        Integer val2 = new Integer(10);
        boolean greater = val1 > val2;  // Should unbox both
        System.out.println("  15 > 10 = " + greater);

        if (greater) {
            System.out.println("Test 5: Comparison passed");
        }

        // Test 6: Increment/decrement
        Integer count = new Integer(5);
        count++;  // Should unbox, increment, and box back
        System.out.println("  5++ = " + count);

        if (count.intValue() == 6) {
            System.out.println("Test 6: Increment passed");
        }

        System.out.println("All arithmetic boxing tests passed!");
    }
}
