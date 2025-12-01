package java.lang.reflect;

public final class Array {
    private Array() {}

    public static native Object newInstance(Class<?> componentType, int length);
    public static native Object newInstance(Class<?> componentType, int... dimensions);
    public static native int getLength(Object array);
    public static native Object get(Object array, int index);
    public static native boolean getBoolean(Object array, int index);
    public static native byte getByte(Object array, int index);
    public static native char getChar(Object array, int index);
    public static native short getShort(Object array, int index);
    public static native int getInt(Object array, int index);
    public static native long getLong(Object array, int index);
    public static native float getFloat(Object array, int index);
    public static native double getDouble(Object array, int index);
    public static native void set(Object array, int index, Object value);
    public static native void setBoolean(Object array, int index, boolean z);
    public static native void setByte(Object array, int index, byte b);
    public static native void setChar(Object array, int index, char c);
    public static native void setShort(Object array, int index, short s);
    public static native void setInt(Object array, int index, int i);
    public static native void setLong(Object array, int index, long l);
    public static native void setFloat(Object array, int index, float f);
    public static native void setDouble(Object array, int index, double d);
}
