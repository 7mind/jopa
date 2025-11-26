public class BoundedGeneric<T extends Number> {
    T value;

    public BoundedGeneric(T value) {
        this.value = value;
    }

    public T getValue() {
        return value;
    }

    public double getDoubleValue() {
        // T extends Number, so can call Number methods
        return value.doubleValue();
    }

    public static void main(String[] args) {
        BoundedGeneric<Integer> bg = new BoundedGeneric<Integer>(Integer.valueOf(42));
        Number n = bg.getValue();
        if (n == null) {
            System.out.println("FAIL: getValue returned null");
            System.exit(1);
        }
        double d = bg.getDoubleValue();
        if (d != 42.0) {
            System.out.println("FAIL: getDoubleValue returned wrong value");
            System.exit(1);
        }
        System.out.println("PASS: BoundedGeneric works");
    }
}
