package java.lang;

public final class Float extends Number implements Comparable<Float> {
    public static final float POSITIVE_INFINITY = 1.0f / 0.0f;
    public static final float NEGATIVE_INFINITY = -1.0f / 0.0f;
    public static final float NaN = 0.0f / 0.0f;
    public static final float MAX_VALUE = 0.0f;
    public static final float MIN_NORMAL = 0.0f;
    public static final float MIN_VALUE = 0.0f;
    public static final int MAX_EXPONENT = 127;
    public static final int MIN_EXPONENT = -126;
    public static final int SIZE = 32;

    private final float value;

    public Float(float value) {
        this.value = value;
    }

    public Float(double value) {
        this.value = (float) value;
    }

    public Float(String s) throws NumberFormatException {
        this.value = parseFloat(s);
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

    public static Float valueOf(String s) throws NumberFormatException {
        return new Float(parseFloat(s));
    }

    public static float parseFloat(String s) throws NumberFormatException {
        return 0.0f;
    }

    public static native int floatToIntBits(float value);
    public static native int floatToRawIntBits(float value);
    public static native float intBitsToFloat(int bits);

    public static boolean isNaN(float v) { return v != v; }
    public static boolean isInfinite(float v) { return v == POSITIVE_INFINITY || v == NEGATIVE_INFINITY; }

    public String toString() {
        return String.valueOf(value);
    }

    public int compareTo(Float anotherFloat) {
        return compare(this.value, anotherFloat.value);
    }

    public static int compare(float f1, float f2) {
        if (f1 < f2) return -1;
        if (f1 > f2) return 1;
        return 0;
    }

    public boolean equals(Object obj) {
        return (obj instanceof Float) && (floatToIntBits(((Float)obj).value) == floatToIntBits(value));
    }

    public int hashCode() {
        return floatToIntBits(value);
    }
}