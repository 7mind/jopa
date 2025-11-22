// Test generics with runtime execution
public class GenericTest {
    public static void main(String[] args) {
        // Test simple generic box
        GenericBox box = new GenericBox();
        box.set("hello");
        Object value = box.get();
        if (value != null && value.equals("hello")) {
            System.out.println("✓ Generic box works");
        } else {
            System.out.println("✗ FAIL: Generic box");
            System.exit(1);
        }

        // Test with null
        GenericBox nullBox = new GenericBox();
        nullBox.set(null);
        Object nullValue = nullBox.get();
        if (nullValue == null) {
            System.out.println("✓ Generic box with null works");
        } else {
            System.out.println("✗ FAIL: Generic box with null");
            System.exit(1);
        }

        // Test type erasure (all types work at runtime)
        GenericBox intBox = new GenericBox();
        intBox.set(new Integer(42));
        Object intValue = intBox.get();
        if (intValue != null) {
            System.out.println("✓ Generic box with Integer works");
        } else {
            System.out.println("✗ FAIL: Generic box with Integer");
            System.exit(1);
        }

        System.out.println("✓ All generic tests passed!");
    }
}
