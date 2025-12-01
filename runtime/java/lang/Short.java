package java.lang;

public final class Short extends Number implements Comparable<Short>, java.io.Serializable {
    public static final short MIN_VALUE = (short) 0x8000;
    public static final short MAX_VALUE = (short) 0x7fff;
    public static final int SIZE = 16;
    public static final int BYTES = 2;
    public static final Class TYPE = getPrimitiveClass("short");
    private static native Class getPrimitiveClass(String name);

    private final short value;

    public Short(short value) {
        this.value = value;
    }

    public byte byteValue() {
        return (byte) value;
    }

    public short shortValue() {
        return value;
    }

    public int intValue() {
        return (int) value;
    }

    public long longValue() {
        return (long) value;
    }

    public float floatValue() {
        return (float) value;
    }

    public double doubleValue() {
        return (double) value;
    }

    public static Short valueOf(short s) {
        return new Short(s);
    }

    public String toString() {
        return String.valueOf((int) value);
    }

    @Override
    public int compareTo(Short anotherShort) {
        return this.value - anotherShort.value;
    }
}
