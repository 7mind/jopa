// Test generic methods

class GMContainer<T> {
    T value;

    GMContainer(T v) {
        value = v;
    }

    T get() {
        return value;
    }
}

class GMTestNumber {
    int value;
    GMTestNumber() { value = 0; }
}

class GenericMethodHelper {
    // Generic method with single type parameter (erases to Object)
    static <T> T identity(T arg) {
        return arg;
    }

    // Generic method with multiple type parameters
    static <K, V> GMContainer<K> firstOfPair(K key, V value) {
        return new GMContainer<K>(key);
    }

    // Generic method with bounded type parameter (erases to GMTestNumber)
    static <T extends GMTestNumber> T max(T a, T b) {
        return a;  // Simplified - would need actual comparison
    }

    // Instance generic method
    <E> E processElement(E element) {
        return element;
    }
}

public class GenericMethods {
    public static void main(String[] args) {
        // Call generic methods
        Object obj = GenericMethodHelper.identity(new Object());
        if (obj == null) {
            System.out.println("FAIL: identity returned null");
            System.exit(1);
        }

        GMContainer<Object> c = GenericMethodHelper.firstOfPair(new Object(), new Object());
        if (c == null || c.get() == null) {
            System.out.println("FAIL: firstOfPair returned null");
            System.exit(1);
        }

        // Test bounded generic method
        GMTestNumber tn = new GMTestNumber();
        GMTestNumber result = GenericMethodHelper.max(tn, tn);
        if (result == null) {
            System.out.println("FAIL: max returned null");
            System.exit(1);
        }

        // Test instance generic method
        GenericMethodHelper gmt = new GenericMethodHelper();
        Object processed = gmt.processElement(new Object());
        if (processed == null) {
            System.out.println("FAIL: processElement returned null");
            System.exit(1);
        }

        System.out.println("PASS: GenericMethods work");
    }
}
