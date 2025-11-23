// Test runtime retention annotations and reflection access

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.annotation.ElementType;

// Runtime-retained annotation with values
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.TYPE, ElementType.METHOD})
@interface TestInfo {
    String value();
    int priority() default 0;
}

// Runtime-retained marker annotation
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
@interface RuntimeMarker {
}

// Class-retained annotation (should NOT be visible at runtime)
@Retention(RetentionPolicy.CLASS)
@Target(ElementType.METHOD)
@interface CompileTimeOnly {
}

@TestInfo(value = "Main test class", priority = 1)
public class RuntimeRetentionTest {

    @TestInfo(value = "High priority method", priority = 10)
    @RuntimeMarker
    public void annotatedMethod() {
        System.out.println("Annotated method called");
    }

    @CompileTimeOnly
    public void compileTimeMethod() {
        System.out.println("Compile-time annotation (not visible at runtime)");
    }

    public void unannotatedMethod() {
        System.out.println("Unannotated method");
    }

    public static void main(String[] args) {
        try {
            Class<?> clazz = RuntimeRetentionTest.class;

            // Test 1: Class-level annotation
            System.out.println("=== Test 1: Class-level annotations ===");
            if (clazz.isAnnotationPresent(TestInfo.class)) {
                TestInfo classInfo = clazz.getAnnotation(TestInfo.class);
                System.out.println("Class annotation found:");
                System.out.println("  value = " + classInfo.value());
                System.out.println("  priority = " + classInfo.priority());
                if (!classInfo.value().equals("Main test class")) {
                    System.err.println("ERROR: Wrong class annotation value");
                    System.exit(1);
                }
                if (classInfo.priority() != 1) {
                    System.err.println("ERROR: Wrong class annotation priority");
                    System.exit(1);
                }
            } else {
                System.err.println("ERROR: Class annotation not found!");
                System.exit(1);
            }

            // Test 2: Method-level annotations
            System.out.println("\n=== Test 2: Method-level annotations ===");
            java.lang.reflect.Method annotatedMethod = clazz.getMethod("annotatedMethod");

            if (annotatedMethod.isAnnotationPresent(TestInfo.class)) {
                TestInfo methodInfo = annotatedMethod.getAnnotation(TestInfo.class);
                System.out.println("Method annotation found:");
                System.out.println("  value = " + methodInfo.value());
                System.out.println("  priority = " + methodInfo.priority());
                if (!methodInfo.value().equals("High priority method")) {
                    System.err.println("ERROR: Wrong method annotation value");
                    System.exit(1);
                }
                if (methodInfo.priority() != 10) {
                    System.err.println("ERROR: Wrong method annotation priority");
                    System.exit(1);
                }
            } else {
                System.err.println("ERROR: Method annotation not found!");
                System.exit(1);
            }

            // Test 3: Multiple annotations on same element
            System.out.println("\n=== Test 3: Multiple annotations ===");
            if (annotatedMethod.isAnnotationPresent(RuntimeMarker.class)) {
                System.out.println("RuntimeMarker annotation found on method");
            } else {
                System.err.println("ERROR: RuntimeMarker annotation not found!");
                System.exit(1);
            }

            // Test 4: CLASS retention should NOT be visible at runtime
            System.out.println("\n=== Test 4: CLASS retention (should be invisible) ===");
            java.lang.reflect.Method compileTimeMethod = clazz.getMethod("compileTimeMethod");
            if (compileTimeMethod.isAnnotationPresent(CompileTimeOnly.class)) {
                System.err.println("ERROR: CLASS-retained annotation visible at runtime!");
                System.exit(1);
            } else {
                System.out.println("CLASS-retained annotation correctly invisible at runtime");
            }

            // Test 5: Unannotated method
            System.out.println("\n=== Test 5: Unannotated method ===");
            java.lang.reflect.Method unannotatedMethod = clazz.getMethod("unannotatedMethod");
            if (unannotatedMethod.isAnnotationPresent(TestInfo.class)) {
                System.err.println("ERROR: Unannotated method shows annotation!");
                System.exit(1);
            } else {
                System.out.println("Unannotated method correctly has no annotations");
            }

            System.out.println("\n✓ All runtime retention tests passed");

        } catch (Exception e) {
            System.err.println("ERROR: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }
}
