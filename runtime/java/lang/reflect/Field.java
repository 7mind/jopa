package java.lang.reflect;

/**
 * Minimal Field class for reflection support
 */
public class Field {
    private String name;

    public Field(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public native Object get(Object obj);
    public native void set(Object obj, Object value);
}
