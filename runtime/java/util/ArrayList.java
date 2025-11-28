// Minimal ArrayList for testing enhanced for-loop
package java.util;

public class ArrayList<E> implements List<E>, java.lang.Iterable<E> {
    private Object[] elements;
    private int size;

    public ArrayList() {
        elements = new Object[10];
        size = 0;
    }

    public boolean add(E e) {
        if (size >= elements.length) {
            Object[] newElements = new Object[elements.length * 2];
            for (int i = 0; i < size; i++) {
                newElements[i] = elements[i];
            }
            elements = newElements;
        }
        elements[size] = e;
        size++;
        return true;
    }

    @SuppressWarnings("unchecked")
    public E get(int index) {
        if (index < 0 || index >= size) {
            throw new RuntimeException("Index out of bounds");
        }
        return (E) elements[index];
    }

    public int size() {
        return size;
    }

    public boolean isEmpty() {
        return size == 0;
    }

    public boolean contains(Object o) {
        return false;
    }

    public Object[] toArray() {
        return elements;
    }

    public <T> T[] toArray(T[] a) {
        return a;
    }

    public boolean remove(Object o) {
        return false;
    }

    public boolean containsAll(Collection<?> c) {
        return false;
    }

    public boolean addAll(Collection<? extends E> c) {
        return false;
    }

    public boolean removeAll(Collection<?> c) {
        return false;
    }

    public boolean retainAll(Collection<?> c) {
        return false;
    }

    public void clear() {
        size = 0;
    }

    public E set(int index, E element) {
        return null;
    }

    public void add(int index, E element) {
    }

    public E remove(int index) {
        return null;
    }

    public int indexOf(Object o) {
        return -1;
    }

    public int lastIndexOf(Object o) {
        return -1;
    }

    public Iterator<E> iterator() {
        return new ArrayListIterator();
    }

    private class ArrayListIterator implements Iterator<E> {
        private int index;

        ArrayListIterator() {
            this.index = 0;
        }

        public boolean hasNext() {
            return index < size;
        }

        @SuppressWarnings("unchecked")
        public E next() {
            if (!hasNext()) {
                throw new RuntimeException("No more elements");
            }
            return (E) elements[index++];
        }

        public void remove() {
            throw new RuntimeException("remove() not supported");
        }
    }
}
