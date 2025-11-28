// Test generic interface defined in same file

interface Provider<T> {
    T get();
}

class StringProvider implements Provider<String> {
    public String get() {
        return "hello";
    }
}

public class LocalInterfaceTest {
    public static void main(String[] args) {
        Provider<String> p = new StringProvider();
        String s = p.get();  // Should return String
        System.out.println(s);
        System.out.println("LocalInterfaceTest: PASS");
    }
}
