package java.lang.reflect;

public final class Constructor {
    private Constructor() {}

    public native String getName();
    public native int getModifiers();
    public native Class[] getParameterTypes();
    public native Class[] getExceptionTypes();
    public native Class getDeclaringClass();
    public native Object newInstance(Object[] args) throws InstantiationException, IllegalAccessException;
    public native boolean isAccessible();
    public native void setAccessible(boolean flag);
}
