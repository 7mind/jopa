// Test interface from JAR generic return type

import java.util.List;
import java.util.ArrayList;

public class JarInterfaceTest {
    public static void main(String[] args) {
        List<String> list = new ArrayList<String>();
        list.add("hello");
        String s = list.get(0);  // Should return String
        System.out.println(s);
        System.out.println("JarInterfaceTest: PASS");
    }
}
