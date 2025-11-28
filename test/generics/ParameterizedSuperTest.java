// Test: class extends parameterized type
// e.g., Entry extends WeakReference<ThreadLocal<?>>

class Container<T> {
    T value;
    public T get() { return value; }
}

class StringContainer extends Container<String> {
    public void test() {
        String s = get();  // Should work - get() returns String
    }
}

class GenericExtender<E> extends Container<E> {
    public void test() {
        E e = get();  // Should work - get() returns E
    }
}

public class ParameterizedSuperTest {
    public static void main(String[] args) {
        StringContainer sc = new StringContainer();
        String s = sc.get();  // Should return String

        GenericExtender<Integer> ge = new GenericExtender<Integer>();
        Integer i = ge.get();  // Should return Integer

        System.out.println("ParameterizedSuperTest: PASS");
    }
}
