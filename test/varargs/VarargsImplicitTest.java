// Test varargs with implicit array creation

public class VarargsImplicitTest {
    public void printStrings(String... args) {
        // Varargs method
    }

    public void test() {
        // These should be automatically wrapped in an array
        printStrings("a", "b", "c");
        printStrings("single");
        printStrings();  // Empty varargs
    }
}
