package java.lang;

public final class StringBuffer {
    public StringBuffer() {}

    public StringBuffer(String str) {}

    public StringBuffer(int capacity) {}

    public native StringBuffer append(String str);

    public native StringBuffer append(boolean b);

    public native StringBuffer append(char c);

    public native StringBuffer append(int i);

    public native StringBuffer append(long l);

    public native StringBuffer append(float f);

    public native StringBuffer append(double d);

    public native StringBuffer append(Object obj);

    public native StringBuffer append(char[] str);

    public native int length();

    public native String toString();
}
