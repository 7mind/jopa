package java.util;

public class Vector<E> extends AbstractList<E> implements List<E>, RandomAccess, Cloneable, java.io.Serializable {
    protected Object[] elementData;
    protected int elementCount;
    protected int capacityIncrement;

    public Vector(int initialCapacity, int capacityIncrement) {}
    public Vector(int initialCapacity) {}
    public Vector() {}
    public Vector(Collection<? extends E> c) {}

    public synchronized void copyInto(Object[] anArray) {}
    public synchronized void trimToSize() {}
    public synchronized void ensureCapacity(int minCapacity) {}
    public synchronized void setSize(int newSize) {}
    public synchronized int capacity() { return 0; }
    public synchronized int size() { return 0; }
    public synchronized boolean isEmpty() { return true; }
    public Enumeration<E> elements() { return null; }
    public boolean contains(Object o) { return false; }
    public int indexOf(Object o) { return -1; }
    public synchronized int indexOf(Object o, int index) { return -1; }
    public synchronized int lastIndexOf(Object o) { return -1; }
    public synchronized int lastIndexOf(Object o, int index) { return -1; }
    public synchronized E elementAt(int index) { return null; }
    public synchronized E firstElement() { return null; }
    public synchronized E lastElement() { return null; }
    public synchronized void setElementAt(E obj, int index) {}
    public synchronized void removeElementAt(int index) {}
    public synchronized void insertElementAt(E obj, int index) {}
    public synchronized void addElement(E obj) {}
    public synchronized boolean removeElement(Object obj) { return false; }
    public synchronized void removeAllElements() {}
    public synchronized Object clone() { return null; }
    public synchronized Object[] toArray() { return null; }
    public synchronized <T> T[] toArray(T[] a) { return null; }
    public synchronized E get(int index) { return null; }
    public synchronized E set(int index, E element) { return null; }
    public synchronized boolean add(E e) { return false; }
    public boolean remove(Object o) { return false; }
    public void add(int index, E element) {}
    public synchronized E remove(int index) { return null; }
    public void clear() {}
    public synchronized boolean containsAll(Collection<?> c) { return false; }
    public synchronized boolean addAll(Collection<? extends E> c) { return false; }
    public synchronized boolean removeAll(Collection<?> c) { return false; }
    public synchronized boolean retainAll(Collection<?> c) { return false; }
    public synchronized boolean addAll(int index, Collection<? extends E> c) { return false; }
    public synchronized boolean equals(Object o) { return false; }
    public synchronized int hashCode() { return 0; }
    public synchronized String toString() { return null; }
    public synchronized List<E> subList(int fromIndex, int toIndex) { return null; }
    public synchronized Iterator<E> iterator() { return null; }
    public synchronized ListIterator<E> listIterator() { return null; }
    public synchronized ListIterator<E> listIterator(int index) { return null; }
}
