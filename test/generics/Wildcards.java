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

    // Upper bounded wildcard: ? extends TestNumber erases to TestNumber
    Container<? extends TestNumber> upperBounded;

    // Lower bounded wildcard: ? super TestInteger erases to Object
    Container<? super TestInteger> lowerBounded;

    void test() {
        // Local variables with wildcards
        Container<?> c1 = new Container<Object>(new Object());
        Container<? extends TestNumber> c2 = new Container<TestNumber>(new TestInteger(42));
        Container<? super TestInteger> c3 = new Container<Object>(new Object());
    }
}

class TestNumber {
    int value;
}

class TestInteger extends TestNumber {
    TestInteger(int v) {
        value = v;
    }
}
