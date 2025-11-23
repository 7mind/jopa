// Minimal ArrayList for testing enhanced for-loop
package java.util;

public class ArrayList implements java.lang.Iterable {
    private Object[] elements;
    private int size;

    public ArrayList() {
        elements = new Object[10];
        size = 0;
    }

    public boolean add(Object e) {
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

    public Object get(int index) {
        if (index < 0 || index >= size) {
            throw new RuntimeException("Index out of bounds");
        }
        return elements[index];
    }

    public int size() {
        return size;
    }

    public Iterator iterator() {
        return new ArrayListIterator(this);
    }

    private static class ArrayListIterator implements Iterator {
        private ArrayList list;
        private int index;

        ArrayListIterator(ArrayList list) {
            this.list = list;
            this.index = 0;
        }

        public boolean hasNext() {
            return index < list.size();
        }

        public Object next() {
            if (!hasNext()) {
                throw new RuntimeException("No more elements");
            }
            Object result = list.get(index);
            index++;
            return result;
        }

        public void remove() {
            throw new RuntimeException("remove() not supported");
        }
    }
}
