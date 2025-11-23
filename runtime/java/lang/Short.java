package java.lang;

public final class Short extends Number {
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
}
