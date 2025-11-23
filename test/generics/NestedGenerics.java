// Test nested parameterized types

class Container<T> {
    T item;

    Container(T item) {
        this.item = item;
    }
}

class Pair<K, V> {
    K key;
    V value;

    Pair(K k, V v) {
        key = k;
        value = v;
    }
}

class TestNested {
    // Nested parameterized types: Container<Pair<...>>
    Container<Pair<Container<Object>, Container<Object>>> nested;

    void test() {
        // This tests that ProcessType recursively processes nested type arguments
        Container<Object> c1 = new Container<Object>(new Object());
        Container<Object> c2 = new Container<Object>(new Object());
        Pair<Container<Object>, Container<Object>> pair =
            new Pair<Container<Object>, Container<Object>>(c1, c2);
        Container<Pair<Container<Object>, Container<Object>>> container =
            new Container<Pair<Container<Object>, Container<Object>>>(pair);
    }
}
