// Test varargs method resolution with multiple arguments and autoboxing
public class VarargsMethodTest {
    // Simple varargs method accepting Objects
    public static void log(String format, Object... args) {
        System.out.println("log: " + format + " (" + args.length + " args)");
    }

    // Varargs method with Level parameter before varargs
    public static void logv(int level, String format, Object... args) {
        System.out.println("logv: level=" + level + ", format=" + format + " (" + args.length + " args)");
    }

    public static void main(String[] args) {
        // Basic varargs call with String only
        log("test {0}", "hello");

        // Multiple args including wrapper Integer
        log("test {0} {1}", "hello", Integer.valueOf(42));

        // Multiple mixed args
        log("wrap {0} {1} {2}", "a", Integer.valueOf(1), "c");

        // Varargs with primitive int autoboxing to Object (using variable)
        int x = 42;
        log("value is {0}", x);

        // Multiple primitive args autoboxed (using variables)
        int a = 10, b = 20, c = 30;
        logv(1, "values: {0} {1} {2}", a, b, c);

        // Mixed primitives and objects in varargs
        int d = 100;
        logv(2, "mixed: {0} {1} {2} {3}", "str", d, "end", Integer.valueOf(999));

        System.out.println("Varargs method test passed!");
    }
}
