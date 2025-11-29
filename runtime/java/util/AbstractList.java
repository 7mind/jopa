package java.util;

public abstract class AbstractList<E> extends AbstractCollection<E> implements List<E> {
    protected transient int modCount = 0;

    protected AbstractList() {}

    public boolean add(E e) { add(size(), e); return true; }
    abstract public E get(int index);
    public E set(int index, E element) { throw new UnsupportedOperationException(); }
    public void add(int index, E element) { throw new UnsupportedOperationException(); }
    public E remove(int index) { throw new UnsupportedOperationException(); }
    public int indexOf(Object o) { return -1; }
    public int lastIndexOf(Object o) { return -1; }
    public void clear() { removeRange(0, size()); }
    public boolean addAll(int index, Collection<? extends E> c) { return false; }
    public Iterator<E> iterator() { return null; }
    public ListIterator<E> listIterator() { return listIterator(0); }
    public ListIterator<E> listIterator(int index) { return null; }
    public List<E> subList(int fromIndex, int toIndex) { return null; }
    public boolean equals(Object o) { return false; }
    public int hashCode() { return 0; }
    protected void removeRange(int fromIndex, int toIndex) {}
}
