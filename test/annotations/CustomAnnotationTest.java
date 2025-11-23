// Test custom annotation type declarations

// Simple marker annotation
@interface MyMarker {
}

// Annotation with single element
@interface SingleValue {
    String value();
}

// Annotation with multiple elements
@interface MultiValue {
    String name();
    int count();
}

// Annotation with default values
@interface WithDefaults {
    String value() default "default";
    int number() default 42;
}

public class CustomAnnotationTest {
    @MyMarker
    public void markerTest() {
        System.out.println("Method with marker annotation");
    }

    @SingleValue("test")
    public void singleValueTest() {
        System.out.println("Method with single value annotation");
    }

    @MultiValue(name = "test", count = 5)
    public void multiValueTest() {
        System.out.println("Method with multi-value annotation");
    }

    @WithDefaults
    public void defaultsTest1() {
        System.out.println("Method with default values");
    }

    @WithDefaults(value = "custom", number = 100)
    public void defaultsTest2() {
        System.out.println("Method with custom values");
    }

    public static void main(String[] args) {
        CustomAnnotationTest test = new CustomAnnotationTest();
        test.markerTest();
        test.singleValueTest();
        test.multiValueTest();
        test.defaultsTest1();
        test.defaultsTest2();
        System.out.println("✓ Custom annotation test passed");
    }
}
