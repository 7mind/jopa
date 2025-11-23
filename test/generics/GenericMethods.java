// Test generic methods

class Container<T> {
    T value;

    Container(T v) {
        value = v;
    }
}

class GenericMethodTest {
    // Generic method with single type parameter
    static <T> T identity(T arg) {
        return arg;
    }

    // Generic method with multiple type parameters
    static <K, V> Container<K> firstOfPair(K key, V value) {
        return new Container<K>(key);
    }

    // Generic method with bounded type parameter
    static <T extends Number> T max(T a, T b) {
        return a;  // Simplified - would need actual comparison
    }

    // Instance generic method
    <E> void processElement(E element) {
        // Process element
    }

    // Generic method that uses class type parameter and method type parameter
    <E> void combine(E methodParam) {
        // Can use both E (method type param) here
    }
}

class Number {
    int value;
}

class TestGenericMethods {
    void test() {
        // Call generic methods
        Object obj = GenericMethodTest.identity(new Object());
        Container<Object> c = GenericMethodTest.firstOfPair(new Object(), new Object());

        GenericMethodTest gmt = new GenericMethodTest();
        gmt.processElement(new Object());
        gmt.combine(new Object());
    }
}
