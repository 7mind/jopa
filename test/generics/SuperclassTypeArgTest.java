// Test generic method return type inference from parameterized superclass

// A simple generic base class
class Box<T> {
    private T value;
    Box(T value) { this.value = value; }
    T get() { return value; }
}

// A class extending a parameterized generic type
class StringBox extends Box<String> {
    StringBox(String s) {
        super(s);
    }
}

public class SuperclassTypeArgTest {
    public static void main(String[] args) {
        StringBox box = new StringBox("hello");
        // get() should return String, not Object
        String s = box.get();
        System.out.println("Got: " + s);
        System.out.println("SuperclassTypeArgTest: PASS");
    }
}
