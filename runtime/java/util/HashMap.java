package java.util;

public class HashMap<K, V> implements Map<K, V> {
    public HashMap() {}
    public HashMap(int initialCapacity) {}
    public HashMap(int initialCapacity, float loadFactor) {}
    public HashMap(Map<? extends K, ? extends V> m) {}

    public int size() { return 0; }
    public boolean isEmpty() { return true; }
    public boolean containsKey(Object key) { return false; }
    public boolean containsValue(Object value) { return false; }
    public V get(Object key) { return null; }
    public V put(K key, V value) { return null; }
    public V remove(Object key) { return null; }
    public void clear() {}
    public Set<K> keySet() { return null; }
    public Collection<V> values() { return null; }
    public Set<Map.Entry<K, V>> entrySet() { return null; }
}
