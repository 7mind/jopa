package java.util;

public abstract class AbstractCollection<E> implements Collection<E> {
    protected AbstractCollection() {}

    public abstract Iterator<E> iterator();
    public abstract int size();

    public boolean isEmpty() { return size() == 0; }
    public boolean contains(Object o) { return false; }
    public Object[] toArray() { return null; }
    public <T> T[] toArray(T[] a) { return null; }
    public boolean add(E e) { throw new UnsupportedOperationException(); }
    public boolean remove(Object o) { return false; }
    public boolean containsAll(Collection<?> c) { return false; }
    public boolean addAll(Collection<? extends E> c) { return false; }
    public boolean removeAll(Collection<?> c) { return false; }
    public boolean retainAll(Collection<?> c) { return false; }
    public void clear() {}
    public String toString() { return ""; }
}
