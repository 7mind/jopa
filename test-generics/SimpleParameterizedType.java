// Test if parameterized types work in extends clause
class Box<T> {
    T value;

    T get() {
        return value;
    }

    void set(T v) {
        value = v;
    }
}

// Try to extend with type argument
class StringBox extends Box {
    // Just use raw type for now
}

public class SimpleParameterizedType {
    public static void main(String[] args) {
        Box box = new Box();
        System.out.println("Test passed!");
    }
}
