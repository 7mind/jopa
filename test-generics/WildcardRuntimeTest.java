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
    static void printNumberBox(Box<? extends Number> box) {
        Number value = box.get();
        System.out.println("Number box: " + value);
    }

    public static void main(String[] args) {
        Box<String> stringBox = new Box<String>("Hello");
        Box<Integer> intBox = new Box<Integer>(new Integer(42));

        printBox(stringBox);
        printBox(intBox);
        printNumberBox(intBox);

        System.out.println("Wildcard tests passed!");
    }
}

class Number {
    int value;

    Number() {
        value = 0;
    }

    public String toString() {
        return "Number(" + value + ")";
    }
}

class Integer extends Number {
    Integer(int v) {
        value = v;
    }

    public String toString() {
        return "" + value;
    }
}
