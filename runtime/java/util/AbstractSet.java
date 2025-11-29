package java.util;

public abstract class AbstractSet<E> extends AbstractCollection<E> implements Set<E> {
    protected AbstractSet() {}

    public boolean equals(Object o) { return false; }
    public int hashCode() { return 0; }
    public boolean removeAll(Collection<?> c) { return false; }
}
