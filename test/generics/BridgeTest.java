// Test generic class inheritance (bridge method generation is limited)

class BridgeBase<T> {
    T value;

    public T getValue() {
        return value;
    }

    public void setValue(T value) {
        this.value = value;
    }
}

class BridgeStringBox extends BridgeBase<String> {
    // Inherits getValue() and setValue() from base with String type argument
}

public class BridgeTest {
    public static void main(String[] args) {
        BridgeStringBox box = new BridgeStringBox();
        box.setValue("hello");

        // Get value through the concrete type
        Object s = box.getValue();
        if (s == null) {
            System.out.println("FAIL: getValue returned null");
            System.exit(1);
        }

        // Verify the value is correct
        BridgeBase<String> base = box;
        Object o = base.getValue();
        if (o == null) {
            System.out.println("FAIL: base getValue returned null");
            System.exit(1);
        }

        System.out.println("PASS: BridgeTest works");
    }
}
