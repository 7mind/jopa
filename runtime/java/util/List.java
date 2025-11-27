package java.util;

/**
 * An ordered collection (also known as a sequence).
 */
public interface List<E> {
    int size();
    boolean isEmpty();
    boolean contains(Object o);
    Iterator<E> iterator();
    E get(int index);
    E set(int index, E element);
    boolean add(E e);
    void add(int index, E element);
    E remove(int index);
    int indexOf(Object o);
    int lastIndexOf(Object o);
}
