// Test type substitution for field access
class Box<T> {
    public T value;

    Box(T v) {
        value = v;
    }

    public T getValue() {
        return value;
    }
}

public class FieldAccessTest {
    public static void test(Box<String> stringBox) {
        // This should work: value should be String, not Object
        String s = stringBox.value;
    }

    public static void main(String[] args) {
        Box<String> box = new Box<String>("hello");
        test(box);
    }
}
