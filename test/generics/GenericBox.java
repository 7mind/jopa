// Simple generic class for testing (erased to Object)
public class GenericBox {
    private Object value;

    public void set(Object v) {
        value = v;
    }

    public Object get() {
        return value;
    }

    public static void main(String[] args) {
        GenericBox box = new GenericBox();
        box.set("hello");
        Object result = box.get();
        if (result == null) {
            System.out.println("FAIL: get returned null");
            System.exit(1);
        }
        System.out.println("PASS: GenericBox works");
    }
}
