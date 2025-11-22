public class TwoTypeParams<K, V> {
    K key;
    V value;

    public TwoTypeParams(K key, V value) {
        this.key = key;
        this.value = value;
    }

    public K getKey() {
        return key;
    }

    public V getValue() {
        return value;
    }
}
