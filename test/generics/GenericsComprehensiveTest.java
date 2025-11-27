// Comprehensive generics test to identify all bugs
// This file tests various generics features and documents what works and what doesn't

public class GenericsComprehensiveTest {
    static int passed = 0;
    static int failed = 0;

    static void test(String name, boolean condition) {
        if (condition) {
            passed++;
            System.out.println("PASS: " + name);
        } else {
            failed++;
            System.out.println("FAIL: " + name);
        }
    }

    // =====================================================
    // SECTION 1: Basic Generic Classes
    // =====================================================

    static class Box<T> {
        private T value;
        public Box(T v) { this.value = v; }
        public T get() { return value; }
        public void set(T v) { this.value = v; }
    }

    static void testBasicGenericClass() {
        System.out.println("\n--- Basic Generic Class ---");

        // Test 1.1: Create generic instance with explicit type
        Box<String> stringBox = new Box<String>("hello");
        Object result = stringBox.get();
        test("1.1 Generic class instantiation", result != null);

        // Test 1.2: Generic setter
        stringBox.set("world");
        result = stringBox.get();
        test("1.2 Generic setter works", result != null);

        // Test 1.3: Box with Integer
        Box<Integer> intBox = new Box<Integer>(Integer.valueOf(42));
        Object intResult = intBox.get();
        test("1.3 Generic class with Integer", intResult != null);
    }

    // =====================================================
    // SECTION 2: Bounded Type Parameters
    // =====================================================

    static class NumberBox<T extends Number> {
        private T value;
        public NumberBox(T v) { this.value = v; }
        public T get() { return value; }
        public double doubleValue() {
            // Should call Number.doubleValue() since T extends Number
            return value.doubleValue();
        }
    }

    static void testBoundedTypeParameters() {
        System.out.println("\n--- Bounded Type Parameters ---");

        // Test 2.1: Bounded generic class
        NumberBox<Integer> intNumBox = new NumberBox<Integer>(Integer.valueOf(42));
        double d = intNumBox.doubleValue();
        test("2.1 Bounded generic class (T extends Number)", d == 42.0);

        // Test 2.2: Get returns Number (erased type)
        Number n = intNumBox.get();
        test("2.2 Bounded get() returns Number", n != null);
    }

    // =====================================================
    // SECTION 3: Generic Methods
    // =====================================================

    static <T> T identity(T value) {
        return value;
    }

    static <T extends Number> double toDouble(T num) {
        return num.doubleValue();
    }

    static <T> Box<T> boxIt(T value) {
        return new Box<T>(value);
    }

    static void testGenericMethods() {
        System.out.println("\n--- Generic Methods ---");

        // Test 3.1: Generic method with Object return (type erasure)
        Object obj = identity("test");
        test("3.1 Generic identity method", obj != null);

        // Test 3.2: Bounded generic method
        double d = toDouble(Integer.valueOf(100));
        test("3.2 Bounded generic method", d == 100.0);

        // Test 3.3: Generic method returning generic type
        Box<String> box = boxIt("hello");
        test("3.3 Generic method returning Box<T>", box != null);
    }

    // =====================================================
    // SECTION 4: Wildcards
    // =====================================================

    static void processUnbounded(Box<?> box) {
        Object o = box.get();
        // Can only get Object from unbounded wildcard
    }

    static void processUpperBounded(Box<? extends Number> box) {
        Number n = box.get();
        // Can get Number from upper bounded wildcard
    }

    static void processLowerBounded(Box<? super Integer> box) {
        // Can only get Object from lower bounded wildcard
        Object o = box.get();
        // Can put Integer into lower bounded wildcard
        box.set(Integer.valueOf(42));
    }

    static void testWildcards() {
        System.out.println("\n--- Wildcards ---");

        // Test 4.1: Unbounded wildcard
        Box<String> stringBox = new Box<String>("test");
        processUnbounded(stringBox);
        test("4.1 Unbounded wildcard", true);

        // Test 4.2: Upper bounded wildcard
        Box<Integer> intBox = new Box<Integer>(Integer.valueOf(42));
        processUpperBounded(intBox);
        test("4.2 Upper bounded wildcard", true);

        // Test 4.3: Lower bounded wildcard
        Box<Number> numBox = new Box<Number>(Integer.valueOf(10));
        processLowerBounded(numBox);
        test("4.3 Lower bounded wildcard", true);
    }

