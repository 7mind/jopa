// Comprehensive reflection test
// NOTE: Primitive class literals (int.class, etc.) cause compiler crash - bug to fix

public class ReflectionComprehensiveTest {
    static int passed = 0;
    static int failed = 0;

    static void test(String name, boolean condition) {
        if (condition) {
            passed++;
            System.out.println("PASS: " + name);
        } else {
            failed++;
            System.out.println("FAIL: " + name);
        }
    }

    // Test fields
    private int privateInt = 42;
    protected String protectedString = "protected";
    public double publicDouble = 3.14;

    // Test methods
    public int getPrivateInt() {
        return privateInt;
    }

    public void setPrivateInt(int value) {
        this.privateInt = value;
    }

    public String sayHello(String name) {
        return "Hello, " + name;
    }

    public static void main(String[] args) throws Exception {
        System.out.println("=== Comprehensive Reflection Test ===");

        Class cls = ReflectionComprehensiveTest.class;
        ReflectionComprehensiveTest obj = new ReflectionComprehensiveTest();

        // Class info tests
        test("1.1 Class.getName()", cls.getName().equals("ReflectionComprehensiveTest"));
        test("1.2 Class.getSimpleName()", cls.getSimpleName().equals("ReflectionComprehensiveTest"));

        Class superClass = cls.getSuperclass();
        test("1.3 Class.getSuperclass()", superClass.getName().equals("java.lang.Object"));

        test("1.4 Class.isInterface()", !cls.isInterface());
        test("1.5 Class.isArray()", !cls.isArray());

        Object[] arr = new Object[1];
        test("1.6 Array class isArray()", arr.getClass().isArray());

        // Field reflection tests
        java.lang.reflect.Field[] fields = cls.getDeclaredFields();
        test("2.1 getDeclaredFields() returns fields", fields.length >= 3);

        java.lang.reflect.Field pubField = cls.getField("publicDouble");
        test("2.2 getField(publicDouble) works", pubField != null);

        Object value = pubField.get(obj);
        test("2.3 Field.get() on public", value != null);

        java.lang.reflect.Field privField = cls.getDeclaredField("privateInt");
        test("2.4 getDeclaredField(privateInt)", privField != null);

        Class fieldType = pubField.getType();
        test("2.5 Field.getType()", fieldType.getName().equals("double"));

        // Method reflection tests
        java.lang.reflect.Method[] methods = cls.getDeclaredMethods();
        test("3.1 getDeclaredMethods() returns methods", methods.length >= 4);

        // Test with String parameter (no primitive class literal needed)
        Class[] stringArg = new Class[1];
        stringArg[0] = String.class;
        java.lang.reflect.Method sayMethod = cls.getMethod("sayHello", stringArg);
        test("3.2 getMethod(sayHello, String)", sayMethod != null);

        test("3.3 Method.getName()", sayMethod.getName().equals("sayHello"));

        Class returnType = sayMethod.getReturnType();
        test("3.4 Method.getReturnType()", returnType.getName().equals("java.lang.String"));

        Class[] paramTypes = sayMethod.getParameterTypes();
        test("3.5 Method.getParameterTypes()", paramTypes.length == 1);

        // Method.invoke test with String parameter
        Object[] invokeArgs = new Object[1];
        invokeArgs[0] = "World";
        Object result = sayMethod.invoke(obj, invokeArgs);
        test("3.6 Method.invoke()", "Hello, World".equals(result));

        // Constructor reflection tests
        java.lang.reflect.Constructor[] constructors = cls.getDeclaredConstructors();
        test("4.1 getDeclaredConstructors()", constructors.length >= 1);

        java.lang.reflect.Constructor defaultCtor = cls.getConstructor(new Class[0]);
        test("4.2 getConstructor()", defaultCtor != null);

        Object instance1 = defaultCtor.newInstance(new Object[0]);
        test("4.3 Constructor.newInstance()", instance1 != null);

        // Modifier tests
        int classMods = cls.getModifiers();
        test("5.1 Class is public", java.lang.reflect.Modifier.isPublic(classMods));

        int privMods = privField.getModifiers();
        test("5.2 Field is private", java.lang.reflect.Modifier.isPrivate(privMods));

        System.out.println("\n=== RESULTS ===");
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);

        if (failed > 0) {
            System.exit(1);
        }
    }
}
