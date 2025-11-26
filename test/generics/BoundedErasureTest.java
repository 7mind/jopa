// Test bounded generics erasure
public class BoundedErasureTest<T extends Number> {
    T value;

    public BoundedErasureTest(T v) {
        this.value = v;
    }

    public double getDouble() {
        // T extends Number, so T should erase to Number
        // This should call Number.doubleValue()
        return value.doubleValue();
    }

    public static void main(String[] args) {
        BoundedErasureTest<Integer> test = new BoundedErasureTest<Integer>(Integer.valueOf(42));
        double d = test.getDouble();
        if (d == 42.0) {
            System.out.println("PASS: bounded erasure works");
        } else {
            System.out.println("FAIL: expected 42.0");
            System.exit(1);
        }
    }
}
