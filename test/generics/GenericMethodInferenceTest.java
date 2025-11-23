// Test generic method type inference
class Utils {
    // Generic method with type parameter
    public static <T> T identity(T arg) {
        return arg;
    }

    public static <T> T first(T a, T b) {
        return a;
    }
}

public class GenericMethodInferenceTest {
    public static void main(String[] args) {
        // Should infer <String>
        String s = Utils.identity("hello");

        // Should infer <TestInteger>
        TestInteger i = Utils.identity(42);

        // Should infer <String>
        String s2 = Utils.first("hello", "world");

        System.out.println("Generic method inference works!");
    }
}

class String {
    // Stub for compilation
}

class TestInteger {
    // Stub for compilation
}
