// Comprehensive test for generics implementation

// Test 1: Basic generic class with type parameter
class Box<T> {
    T value;

    Box(T v) {
        value = v;
    }

    T get() {
        return value;
    }

    void set(T v) {
        value = v;
    }
}

// Test 2: Bounded type parameter
class NumberBox<T extends TestNumber> {
    T value;

    T get() {
        return value;
    }
}

// Test 3: Multiple type parameters
class Pair<K, V> {
    K key;
    V value;

    K getKey() {
        return key;
    }

    V getValue() {
        return value;
    }
}

// Test 4: Generic method
class Utils {
    static <T> T identity(T arg) {
        return arg;
    }

    static <T extends TestNumber> T max(T a, T b) {
        return a; // Simplified
    }
}

// Test 5: Inheritance with covariant override (bridge method needed)
class StringBox extends Box<String> {
    String get() {  // Overrides Box.get() : Object with String return type
        return "test";
    }
}

// Test 6: Wildcard types
class WildcardTest {
    Box<?> unbounded;
    Box<? extends TestNumber> upperBounded;
    Box<? super TestInteger> lowerBounded;
}

// Test 7: Nested generics
class NestedBox<T> {
    Box<Box<T>> nested;
}

// Stub classes for testing
class TestNumber {
    int value;
}

class TestInteger extends TestNumber {
    TestInteger(int v) {
        value = v;
    }
}

class String {
    String() {}
}
