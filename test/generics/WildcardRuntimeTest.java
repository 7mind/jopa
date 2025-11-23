// Runtime test for wildcard types
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

public class WildcardRuntimeTest {
    // Test unbounded wildcard
    static void printBox(Box<?> box) {
        Object value = box.get();
        System.out.println("Box contains: " + value);
    }

    // Test upper bounded wildcard
    static void printNumberBox(Box<? extends TestNumber> box) {
        TestNumber value = box.get();
        System.out.println("Number box: " + value);
    }

    public static void main(String[] args) {
        Box<String> stringBox = new Box<String>("Hello");
        Box<TestInteger> intBox = new Box<TestInteger>(new TestInteger(42));

        printBox(stringBox);
        printBox(intBox);
        printNumberBox(intBox);

        System.out.println("Wildcard tests passed!");
    }
}

class TestNumber {
    int value;

    TestNumber() {
        value = 0;
    }

    public String toString() {
        return "Number(" + value + ")";
    }
}

class TestInteger extends TestNumber {
    TestInteger(int v) {
        value = v;
    }

    public String toString() {
        return "" + value;
    }
}
