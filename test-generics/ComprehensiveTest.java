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
class NumberBox<T extends Number> {
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

    static <T extends Number> T max(T a, T b) {
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
    Box<? extends Number> upperBounded;
    Box<? super Integer> lowerBounded;
}

// Test 7: Nested generics
class NestedBox<T> {
    Box<Box<T>> nested;
}

// Stub classes for testing
class Number {
    int value;
}

class Integer extends Number {
    Integer(int v) {
        value = v;
    }
}

class String {
    String() {}
}
