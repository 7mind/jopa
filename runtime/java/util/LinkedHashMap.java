package java.util;

public class LinkedHashMap<K, V> extends HashMap<K, V> {
    public LinkedHashMap() {}
    public LinkedHashMap(int initialCapacity) { super(initialCapacity); }
    public LinkedHashMap(int initialCapacity, float loadFactor) { super(initialCapacity, loadFactor); }
    public LinkedHashMap(int initialCapacity, float loadFactor, boolean accessOrder) {
        super(initialCapacity, loadFactor);
    }
    public LinkedHashMap(Map<? extends K, ? extends V> m) {}
}
