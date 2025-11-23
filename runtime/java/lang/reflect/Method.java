package java.lang.reflect;

/**
 * Minimal Method class for reflection support
 */
public class Method {
    private String name;

    public Method(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public native Object invoke(Object obj, Object[] args);
}
