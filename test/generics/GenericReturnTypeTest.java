// Test generic method return type inference from argument type
public class GenericReturnTypeTest {
    // Generic method that returns the argument's type
    public static <T> T identity(T arg) {
        return arg;
    }

    // Generic method that returns array of type parameter
    public static <T> T[] toArray(T elem) {
        return null;  // Just testing type inference
    }

    // Generic method that infers return type from Class argument
    public static <T> T getInstance(Class<T> clazz) {
        return null;
    }

    public static void main(String[] args) {
        // Should infer T=String from String argument, return type should be String
        String s1 = identity("hello");

        // Should infer T=String from String argument, return type should be String[]
        String[] arr = toArray("hello");

        // Should infer T=String from Class<String> argument
        String str = getInstance(String.class);

        System.out.println("Generic return type inference test passed!");
    }
}
