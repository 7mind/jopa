package java.util;

public class HashSet<E> extends AbstractSet<E> implements Set<E>, Cloneable, java.io.Serializable {
    public HashSet() {}
    public HashSet(Collection<? extends E> c) {}
    public HashSet(int initialCapacity, float loadFactor) {}
    public HashSet(int initialCapacity) {}

    public Iterator<E> iterator() { return null; }
    public int size() { return 0; }
    public boolean isEmpty() { return true; }
    public boolean contains(Object o) { return false; }
    public boolean add(E e) { return false; }
    public boolean remove(Object o) { return false; }
    public void clear() {}
    public Object clone() { return null; }
}
