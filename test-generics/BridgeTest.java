// Test bridge method generation for covariant returns

class Base<T> {
    public T getValue() {
        return null;
    }

    public void setValue(T value) {
    }
}

class StringBox extends Base<String> {
    private String value;

    public String getValue() {  // Covariant return - needs bridge
        return value;
    }

    public void setValue(String value) {
        this.value = value;
    }
}
