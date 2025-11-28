import java.util.Map;
import java.util.HashMap;

public class ChainedGenericReturnType {
    // Test 1: Map.get() returning generic type V
    private Map<String, Integer> map = new HashMap<String, Integer>();
    
    public Integer getValue(String key) {
        return map.get(key);  // Should return Integer, not Object
    }
    
    // Test 2: Chained method call with generic return
    public Map<String, Integer> getMap() {
        return map;
    }
    
    public Integer getValueChained(String key) {
        return getMap().get(key);  // Chained call - should return Integer
    }
    
    public static void main(String[] args) {
        ChainedGenericReturnType test = new ChainedGenericReturnType();
        test.map.put("test", 42);
        Integer v1 = test.getValue("test");
        Integer v2 = test.getValueChained("test");
        System.out.println("v1=" + v1 + ", v2=" + v2);
    }
}
