package java.lang.reflect;

public class Field {
    private Field() {}

    public native String getName();
    public native int getModifiers();
    public native Class getType();
    public native Class getDeclaringClass();
    public native Object get(Object obj) throws IllegalAccessException;
    public native void set(Object obj, Object value) throws IllegalAccessException;
    public native boolean isAccessible();
    public native void setAccessible(boolean flag);
}
