// Simpler test without type arguments

class GenericBase<T> {
    public T getValue() {
        return null;
    }
}

class Simple extends GenericBase {
    private String value;

    public String getValue() {
        return value;
    }
}
