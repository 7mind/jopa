// Test for captured local variable with generic type in anonymous class
import java.util.Iterator;
import java.util.ArrayList;

interface MyEnum<E> {
    E next();
    boolean hasMore();
}

class MyClass {
    public String getID() { return "test"; }
}

public class CapturedIteratorTest {
    MyEnum<MyClass> getAll() {
        final ArrayList<MyClass> list = new ArrayList<MyClass>();
        list.add(new MyClass());
        final Iterator<MyClass> iter = list.iterator();
        return new MyEnum<MyClass>() {
            public MyClass next() { return iter.next(); }
            public boolean hasMore() { return iter.hasNext(); }
        };
    }

    MyEnum<String> getIDs() {
        final MyEnum<MyClass> attrs = getAll();
        return new MyEnum<String>() {
            public boolean hasMore() {
                return attrs.hasMore();
            }
            public String next() {
                return attrs.next().getID(); // next() returns MyClass
            }
        };
    }

    public static void main(String[] args) {
        CapturedIteratorTest t = new CapturedIteratorTest();
        String result = t.getIDs().next();
        if (!"test".equals(result)) {
            System.out.println("FAIL: expected test, got " + result);
            System.exit(1);
        }
        System.out.println("PASS: CapturedIteratorTest");
    }
}
