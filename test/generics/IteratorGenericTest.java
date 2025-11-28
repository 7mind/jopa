// Test Iterator generic return type

import java.util.Iterator;

public class IteratorGenericTest {
    static class MyIterator<E> implements Iterator<E> {
        private E value;
        private boolean hasValue = true;

        MyIterator(E value) {
            this.value = value;
        }

        public boolean hasNext() {
            return hasValue;
        }

        public E next() {
            hasValue = false;
            return value;
        }

        public void remove() {}
    }

    public static void main(String[] args) {
        Iterator<String> it = new MyIterator<String>("hello");
        String s = it.next();  // Should return String, not Object
        System.out.println(s);
        System.out.println("IteratorGenericTest: PASS");
    }
}
