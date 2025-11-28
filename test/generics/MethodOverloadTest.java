// Test method overload resolution with boxing/unboxing

public class MethodOverloadTest {
    public static void print(Object obj) {
        System.out.println("Object version: " + obj);
    }

    public static void print(boolean b) {
        System.out.println("boolean version: " + b);
    }

    public static void print(int i) {
        System.out.println("int version: " + i);
    }

    public static void main(String[] args) {
        // Primitive calls - should use primitive versions
        print(true);   // should call print(boolean)
        print(42);     // should call print(int)

        // Boxed calls - should use Object version
        Boolean b = Boolean.TRUE;
        Integer i = Integer.valueOf(42);
        print(b);      // should call print(Object)
        print(i);      // should call print(Object)

        System.out.println("MethodOverloadTest: PASS");
    }
}
