// Test if generics work when both classes are in same file
class Box<T> {
    T value;
}

class StringBox extends Box<String> {
    // Now use type argument
}

class String {
    // Stub
}

public class SimpleSameFileTest {
    public static void main(String[] args) {
        System.out.println("Test");
    }
}
