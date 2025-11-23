// Test wildcard types

class Container<T> {
    T item;

    Container(T item) {
        this.item = item;
    }

    T get() {
        return item;
    }
}

class TestWildcards {
    // Unbounded wildcard: ? erases to Object
    Container<?> unbounded;

    // Upper bounded wildcard: ? extends Number erases to Number
    Container<? extends Number> upperBounded;

    // Lower bounded wildcard: ? super Integer erases to Object
    Container<? super Integer> lowerBounded;

    void test() {
        // Local variables with wildcards
        Container<?> c1 = new Container<Object>(new Object());
        Container<? extends Number> c2 = new Container<Number>(new Integer(42));
        Container<? super Integer> c3 = new Container<Object>(new Object());
    }
}

class Number {
    int value;
}

class Integer extends Number {
    Integer(int v) {
        value = v;
    }
}
