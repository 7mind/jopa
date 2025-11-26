public class SimpleGeneric<T> {
    T value;

    public SimpleGeneric(T value) {
        this.value = value;
    }

    public T getValue() {
        return value;
    }

    public void setValue(T value) {
        this.value = value;
    }

    public static void main(String[] args) {
        SimpleGeneric<String> sg = new SimpleGeneric<String>("hello");
        Object result = sg.getValue();
        if (result == null) {
            System.out.println("FAIL: getValue returned null");
            System.exit(1);
        }
        sg.setValue("world");
        result = sg.getValue();
        if (result == null) {
            System.out.println("FAIL: setValue/getValue failed");
            System.exit(1);
        }
        System.out.println("PASS: SimpleGeneric works");
    }
}
