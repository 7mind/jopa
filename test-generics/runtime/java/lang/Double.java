package java.lang;

public final class Double extends Number {
    private final double value;

    public Double(double value) {
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
        return (long) value;
    }

    public float floatValue() {
        return (float) value;
    }

    public double doubleValue() {
        return value;
    }

    public static Double valueOf(double d) {
        return new Double(d);
    }

    public String toString() {
        return String.valueOf((int) value);
    }
}
