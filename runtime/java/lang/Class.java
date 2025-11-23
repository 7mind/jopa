package java.lang;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

public final class Class {
    private Class() {}

    public String toString() {
        return "";
    }

    public native String getName();

    public static native Class forName(String className);

    public native Method[] getDeclaredMethods();
    public native Method[] getMethods();

    public native Field[] getDeclaredFields();
    public native Field[] getFields();

    public native Method getMethod(String name, Class[] parameterTypes);
    public native Method getDeclaredMethod(String name, Class[] parameterTypes);

    public native Field getField(String name);
    public native Field getDeclaredField(String name);
}
