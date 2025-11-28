// Test interface generic return type

interface Supplier<T> {
    T get();
}

public class InterfaceGenericTest {
    static class StringSupplier implements Supplier<String> {
        public String get() {
            return "hello";
        }
    }

    public static void main(String[] args) {
        Supplier<String> sup = new StringSupplier();
        String s = sup.get();  // Should return String
        System.out.println(s);
        System.out.println("InterfaceGenericTest: PASS");
    }
}
