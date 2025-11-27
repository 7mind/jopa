// Minimal reflection test

public class ReflectionMinimalTest {
    private int privateInt = 42;

    public static void main(String[] args) throws Exception {
        Class cls = ReflectionMinimalTest.class;
        System.out.println("Name: " + cls.getName());

        // Test Method[]
        java.lang.reflect.Method[] methods = cls.getDeclaredMethods();
        System.out.println("Methods: " + methods.length);

        // Test Field[]
        java.lang.reflect.Field[] fields = cls.getDeclaredFields();
        System.out.println("Fields: " + fields.length);

        // Test Constructor[]
        java.lang.reflect.Constructor[] constructors = cls.getDeclaredConstructors();
        System.out.println("Constructors: " + constructors.length);

        // Test getField
        java.lang.reflect.Field f = cls.getDeclaredField("privateInt");
        System.out.println("Field: " + f.getName());

        // Test Modifier
        int mods = f.getModifiers();
        boolean isPrivate = java.lang.reflect.Modifier.isPrivate(mods);
        System.out.println("isPrivate: " + isPrivate);

        System.out.println("PASS");
    }
}
