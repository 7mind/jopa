public class RawGenericTest {
    public static void main(String[] args) {
        Box<String> box = new Box<String>("test");
        Object obj = box;

        // This crashes the compiler
        if (obj instanceof Box) {
            System.out.println("is a Box");
        }
    }
}

class Box<T> {
    private T value;
    public Box(T value) { this.value = value; }
    public T get() { return value; }
}
