package java.net;

import java.io.IOException;
import java.security.AccessControlContext;
import java.util.Enumeration;

public class URLClassLoader extends ClassLoader {
    protected URLClassLoader(URL[] urls, ClassLoader parent, AccessControlContext acc) {
        super(parent);
    }

    public URLClassLoader(URL[] urls) {
        super();
    }

    public URLClassLoader(URL[] urls, ClassLoader parent) {
        super(parent);
    }

    public static URLClassLoader newInstance(URL[] urls) {
        return new URLClassLoader(urls);
    }

    public static URLClassLoader newInstance(URL[] urls, ClassLoader parent) {
        return new URLClassLoader(urls, parent);
    }

    public URL findResource(String name) {
        return null;
    }

    public Enumeration<URL> findResources(String name) throws IOException {
        return null;
    }

    protected Class<?> findClass(String name) throws ClassNotFoundException {
        throw new ClassNotFoundException(name);
    }
}
