// Test type argument arity validation

class Box<T> {
    T value;
}

class Pair<K, V> {
    K key;
    V value;
}

class TestArity {
    // ERROR: Box has 1 type parameter, but given 2 type arguments
    Box<Object, Object> wrongBox;

    // ERROR: Pair has 2 type parameters, but given 1 type argument
    Pair<Object> wrongPair;

    // ERROR: Pair has 2 type parameters, but given 3 type arguments
    Pair<Object, Object, Object> wrongPair2;
}
