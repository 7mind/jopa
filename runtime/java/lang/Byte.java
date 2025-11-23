package java.lang;

public final class Byte extends Number {
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
}
