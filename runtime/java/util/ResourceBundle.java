package java.util;

public abstract class ResourceBundle {
    public static final ResourceBundle getBundle(String baseName) {
        return null;
    }
    public static final ResourceBundle getBundle(String baseName, Locale locale) {
        return null;
    }
    public abstract Object handleGetObject(String key);
    public abstract Enumeration<String> getKeys();
    public String getString(String key) {
        return (String) handleGetObject(key);
    }
    public Object getObject(String key) {
        return handleGetObject(key);
    }
}
