// Test generic methods
public class GenericMethodRuntimeTest {
    // Generic method with single type parameter
    static <T> T identity(T arg) {
        return arg;
    }

    // Generic method with multiple type parameters
    static <K, V> K getKey(K key, V value) {
        return key;
    }

    // Generic method with bounded type parameter
    static <T extends TestNumber> T firstNumber(T a, T b) {
        return a;
    }

    // Instance generic method
    <E> E process(E element) {
        return element;
    }

    public static void main(String[] args) {
        // Test static generic methods
        String s = identity("hello");
        TestInteger i = identity(new TestInteger(42));

        String key = getKey("key", "value");
        TestInteger num = getKey(new TestInteger(1), "value");

        TestInteger first = firstNumber(new TestInteger(10), new TestInteger(20));

        // Test instance generic method
        GenericMethodRuntimeTest test = new GenericMethodRuntimeTest();
        String result = test.process("test");

        System.out.println("Generic method tests passed!");
        System.out.println("s = " + s);
        System.out.println("i = " + i);
        System.out.println("key = " + key);
        System.out.println("num = " + num);
        System.out.println("first = " + first);
        System.out.println("result = " + result);
    }
}

class TestNumber {
    int value;
    public String toString() { return "Number"; }
}

class TestInteger extends TestNumber {
    TestInteger(int v) { value = v; }
    public String toString() { return "" + value; }
}
