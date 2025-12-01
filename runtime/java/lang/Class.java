package java.lang;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.annotation.Annotation;

public final class Class<T> implements java.lang.reflect.Type {
    private Class() {}

    public String toString() {
        return (isInterface() ? "interface " : "class ") + getName();
    }

    public native String getName();
    public native String getSimpleName();
    public native String getCanonicalName();

    public static native Class forName(String className) throws ClassNotFoundException;

    // Methods
    public native Method[] getDeclaredMethods();
    public native Method[] getMethods();
    public native Method getMethod(String name, Class... parameterTypes) throws NoSuchMethodException, SecurityException;
    public native Method getDeclaredMethod(String name, Class... parameterTypes) throws NoSuchMethodException, SecurityException;

    // Fields
    public native Field[] getDeclaredFields();
    public native Field[] getFields();
    public native Field getField(String name) throws NoSuchFieldException, SecurityException;
    public native Field getDeclaredField(String name) throws NoSuchFieldException, SecurityException;

    // Constructors
    public native Constructor[] getDeclaredConstructors();
    public native Constructor[] getConstructors();
    public native Constructor getConstructor(Class... parameterTypes) throws NoSuchMethodException, SecurityException;
    public native Constructor getDeclaredConstructor(Class... parameterTypes) throws NoSuchMethodException, SecurityException;

    // Class information
    public native Class getSuperclass();
    public native Class[] getInterfaces();
    public native int getModifiers();
    public native boolean isInterface();
    public native boolean isArray();
    public native boolean isPrimitive();
    public native boolean isEnum();
    public native boolean isAnnotation();
    public native boolean isSynthetic();
    public native boolean isAnonymousClass();
    public native boolean isLocalClass();
    public native boolean isMemberClass();

    // Component type for arrays
    public native Class getComponentType();

    // Package
    public native Package getPackage();

    // Annotations
    public native Annotation[] getDeclaredAnnotations();
    public native Annotation[] getAnnotations();
    public native <A extends Annotation> A getAnnotation(Class<A> annotationType);
    public native boolean isAnnotationPresent(Class<? extends Annotation> annotationType);

    // Declaring class for inner classes
    public native Class getDeclaringClass();
    public native Class getEnclosingClass();

    // Instance creation
    public native Object newInstance() throws InstantiationException, IllegalAccessException;

    // Type hierarchy
    public native boolean isAssignableFrom(Class cls);
    public native boolean isInstance(Object obj);
    public native Class[] getDeclaredClasses();
    public native Class[] getClasses();

    // Generic type information
    public native java.lang.reflect.Type getGenericSuperclass();
    public native java.lang.reflect.Type[] getGenericInterfaces();

    // Enum methods
    public native T[] getEnumConstants();

    // Class loading
    public native ClassLoader getClassLoader();
    public native java.security.ProtectionDomain getProtectionDomain();
    public native java.net.URL getResource(String name);
    public native java.io.InputStream getResourceAsStream(String name);

    public boolean desiredAssertionStatus() {
        return false;
    }
}
