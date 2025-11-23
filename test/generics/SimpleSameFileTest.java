// Test if generics work when both classes are in same file
class Box<T> {
    T value;
}

class StringBox extends Box<TestString> {
    // Now use type argument
}

class TestString {
    // Stub
}

public class SimpleSameFileTest {
    public static void main(String[] args) {
        System.out.println("Test");
    }
}
