// Test marker annotations (annotations with no parameters)
public class MarkerAnnotationTest {
    // @Override is a marker annotation
    @Override
    public String toString() {
        return "MarkerAnnotationTest";
    }

    // @Deprecated is a marker annotation
    @Deprecated
    public void oldMethod() {
        System.out.println("This method is deprecated");
    }

    public static void main(String[] args) {
        MarkerAnnotationTest test = new MarkerAnnotationTest();
        System.out.println(test.toString());
        test.oldMethod();
        System.out.println("✓ Marker annotation test passed");
    }
}
