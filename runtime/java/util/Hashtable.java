package java.util;

public class Hashtable<K, V> implements Map<K, V>, java.io.Serializable {
    public Hashtable() {}
    public Hashtable(int initialCapacity) {}
    public Hashtable(int initialCapacity, float loadFactor) {}
    public Hashtable(Map<? extends K, ? extends V> t) {}

    public int size() { return 0; }
    public boolean isEmpty() { return false; }
    public boolean containsKey(Object key) { return false; }
    public boolean containsValue(Object value) { return false; }
    public V get(Object key) { return null; }
    public V put(K key, V value) { return null; }
    public V remove(Object key) { return null; }
    public void putAll(Map<? extends K, ? extends V> t) {}
    public void clear() {}
    public Set<K> keySet() { return null; }
    public Collection<V> values() { return null; }
    public Set<Map.Entry<K, V>> entrySet() { return null; }
    public boolean equals(Object o) { return false; }
    public int hashCode() { return 0; }
    public Enumeration<K> keys() { return null; }
    public Enumeration<V> elements() { return null; }
    public boolean contains(Object value) { return false; }
    public Object clone() { return null; }
    public String toString() { return null; }
}
