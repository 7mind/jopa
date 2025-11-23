// Test 2: Generic class with bounded type parameter
class TestNumberBox<T extends TestNumber> {
    private T value;

    public TestNumberBox(T value) {
        this.value = value;
    }

    public T getValue() {
        return value;
    }
}
