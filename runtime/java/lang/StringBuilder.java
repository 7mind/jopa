package java.lang;

public final class StringBuilder {
    public StringBuilder() {}

    public StringBuilder(String str) {}

    public StringBuilder(int capacity) {}

    public native StringBuilder append(String str);

    public native StringBuilder append(boolean b);

    public native StringBuilder append(char c);

    public native StringBuilder append(int i);

    public native StringBuilder append(long l);

    public native StringBuilder append(float f);

    public native StringBuilder append(double d);

    public native StringBuilder append(Object obj);

    public native StringBuilder append(char[] str);

    public native int length();

    public native String toString();
}
