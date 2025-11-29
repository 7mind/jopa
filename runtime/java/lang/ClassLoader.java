package java.lang;

import java.io.InputStream;
import java.net.URL;

public abstract class ClassLoader {
    protected ClassLoader() {}

    protected ClassLoader(ClassLoader parent) {}

    public Class<?> loadClass(String name) throws ClassNotFoundException {
        return null;
    }

    protected Class<?> findClass(String name) throws ClassNotFoundException {
        return null;
    }

    public URL getResource(String name) {
        return null;
    }

    public InputStream getResourceAsStream(String name) {
        return null;
    }

    public static URL getSystemResource(String name) {
        return null;
    }

    public static InputStream getSystemResourceAsStream(String name) {
        return null;
    }

    public final ClassLoader getParent() {
        return null;
    }

    public static ClassLoader getSystemClassLoader() {
        return null;
    }

    public void setDefaultAssertionStatus(boolean enabled) {}
    public void setPackageAssertionStatus(String packageName, boolean enabled) {}
    public void setClassAssertionStatus(String className, boolean enabled) {}
    public void clearAssertionStatus() {}
}
