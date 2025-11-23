// Test generic constructors
class Box<T> {
    T value;

    // Generic constructor
    <U> Box(U initialValue, Class<T> typeClass) {
        // Constructor with type parameter U different from class type parameter T
        // This tests if constructor type parameters are handled
    }

    // Regular constructor
    Box(T v) {
        value = v;
    }
}

class Container {
    // Generic constructor in non-generic class
    <T> Container(T value) {
        // Constructor with type parameter in non-generic class
    }
}

public class GenericConstructorTest {
    public static void main(String[] args) {
        // Test generic constructor
        Box<String> box1 = new Box<String>("hello");

        // Test constructor with type parameter
        Container c = new Container("test");

        System.out.println("Generic constructor test passed!");
    }
}

class Class<T> {
    // Stub Class type for compilation
}
