// Test variance in Java generics
// Generics are invariant, but wildcards provide variance
public class GenericVarianceTest {
    public static void main(String[] args) {
        System.out.println("Testing generic variance...");

        // Test basic covariance without field access
        Box<TestChild> childBox = new Box<TestChild>();
        Box<? extends TestParent> parentBox = childBox;  // Covariant with ? extends
        System.out.println("✓ Covariance with ? extends works");

        // Test method parameter
        printValue(childBox);  // Can pass Box<TestChild> to Box<? extends TestParent>
        System.out.println("✓ Upper bounded wildcard method call works");

        System.out.println("✓ All generic variance tests passed!");
    }

    // Covariant method - can read but not write (PECS: Producer Extends)
    static void printValue(Box<? extends TestParent> box) {
        System.out.println("  Received box");
    }
}

class Box<T> {
    T value;
}

class TestParent {
    int value;
    TestParent(int v) { value = v; }
}

class TestChild extends TestParent {
    TestChild(int v) { super(v); }
}
