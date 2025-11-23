// Test array covariance in Java
// Arrays are covariant: if S <: T, then S[] <: T[]
public class ArrayCovarianceTest {
    public static void main(String[] args) {
        System.out.println("Testing array covariance...");

        // Test 1: String[] is assignable to Object[]
        String[] strings = new String[3];
        strings[0] = "Hello";
        strings[1] = "World";
        strings[2] = "!";

        Object[] objects = strings;  // Covariant assignment
        System.out.println("✓ String[] assignable to Object[]");

        // Test 2: Reading from covariant array
        Object obj = objects[0];
        if (obj instanceof String) {
            System.out.println("✓ Can read from covariant array: " + obj);
        } else {
            System.out.println("✗ FAIL: Object read is not String");
            System.exit(1);
        }

        // Test 3: Array type checking at compile time
        TestChild[] children = new TestChild[2];
        children[0] = new TestChild(1);
        children[1] = new TestChild(2);

        TestParent[] parents = children;  // Covariant assignment
        System.out.println("✓ TestChild[] assignable to TestParent[]");

        // Test 4: Reading works
        TestParent p = parents[0];
        if (p instanceof TestChild) {
            System.out.println("✓ Can read TestChild as TestParent: value=" + p.value);
        } else {
            System.out.println("✗ FAIL: TestParent is not TestChild");
            System.exit(1);
        }

        // Test 5: Multi-dimensional array covariance
        String[][] strings2D = new String[2][2];
        Object[][] objects2D = strings2D;  // Covariant assignment
        System.out.println("✓ String[][] assignable to Object[][]");

        System.out.println("✓ All array covariance tests passed!");
    }
}

class TestParent {
    int value;
    TestParent(int v) { value = v; }
}

class TestChild extends TestParent {
    TestChild(int v) { super(v); }
}
