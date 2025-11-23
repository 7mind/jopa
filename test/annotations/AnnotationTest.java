// Test annotation support

public class AnnotationTest {
    @Override
    public String toString() {
        return "AnnotationTest";
    }

    @Deprecated
    public void oldMethod() {
        System.out.println("Deprecated method called");
    }

    public static void main(String[] args) {
        AnnotationTest test = new AnnotationTest();
        System.out.println(test.toString());
        test.oldMethod();
        System.out.println("✓ Annotation test passed");
    }
}
