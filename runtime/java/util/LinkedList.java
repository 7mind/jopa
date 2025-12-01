package java.util;

public class LinkedList<E> extends AbstractList<E> implements List<E>, java.io.Serializable {
    public LinkedList() {}
    public LinkedList(Collection<? extends E> c) {}

    public E getFirst() { return null; }
    public E getLast() { return null; }
    public E removeFirst() { return null; }
    public E removeLast() { return null; }
    public void addFirst(E e) {}
    public void addLast(E e) {}
    public boolean contains(Object o) { return false; }
    public int size() { return 0; }
    public boolean add(E e) { return false; }
    public boolean remove(Object o) { return false; }
    public boolean addAll(Collection<? extends E> c) { return false; }
    public boolean addAll(int index, Collection<? extends E> c) { return false; }
    public void clear() {}
    public E get(int index) { return null; }
    public E set(int index, E element) { return null; }
    public void add(int index, E element) {}
    public E remove(int index) { return null; }
    public int indexOf(Object o) { return 0; }
    public int lastIndexOf(Object o) { return 0; }
    public E peek() { return null; }
    public E element() { return null; }
    public E poll() { return null; }
    public boolean offer(E e) { return false; }
    public boolean offerFirst(E e) { return false; }
    public boolean offerLast(E e) { return false; }
    public E peekFirst() { return null; }
    public E peekLast() { return null; }
    public E pollFirst() { return null; }
    public E pollLast() { return null; }
    public void push(E e) {}
    public E pop() { return null; }
    public boolean removeFirstOccurrence(Object o) { return false; }
    public boolean removeLastOccurrence(Object o) { return false; }
    public ListIterator<E> listIterator(int index) { return null; }
    public Iterator<E> descendingIterator() { return null; }
    public Object clone() { return null; }
    public Object[] toArray() { return null; }
    public <T> T[] toArray(T[] a) { return null; }
}
