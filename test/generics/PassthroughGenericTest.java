// Test for passthrough type parameters
import java.util.HashMap;

public class PassthroughGenericTest {
    // Subclass with type parameters that pass through to parent
    static class MyMap<A, B> extends HashMap<A, B> {
        public MyMap() { super(); }
    }

    private MyMap<String, Integer> map = new MyMap<String, Integer>();

    public Integer getValue(String key) {
        return map.get(key); // Should return Integer (B->Integer), not Object
    }

    public static void main(String[] args) {
        PassthroughGenericTest t = new PassthroughGenericTest();
        t.map.put("test", 42);
        Integer result = t.getValue("test");
        if (result == null || result.intValue() != 42) {
            System.out.println("FAIL: expected 42, got " + result);
            System.exit(1);
        }
        System.out.println("PASS: PassthroughGenericTest");
    }
}
