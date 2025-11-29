// Test for chained generic method calls
import java.util.HashMap;
import java.util.Map;

public class ChainedGenericsTest {
    private Map<String, Map<String, Integer>> nestedMap = new HashMap<String, Map<String, Integer>>();

    public Integer getValue(String outer, String inner) {
        // This chains two .get() calls
        // First get() returns Map<String, Integer>
        // Second get() should return Integer
        return nestedMap.get(outer).get(inner);
    }

    public static void main(String[] args) {
        ChainedGenericsTest t = new ChainedGenericsTest();
        t.nestedMap.put("a", new HashMap<String, Integer>());
        t.nestedMap.get("a").put("b", 42);
        Integer result = t.getValue("a", "b");
        if (result == null || result.intValue() != 42) {
            System.out.println("FAIL: expected 42, got " + result);
            System.exit(1);
        }
        System.out.println("PASS: ChainedGenericsTest");
    }
}
