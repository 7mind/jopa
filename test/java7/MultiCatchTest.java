// Test for Java 7 multi-catch
public class MultiCatchTest {
    static class ExceptionA extends Exception {}
    static class ExceptionB extends Exception {}

    static void throwA() throws ExceptionA {
        throw new ExceptionA();
    }

    static void throwB() throws ExceptionB {
        throw new ExceptionB();
    }

    public static void main(String[] args) {
        int count = 0;

        // Test multi-catch
        try {
            throwA();
        } catch (ExceptionA | ExceptionB e) {
            count++;
        }

        try {
            throwB();
        } catch (ExceptionA | ExceptionB e) {
            count++;
        }

        if (count == 2) {
            System.out.println("Multi-catch test passed!");
        } else {
            System.out.println("FAIL: expected 2 catches, got " + count);
            System.exit(1);
        }
    }
}
