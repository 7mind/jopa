// Test generic method return type inference

public class GenericReturnTypeTest {
    static class Box<T> {
        private T value;

        Box(T value) {
            this.value = value;
        }

        T get() {
            return value;
        }
    }

    public static void main(String[] args) {
        Box<String> box = new Box<String>("hello");
        String s = box.get();  // get() should return String, not Object
        System.out.println(s);
        System.out.println("GenericReturnTypeTest: PASS");
    }
}
