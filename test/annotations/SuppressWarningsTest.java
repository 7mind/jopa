// Test @SuppressWarnings annotation
public class SuppressWarningsTest {
    // Single warning suppression
    @SuppressWarnings("unchecked")
    public void testUnchecked() {
        System.out.println("Method with @SuppressWarnings(\"unchecked\")");
    }

    // Multiple warnings suppression (using array)
    @SuppressWarnings({"unchecked", "deprecation"})
    public void testMultiple() {
        System.out.println("Method with multiple warnings suppressed");
    }

    // Class-level annotation
    @SuppressWarnings("all")
    static class InnerClass {
        public void test() {
            System.out.println("Class with @SuppressWarnings");
        }
    }

    public static void main(String[] args) {
        SuppressWarningsTest test = new SuppressWarningsTest();
        test.testUnchecked();
        test.testMultiple();

        InnerClass inner = new InnerClass();
        inner.test();

        System.out.println("✓ @SuppressWarnings test passed");
    }
}
