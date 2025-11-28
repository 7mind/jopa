// Test constructor overload resolution with boxing/unboxing

public class ConstructorOverloadTest {
    // Constructor taking primitives
    public ConstructorOverloadTest(double a, double b) {
        // This calls the Object version, NOT itself
        this(new Double(a), new Double(b));
    }

    // Constructor taking Objects
    public ConstructorOverloadTest(Number a, Number b) {
        System.out.println("Number constructor called");
    }

    public static void main(String[] args) {
        new ConstructorOverloadTest(1.0, 2.0);
        System.out.println("ConstructorOverloadTest: PASS");
    }
}
