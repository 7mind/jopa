package java.lang;

public final class Long extends Number implements Comparable<Long>, java.io.Serializable {
    public static final long MIN_VALUE = 0x8000000000000000L;
    public static final long MAX_VALUE = 0x7fffffffffffffffL;
    public static final int SIZE = 64;
    public static final int BYTES = 8;
    public static final Class TYPE = getPrimitiveClass("long");
    private static native Class getPrimitiveClass(String name);

    private final long value;

    public Long(long value) {
        this.value = value;
    }

    public byte byteValue() {
        return (byte) value;
    }

    public short shortValue() {
        return (short) value;
    }

    public int intValue() {
        return (int) value;
    }

    public long longValue() {
        return value;
    }

    public float floatValue() {
        return (float) value;
    }

    public double doubleValue() {
        return (double) value;
    }

    public static Long valueOf(long l) {
        return new Long(l);
    }

    public String toString() {
        return String.valueOf(value);
    }

    public static String toString(long l) {
        return String.valueOf(l);
    }

    @Override
    public int compareTo(Long anotherLong) {
        return this.value < anotherLong.value ? -1 : (this.value == anotherLong.value ? 0 : 1);
    }
}
