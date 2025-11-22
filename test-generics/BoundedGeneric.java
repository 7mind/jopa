public class BoundedGeneric<T extends Number> {
    T value;

    public BoundedGeneric(T value) {
        this.value = value;
    }

    public T getValue() {
        return value;
    }
}
