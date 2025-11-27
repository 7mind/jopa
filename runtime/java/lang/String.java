package java.lang;

public final class String {
    public String() {}

    public String(char[] value) {}

    public String(char[] value, int offset, int count) {}

    public String(String original) {}

    public String(byte[] bytes) {}

    public String(byte[] bytes, int offset, int length) {}

    public native int length();

    public native char charAt(int index);

    public native boolean equals(Object obj);

    public native int hashCode();

    public String toString() {
        return this;
    }

    public native String substring(int beginIndex);

    public native String substring(int beginIndex, int endIndex);

    public native int indexOf(int ch);

    public native int indexOf(String str);

    public native int lastIndexOf(int ch);

    public native int lastIndexOf(String str);

    public native char[] toCharArray();

    public native String toLowerCase();

    public native String toUpperCase();

    public native String trim();

    public native boolean startsWith(String prefix);

    public native boolean endsWith(String suffix);

    public native boolean isEmpty();

    public native String replace(char oldChar, char newChar);

    public native String[] split(String regex);

    public native byte[] getBytes();

    // Static valueOf methods
    public static native String valueOf(boolean b);

    public static native String valueOf(char c);

    public static native String valueOf(char[] data);

    public static native String valueOf(int i);

    public static native String valueOf(long l);

    public static native String valueOf(float f);

    public static native String valueOf(double d);

    public static native String valueOf(Object obj);
}
