package java.lang;

// Stub StringBuilder class for testing
public final class StringBuilder {
    private String value;

    public StringBuilder() {
        value = "";
    }

    public StringBuilder(String str) {
        value = str;
    }

    public StringBuilder append(String str) {
        if (str == null) {
            str = "null";
        }
        value = value + str;
        return this;
    }

    public StringBuilder append(int i) {
        value = value + String.valueOf(i);
        return this;
    }

    public StringBuilder append(Object obj) {
        if (obj == null) {
            value = value + "null";
        } else {
            value = value + obj.toString();
        }
        return this;
    }

    public String toString() {
        return value;
    }
}
