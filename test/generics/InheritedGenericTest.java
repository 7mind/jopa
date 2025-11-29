// Test for inherited generic method return type with concrete parent params
import java.util.HashMap;

public class InheritedGenericTest {
    // Subclass of HashMap - does NOT add its own type parameters
    static class MyMap extends HashMap<String, Integer> {
        public MyMap() { super(); }
    }

    private MyMap map = new MyMap();

    public Integer getValue(String key) {
        return map.get(key); // Should return Integer, not Object
    }

    public static void main(String[] args) {
        InheritedGenericTest t = new InheritedGenericTest();
        t.map.put("test", 42);
        Integer result = t.getValue("test");
        if (result == null || result.intValue() != 42) {
            System.out.println("FAIL: expected 42, got " + result);
            System.exit(1);
        }
        System.out.println("PASS: InheritedGenericTest");
    }
}
