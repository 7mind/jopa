// Generic base class in separate file
package generics;

public class GenericBase<T> {
    private T value;

    public GenericBase(T value) {
        this.value = value;
    }

    public T get() {
        return value;
    }
}
