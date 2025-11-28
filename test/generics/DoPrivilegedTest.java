import java.security.AccessController;
import java.security.PrivilegedAction;

public class DoPrivilegedTest {
    public static void main(String[] args) {
        // Test that doPrivileged returns the type from the action, not Object
        String result = AccessController.doPrivileged(new PrivilegedAction<String>() {
            public String run() {
                return "test";
            }
        });

        if ("test".equals(result)) {
            System.out.println("DoPrivilegedTest: PASS");
        } else {
            System.out.println("DoPrivilegedTest: FAIL");
        }
    }
}
