package java.util;

/**
 * A collection that contains no duplicate elements.
 */
public interface Set<E> extends Collection<E> {
    int size();
    boolean isEmpty();
    boolean contains(Object o);
    Iterator<E> iterator();
    boolean add(E e);
    boolean remove(Object o);
    void clear();
}
