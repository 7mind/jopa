package java.lang;

public final class Byte extends Number implements Comparable<Byte>, java.io.Serializable {
    public static final byte MIN_VALUE = (byte) 0x80;
    public static final byte MAX_VALUE = (byte) 0x7f;
    public static final int SIZE = 8;
    public static final int BYTES = 1;
    public static final Class TYPE = getPrimitiveClass("byte");
    private static native Class getPrimitiveClass(String name);

    private final byte value;

    public Byte(byte value) {
        this.value = value;
    }

    public byte byteValue() {
        return value;
    }

    public short shortValue() {
        return (short) value;
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

    public static Byte valueOf(byte b) {
        return new Byte(b);
    }

    public String toString() {
        return String.valueOf((int) value);
    }

    @Override
    public int compareTo(Byte anotherByte) {
        return this.value - anotherByte.value;
    }
}
