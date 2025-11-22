package java.lang;

// Stub Integer class for testing
public final class Integer {
    private final int value;

    public Integer(int value) {
        this.value = value;
    }

    public int intValue() {
        return value;
    }

    public String toString() {
        return String.valueOf(value);
    }

    public boolean equals(Object obj) {
        if (obj instanceof Integer) {
            Integer other = (Integer) obj;
            return value == other.value;
        }
        return false;
    }
}
