// Test generic method type inference from implemented interfaces

import java.security.AccessController;
import java.security.PrivilegedAction;

// Top-level class that implements PrivilegedAction<String>
class StringAction implements PrivilegedAction<String> {
    private String value;

    StringAction(String value) {
        this.value = value;
    }

    public String run() {
        return value;
    }
}

public class InterfaceTypeArgTest {
    public static void main(String[] args) {
        // Test with a variable
        StringAction stringAction = new StringAction("hello");
        String s1 = AccessController.doPrivileged(stringAction);

        // Test with inline new expression
        String s2 = AccessController.doPrivileged(new StringAction("world"));

        if ("hello".equals(s1) && "world".equals(s2)) {
            System.out.println("InterfaceTypeArgTest: PASS");
        } else {
            System.out.println("InterfaceTypeArgTest: FAIL");
        }
    }
}
