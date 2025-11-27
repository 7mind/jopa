package java.lang;

public final class Long extends Number {
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
        return String.valueOf((int) value);
    }
}
