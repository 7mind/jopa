package java.util;

public class EnumMap<K extends Enum<K>, V> implements Map<K, V>, java.io.Serializable, Cloneable {
    public EnumMap(Class<K> keyType) {}
    public EnumMap(EnumMap<K, ? extends V> m) {}
    public EnumMap(Map<K, ? extends V> m) {}

    public int size() { return 0; }
    public boolean isEmpty() { return true; }
    public boolean containsKey(Object key) { return false; }
    public boolean containsValue(Object value) { return false; }
    public V get(Object key) { return null; }
    public V put(K key, V value) { return null; }
    public V remove(Object key) { return null; }
    public void putAll(Map<? extends K, ? extends V> m) {}
    public void clear() {}
    public Set<K> keySet() { return null; }
    public Collection<V> values() { return null; }
    public Set<Map.Entry<K, V>> entrySet() { return null; }
    public EnumMap<K, V> clone() { return null; }
}
