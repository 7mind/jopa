package java.util;

public class TreeSet<E> extends AbstractSet<E> implements NavigableSet<E>, Cloneable, java.io.Serializable {
    public TreeSet() {}
    public TreeSet(Comparator<? super E> comparator) {}
    public TreeSet(Collection<? extends E> c) {}
    public TreeSet(SortedSet<E> s) {}

    public Iterator<E> iterator() { return null; }
    public Iterator<E> descendingIterator() { return null; }
    public NavigableSet<E> descendingSet() { return null; }
    public int size() { return 0; }
    public boolean isEmpty() { return true; }
    public boolean contains(Object o) { return false; }
    public boolean add(E e) { return false; }
    public boolean remove(Object o) { return false; }
    public void clear() {}
    public E first() { return null; }
    public E last() { return null; }
    public E lower(E e) { return null; }
    public E floor(E e) { return null; }
    public E ceiling(E e) { return null; }
    public E higher(E e) { return null; }
    public E pollFirst() { return null; }
    public E pollLast() { return null; }
    public Object clone() { return null; }
    public Comparator<? super E> comparator() { return null; }
    public SortedSet<E> subSet(E fromElement, E toElement) { return null; }
    public SortedSet<E> headSet(E toElement) { return null; }
    public SortedSet<E> tailSet(E fromElement) { return null; }
    public NavigableSet<E> subSet(E fromElement, boolean fromInclusive, E toElement, boolean toInclusive) { return null; }
    public NavigableSet<E> headSet(E toElement, boolean inclusive) { return null; }
    public NavigableSet<E> tailSet(E fromElement, boolean inclusive) { return null; }
}
