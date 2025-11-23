package test;

// ==================================================================
// COMPREHENSIVE TEST: All Implemented Java 5 Generics Features
// ==================================================================

// ===== FEATURE 1: Basic Generic Class with Type Parameter =====
class Box<T> {
    private T value;

    public Box(T v) {
        value = v;
    }

    public T getValue() {
        return value;
    }

    public void setValue(T v) {
        value = v;
    }
}

// ===== FEATURE 2: Bounded Type Parameters =====
class NumericBox<T extends TestNumber> {
    private T number;

    public T getNumber() {
        return number;
    }
}

// ===== FEATURE 3: Multiple Type Parameters =====
class Pair<K, V> {
    private K key;
    private V value;

    public Pair(K k, V v) {
        key = k;
        value = v;
    }

    public K getKey() { return key; }
    public V getValue() { return value; }
}

// ===== FEATURE 4: Multiple Bounds on Type Parameters =====
interface Comparable<T> {
    int compareTo(T other);
}

class BoundedPair<T extends TestNumber & Comparable<T>> {
    private T first;
    private T second;
}

// ===== FEATURE 5: Generic Methods =====
class Utils {
    // Generic method with single type parameter
    public static <T> T identity(T arg) {
        return arg;
    }

    // Generic method with bounded type parameter
    public static <T extends TestNumber> T max(T a, T b) {
        return a;  // Simplified
    }

    // Generic method with multiple type parameters
    public static <K, V> Pair<K, V> makePair(K k, V v) {
        return new Pair<K, V>(k, v);
    }
}

// ===== FEATURE 6: Generic Constructors =====
class Container {
    // Generic constructor with type parameter
    public <T> Container(T value) {
        // Constructor with generic type
    }

    // Regular constructor
    public Container() {
    }
}

// ===== FEATURE 7: Inheritance with Generics (Bridge Methods) =====
class GenericBase<T> {
    public T get() {
        return null;
    }

    public void set(T value) {
    }
}

// Covariant override - triggers bridge method generation
class StringChild extends GenericBase<String> {
    public String get() {  // Overrides GenericBase<String>.get() : Object
        return "test";
    }

    public void set(String value) {  // Overrides GenericBase<String>.set(Object)
    }
}

// ===== FEATURE 8: Wildcard Types =====
class WildcardExamples {
    // Unbounded wildcard
    Box<?> unknownBox;

    // Upper bounded wildcard
    Box<? extends TestNumber> numberBox;

    // Lower bounded wildcard
    Box<? super TestInteger> integerSuperBox;

    // Method with wildcards
    public void processBox(Box<? extends TestNumber> box) {
    }
}

// ===== FEATURE 9: Nested Generics =====
class NestedGenerics {
    Box<Box<String>> boxOfBoxes;
    Pair<Box<TestInteger>, Box<String>> pairOfBoxes;
    Box<Pair<String, TestInteger>> boxOfPair;
}

// ===== FEATURE 10: Raw Types (with warnings) =====
class RawTypeExample {
    Box rawBox;  // Raw type usage - generates warning

    public void useRawType() {
        Box b = new Box("test");  // Raw type - generates warning
    }
}

// ===== Supporting Classes =====
class TestNumber {
    protected int value;

    public TestNumber() {
        value = 0;
    }
}

class TestInteger extends TestNumber implements Comparable<TestInteger> {
    public TestInteger(int v) {
        value = v;
    }

    public int compareTo(Integer other) {
        return value - other.value;
    }
}

class TestDouble extends TestNumber {
    private double dvalue;

    public TestDouble(double v) {
        dvalue = v;
    }
}

class String {
    public String() {}
}

// ===== FEATURE 11: Type Parameter Scope =====
class ScopeTest<T> {
    private T classParam;

    // Method type parameter shadows class type parameter
    public <T> T methodParam(T arg) {
        return arg;  // Returns method's T, not class's T
    }

    // Using class type parameter
    public T getClassParam() {
        return classParam;
    }
}

// ===== FEATURE 12: Generic Interfaces =====
interface GenericInterface<T> {
    T process(T input);
    void accept(T value);
}

class GenericImpl<T> implements GenericInterface<T> {
    public T process(T input) {
        return input;
    }

    public void accept(T value) {
    }
}

// ===== Test Client Code =====
class TestClient {
    public void testAllFeatures() {
        // Test parameterized types
        Box<String> stringBox = new Box<String>("hello");
        Box<TestInteger> intBox = new Box<TestInteger>(new TestInteger(42));

        // Test generic methods
        String s = Utils.identity("test");
        TestInteger i = Utils.max(new TestInteger(1), new TestInteger(2));

        // Test wildcards
        Box<? extends TestNumber> numBox = intBox;

        // Test nested generics
        Box<Box<String>> nested = new Box<Box<String>>(stringBox);

        // Test pairs
        Pair<String, TestInteger> pair = new Pair<String, TestInteger>("age", new TestInteger(25));
    }
}
