// Test raw types (generic types used without type arguments)

class Box<T> {
    T value;

    Box(T v) {
        value = v;
    }

    T get() {
        return value;
    }
}

class TestRawTypes {
    // Raw type - Box used without type arguments
    Box rawBox;

    void test() {
        // Raw type in local variable
        Box raw = new Box(new Object());

        // Mixed: parameterized type assigned to raw type
        Box raw2 = new Box<Object>(new Object());

        // Mixed: raw type assigned to parameterized type
        Box<Object> param = new Box(new Object());
    }
}
