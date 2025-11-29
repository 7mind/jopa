package java.nio.charset;

public abstract class Charset implements Comparable<Charset> {
    protected Charset(String canonicalName, String[] aliases) {}

    public static boolean isSupported(String charsetName) { return false; }
    public static Charset forName(String charsetName) { return null; }
    public static Charset defaultCharset() { return null; }

    public final String name() { return ""; }
    public String displayName() { return ""; }
    public final boolean isRegistered() { return false; }
    public boolean canEncode() { return false; }
    public abstract CharsetDecoder newDecoder();
    public abstract CharsetEncoder newEncoder();
    public abstract boolean contains(Charset cs);

    public final int compareTo(Charset that) { return 0; }
    public final int hashCode() { return 0; }
    public final boolean equals(Object ob) { return false; }
    public final String toString() { return ""; }
}
