package java.util;

public abstract class ResourceBundle {
    public ResourceBundle() {}

    public static final ResourceBundle getBundle(String baseName) {
        return null;
    }

    public static final ResourceBundle getBundle(String baseName, Locale locale) {
        return null;
    }

    public static final ResourceBundle getBundle(String baseName, Locale locale, ClassLoader loader) {
        return null;
    }

    public final String getString(String key) {
        return null;
    }

    public final Object getObject(String key) {
        return null;
    }

    public Enumeration<String> getKeys() {
        return null;
    }

    protected abstract Object handleGetObject(String key);
    protected abstract Enumeration<String> handleGetKeys();
    public Locale getLocale() { return null; }
}
