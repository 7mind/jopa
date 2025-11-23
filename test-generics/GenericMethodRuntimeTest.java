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
    static <T extends Number> T firstNumber(T a, T b) {
        return a;
    }

    // Instance generic method
    <E> E process(E element) {
        return element;
    }

    public static void main(String[] args) {
        // Test static generic methods
        String s = identity("hello");
        Integer i = identity(new Integer(42));

        String key = getKey("key", "value");
        Integer num = getKey(new Integer(1), "value");

        Integer first = firstNumber(new Integer(10), new Integer(20));

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

class Number {
    int value;
    public String toString() { return "Number"; }
}

class Integer extends Number {
    Integer(int v) { value = v; }
    public String toString() { return "" + value; }
}
