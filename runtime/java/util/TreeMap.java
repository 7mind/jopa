package java.util;

public class TreeMap<K, V> implements Map<K, V>, NavigableMap<K, V>, java.io.Serializable, Cloneable {
    public TreeMap() {}
    public TreeMap(Comparator<? super K> comparator) {}
    public TreeMap(Map<? extends K, ? extends V> m) {}
    public TreeMap(SortedMap<K, ? extends V> m) {}

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
    public Comparator<? super K> comparator() { return null; }
    public K firstKey() { return null; }
    public K lastKey() { return null; }
    public SortedMap<K, V> subMap(K fromKey, K toKey) { return null; }
    public SortedMap<K, V> headMap(K toKey) { return null; }
    public SortedMap<K, V> tailMap(K fromKey) { return null; }
    public Map.Entry<K, V> firstEntry() { return null; }
    public Map.Entry<K, V> lastEntry() { return null; }
    public Map.Entry<K, V> pollFirstEntry() { return null; }
    public Map.Entry<K, V> pollLastEntry() { return null; }
    public Map.Entry<K, V> lowerEntry(K key) { return null; }
    public K lowerKey(K key) { return null; }
    public Map.Entry<K, V> floorEntry(K key) { return null; }
    public K floorKey(K key) { return null; }
    public Map.Entry<K, V> ceilingEntry(K key) { return null; }
    public K ceilingKey(K key) { return null; }
    public Map.Entry<K, V> higherEntry(K key) { return null; }
    public K higherKey(K key) { return null; }
    public NavigableSet<K> navigableKeySet() { return null; }
    public NavigableSet<K> descendingKeySet() { return null; }
    public NavigableMap<K, V> descendingMap() { return null; }
    public NavigableMap<K, V> subMap(K fromKey, boolean fromInclusive, K toKey, boolean toInclusive) { return null; }
    public NavigableMap<K, V> headMap(K toKey, boolean inclusive) { return null; }
    public NavigableMap<K, V> tailMap(K fromKey, boolean inclusive) { return null; }
    public Object clone() { return null; }
}
