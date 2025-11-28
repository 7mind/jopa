// Test: class extends parameterized type with wildcard (like GNU Classpath)

import java.lang.ref.WeakReference;

class Entry extends WeakReference<String> {
    public Entry(String referent) {
        super(referent);
    }
}

public class WeakRefTest {
    public static void main(String[] args) {
        Entry e = new Entry("hello");
        String s = e.get();  // Should return String
        System.out.println(s);
        System.out.println("WeakRefTest: PASS");
    }
}
