// Test enhanced for-loop with Iterable collections
import java.util.ArrayList;

public class ForEachIterableTest {
    public static void main(String[] args) {
        // Create ArrayList with Integer objects
        ArrayList list = new ArrayList();
        list.add(new Integer(1));
        list.add(new Integer(2));
        list.add(new Integer(3));

        // Test enhanced for-loop with Iterable
        int sum = 0;
        for (Object obj : list) {
            Integer i = (Integer) obj;
            sum = sum + i.intValue();
        }

        if (sum == 6) {
            // Test passed
        } else {
            throw new RuntimeException("Enhanced for-loop failed: sum=" + sum);
        }
    }
}
