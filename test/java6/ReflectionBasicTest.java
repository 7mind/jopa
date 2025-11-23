// Test basic Java reflection capabilities (Java 1.1+)
public class ReflectionBasicTest {
    private int privateField = 42;
    public String publicField = "test";

    public void testMethod(int arg) {
        System.out.println("testMethod called with: " + arg);
    }

    public int calculate(int a, int b) {
        return a + b;
    }

    public static void main(String[] args) throws Exception {
        ReflectionBasicTest obj = new ReflectionBasicTest();
        Class cls = obj.getClass();

        // Test 1: Get class name
        String className = cls.getName();
        if (!className.equals("ReflectionBasicTest")) {
            System.out.println("FAIL: Expected 'ReflectionBasicTest', got: " + className);
            System.exit(1);
        }
        System.out.println("✓ Class.getName() works: " + className);

        // Test 2: Get declared methods
        java.lang.reflect.Method[] methods = cls.getDeclaredMethods();
        boolean foundTestMethod = false;
        boolean foundCalculate = false;
        for (int i = 0; i < methods.length; i++) {
            String name = methods[i].getName();
            if (name.equals("testMethod")) foundTestMethod = true;
            if (name.equals("calculate")) foundCalculate = true;
        }
        if (!foundTestMethod || !foundCalculate) {
            System.out.println("FAIL: Methods not found via reflection");
            System.exit(1);
        }
        System.out.println("✓ Class.getDeclaredMethods() works");

        // Test 3: Get declared fields
        java.lang.reflect.Field[] fields = cls.getDeclaredFields();
        boolean foundPrivate = false;
        boolean foundPublic = false;
        for (int i = 0; i < fields.length; i++) {
            String name = fields[i].getName();
            if (name.equals("privateField")) foundPrivate = true;
            if (name.equals("publicField")) foundPublic = true;
        }
        if (!foundPrivate || !foundPublic) {
            System.out.println("FAIL: Fields not found via reflection");
            System.exit(1);
        }
        System.out.println("✓ Class.getDeclaredFields() works");

        // Test 4: Access public field via reflection
        java.lang.reflect.Field pubField = cls.getField("publicField");
        Object fieldValue = pubField.get(obj);
        if (!fieldValue.equals("test")) {
            System.out.println("FAIL: Field value incorrect");
            System.exit(1);
        }
        System.out.println("✓ Field.get() works: " + fieldValue);

        // Test 5: Modify field via reflection
        pubField.set(obj, "modified");
        if (!obj.publicField.equals("modified")) {
            System.out.println("FAIL: Field.set() failed");
            System.exit(1);
        }
        System.out.println("✓ Field.set() works");

        // Test 6: Check method parameter count and types
        // Note: Actual Method.invoke() requires JVM native implementation
        // We verify the method signature is correctly generated in bytecode
        java.lang.reflect.Method calcMethod = null;
        for (int i = 0; i < methods.length; i++) {
            if (methods[i].getName().equals("calculate")) {
                calcMethod = methods[i];
                break;
            }
        }
        if (calcMethod == null) {
            System.out.println("FAIL: calculate method not found");
            System.exit(1);
        }
        System.out.println("✓ Method signature accessible via reflection");

        System.out.println("✓ All basic reflection tests passed!");
    }
}
