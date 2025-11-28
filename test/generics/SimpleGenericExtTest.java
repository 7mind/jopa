// Minimal test for class extending parameterized type

class Box<T> {
    public T get() { return null; }
}

class IntBox extends Box<Integer> {
}

public class SimpleGenericExtTest {
    public static void main(String[] args) {
        IntBox box = new IntBox();
        Integer i = box.get();  // Should return Integer, not Object
        System.out.println("SimpleGenericExtTest: PASS");
    }
}
