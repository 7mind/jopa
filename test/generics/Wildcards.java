// Test wildcard types

class WildcardContainer<T> {
    T item;

    WildcardContainer(T item) {
        this.item = item;
    }

    T get() {
        return item;
    }
}

class WildcardTestNumber {
    int value;
    WildcardTestNumber() { value = 0; }
}

class WildcardTestInteger extends WildcardTestNumber {
    WildcardTestInteger(int v) {
        value = v;
    }
}

public class Wildcards {
    public static void main(String[] args) {
        // Unbounded wildcard: ? erases to Object
        WildcardContainer<?> c1 = new WildcardContainer<Object>(new Object());
        Object o1 = c1.get();
        if (o1 == null) {
            System.out.println("FAIL: unbounded wildcard get returned null");
            System.exit(1);
        }

        // Upper bounded wildcard: ? extends WildcardTestNumber erases to WildcardTestNumber
        WildcardContainer<? extends WildcardTestNumber> c2 = new WildcardContainer<WildcardTestNumber>(new WildcardTestInteger(42));
        WildcardTestNumber n = c2.get();
        if (n == null) {
            System.out.println("FAIL: upper bounded wildcard get returned null");
            System.exit(1);
        }

        // Lower bounded wildcard: ? super WildcardTestInteger erases to Object
        WildcardContainer<? super WildcardTestInteger> c3 = new WildcardContainer<Object>(new Object());
        Object o3 = c3.get();
        if (o3 == null) {
            System.out.println("FAIL: lower bounded wildcard get returned null");
            System.exit(1);
        }

        System.out.println("PASS: Wildcards work");
    }
}
