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

    public static void main(String[] args) {
        TwoTypeParams<String, Integer> pair = new TwoTypeParams<String, Integer>("key", Integer.valueOf(42));
        Object k = pair.getKey();
        Object v = pair.getValue();
        if (k == null || v == null) {
            System.out.println("FAIL: getKey or getValue returned null");
            System.exit(1);
        }
        System.out.println("PASS: TwoTypeParams works");
    }
}