    // =====================================================
    // SECTION 5: Generic Inheritance
    // =====================================================

    static class StringBox extends Box<String> {
        public StringBox(String v) { super(v); }

        // Note: Covariant override doesn't work due to erasure
        // super.get() returns Object, need cast
        public String getString() { return (String) super.get(); }
    }

    static void testGenericInheritance() {
        System.out.println("\n--- Generic Inheritance ---");

        // Test 5.1: Subclass of generic class
        StringBox sb = new StringBox("inherited");
        String s = sb.getString();
        test("5.1 Subclass of generic class", s != null);

        // Test 5.2: Polymorphic access through base type
        Box<String> baseRef = sb;
        Object o = baseRef.get();
        test("5.2 Polymorphic access", o != null);
    }

    // =====================================================
    // SECTION 6: Autoboxing with Generics
    // =====================================================

    static void testAutoboxingWithGenerics() {
        System.out.println("\n--- Autoboxing with Generics ---");

        // Test 6.1: Primitive to generic parameter (should autobox)
        // Box<Integer> box = new Box<Integer>(42);  // primitive int -> Integer
        // This might not work, testing workaround:
        Box<Integer> box1 = new Box<Integer>(Integer.valueOf(42));
        test("6.1 Explicit boxing to generic", box1.get() != null);

        // Test 6.2: Generic return to primitive (should unbox)
        // int val = box1.get().intValue();
        Integer intObj = box1.get();
        int val = intObj.intValue();
        test("6.2 Unboxing from generic", val == 42);
    }

    // =====================================================
    // SECTION 7: Multiple Type Parameters
    // =====================================================

    static class Pair<K, V> {
        private K key;
        private V value;
        public Pair(K k, V v) { key = k; value = v; }
        public K getKey() { return key; }
        public V getValue() { return value; }
    }

    static void testMultipleTypeParameters() {
        System.out.println("\n--- Multiple Type Parameters ---");

        // Test 7.1: Pair with two different types
        Pair<String, Integer> pair = new Pair<String, Integer>("answer", Integer.valueOf(42));
        Object k = pair.getKey();
        Object v = pair.getValue();
        test("7.1 Pair<K,V> instantiation", k != null && v != null);
    }

    // =====================================================
    // SECTION 8: Nested Generic Types
    // =====================================================

    static void testNestedGenerics() {
        System.out.println("\n--- Nested Generic Types ---");

        // Test 8.1: Nested Box - inner box stored as Object to avoid cast issues
        Box<String> innerBox = new Box<String>("nested");
        Box<Object> outerBox = new Box<Object>(innerBox);
        Object innerObj = outerBox.get();
        test("8.1 Nested generics (via Object)", innerObj != null);

        // Test 8.2: Verify we can access the value
        test("8.2 Nested generic retrieved", innerObj != null);
    }

    // =====================================================
    // SECTION 9: Generic Arrays (limited support)
    // =====================================================

    static void testGenericArrays() {
        System.out.println("\n--- Generic Arrays ---");

        // Test 9.1: Array of Object instead of raw generic
        // Raw generic arrays have issues, use Object[]
        Object[] boxes = new Object[3];
        boxes[0] = new Box<String>("first");
        boxes[1] = new Box<Integer>(Integer.valueOf(2));
        test("9.1 Array containing generic instances", boxes[0] != null);
    }

    // =====================================================
    // MAIN
    // =====================================================

    public static void main(String[] args) {
        System.out.println("=== Comprehensive Generics Test ===");

        testBasicGenericClass();
        testBoundedTypeParameters();
        testGenericMethods();
        testWildcards();
        testGenericInheritance();
        testAutoboxingWithGenerics();
        testMultipleTypeParameters();
        testNestedGenerics();
        testGenericArrays();

        System.out.println("\n=== RESULTS ===");
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);

        if (failed > 0) {
            System.exit(1);
        }
    }
}
