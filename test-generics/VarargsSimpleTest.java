// Simple varargs test with explicit array

public class VarargsSimpleTest {
    public void printStrings(String... args) {
        // Varargs method
    }

    public void test() {
        // Test with explicit array - should work
        String[] arr = {"a", "b"};
        printStrings(arr);
    }
}
