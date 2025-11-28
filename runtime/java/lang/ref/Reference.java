package java.lang.ref;

public abstract class Reference<T> {
    public T get() { return null; }
    public void clear() {}
    public boolean isEnqueued() { return false; }
    public boolean enqueue() { return false; }
}
