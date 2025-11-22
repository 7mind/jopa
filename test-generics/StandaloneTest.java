package test;

// Minimal standalone test

class SimpleGeneric<T> {
    T field;
}

class TwoParams<K, V> {
    K key;
    V val;
}

class BoundedGeneric<T extends Base> {
    T item;
}

class Base {
    int x;
}

class Derived extends Base {
    int y;
}

// Test covariant override - should generate bridge method
class GenericBase<T> {
    T get() {
        return null;
    }
}

class SpecializedDerived extends GenericBase<Derived> {
    Derived get() {  // Covariant override: returns Derived instead of Base/Object
        return null;
    }
}
