package java.lang;

public final class Float extends Number {
    private final float value;

    public Float(float value) {
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
        return value;
    }

    public double doubleValue() {
        return (double) value;
    }

    public static Float valueOf(float f) {
        return new Float(f);
    }

    public String toString() {
        return String.valueOf((int) value);
    }
}
