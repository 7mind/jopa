// Test generic type inference for iterator return type

import java.util.*;

public class IteratorReturnTypeTest {
    static class Wrapper<E> implements Iterator<E> {
        private Iterator<E> inner;

        Wrapper(Iterator<E> inner) {
            this.inner = inner;
        }

        public boolean hasNext() {
            return inner.hasNext();
        }

        public E next() {
            return inner.next();  // Should return E, not Object
        }

        public void remove() {
            inner.remove();
        }
    }

    static <K, V> Iterator<Map.Entry<K, V>> wrapEntries(Set<Map.Entry<K, V>> entries) {
        return new Wrapper<Map.Entry<K, V>>(entries.iterator()) {
            @Override
            public Map.Entry<K, V> next() {
                Map.Entry<K, V> entry = super.next();  // super.next() should return Map.Entry<K,V>
                return entry;
            }
        };
    }

    public static void main(String[] args) {
        Map<String, Integer> map = new HashMap<String, Integer>();
        map.put("one", 1);
        Iterator<Map.Entry<String, Integer>> it = wrapEntries(map.entrySet());
        while (it.hasNext()) {
            Map.Entry<String, Integer> e = it.next();
            System.out.println(e.getKey() + " = " + e.getValue());
        }
        System.out.println("IteratorReturnTypeTest: PASS");
    }
}
