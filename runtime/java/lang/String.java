package java.lang;

public final class String implements java.io.Serializable, Comparable<String>, CharSequence {
    public String() {}

    public String(char[] value) {}

    public String(char[] value, int offset, int count) {}

    public String(String original) {}

    public String(byte[] bytes) {}

    public String(byte[] bytes, int offset, int length) {}

    @Deprecated
    public String(byte[] ascii, int hibyte) {}

    public native int length();

    public native char charAt(int index);

    public native boolean equals(Object obj);

    public native int hashCode();

    public String toString() {
        return this;
    }

    public native String substring(int beginIndex, int endIndex);

    public native String concat(String str);

    public CharSequence subSequence(int start, int end) {
        return substring(start, end);
    }

    public int indexOf(int ch) { return -1; }
    public int indexOf(int ch, int fromIndex) { return -1; }
    public int indexOf(String str) { return -1; }
    public int indexOf(String str, int fromIndex) { return -1; }
    public int lastIndexOf(int ch) { return -1; }
    public int lastIndexOf(int ch, int fromIndex) { return -1; }
    public int lastIndexOf(String str) { return -1; }
    public int lastIndexOf(String str, int fromIndex) { return -1; }
    public String substring(int beginIndex) { return null; }

    public native char[] toCharArray();

    public native String toLowerCase();

    public native String toUpperCase();

    public native String trim();

    public native boolean startsWith(String prefix);

    public native boolean endsWith(String suffix);

    public native boolean isEmpty();

    public native String replace(char oldChar, char newChar);

    public native String replace(CharSequence target, CharSequence replacement);

    public native String intern();

    public native int compareTo(String anotherString);

    public native int compareToIgnoreCase(String str);

    public native boolean contains(CharSequence s);

    public native boolean equalsIgnoreCase(String anotherString);

    public native boolean matches(String regex);

    public native String replaceAll(String regex, String replacement);

    public native String replaceFirst(String regex, String replacement);

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

    public static String format(String format, Object... args) { return ""; }
}
