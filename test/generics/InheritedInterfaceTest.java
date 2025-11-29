// Test for inherited interface method with generic return type
import java.util.Enumeration;

interface MyEnumeration<E> extends Enumeration<E> {
    boolean hasMore();
    E next();
}

class MyAttribute {
    public String getID() { return "test"; }
}

class MyAttrs implements MyEnumeration<MyAttribute> {
    private MyAttribute attr = new MyAttribute();
    public boolean hasMore() { return true; }
    public MyAttribute next() { return attr; }
    public boolean hasMoreElements() { return true; }
    public MyAttribute nextElement() { return attr; }
}

public class InheritedInterfaceTest {
    MyEnumeration<MyAttribute> getAll() {
        return new MyAttrs();
    }

    public String getFirst() {
        MyEnumeration<MyAttribute> attrs = getAll();
        return attrs.next().getID(); // next() returns MyAttribute
    }

    public String getFirstViaInherited() {
        MyEnumeration<MyAttribute> attrs = getAll();
        return attrs.nextElement().getID(); // nextElement() returns MyAttribute via Enumeration
    }

    public static void main(String[] args) {
        InheritedInterfaceTest t = new InheritedInterfaceTest();
        String r1 = t.getFirst();
        String r2 = t.getFirstViaInherited();
        if (!"test".equals(r1) || !"test".equals(r2)) {
            System.out.println("FAIL: expected test, got " + r1 + " and " + r2);
            System.exit(1);
        }
        System.out.println("PASS: InheritedInterfaceTest");
    }
}
