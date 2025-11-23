// Simplified test for method autoboxing
public class SimpleMethodBoxingTest {
    static void takeInteger(Integer i) {
        int x = i.intValue();
    }

    public static void main(String[] args) {
        int primitiveInt = 42;
        takeInteger(primitiveInt);  // Should autobox int -> Integer
    }
}
