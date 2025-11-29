package java.lang;

public final class Float extends Number {
    public static final Class TYPE = getPrimitiveClass("float");
    private static native Class getPrimitiveClass(String name);

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

    public static final float POSITIVE_INFINITY = 1.0f / 0.0f;
    public static final float NEGATIVE_INFINITY = -1.0f / 0.0f;
    public static final float NaN = 0.0f / 0.0f;
    public static final float MAX_VALUE = 3.4028235e+38f;
    public static final float MIN_VALUE = 1.4e-45f;

    public String toString() {
        return String.valueOf((int) value);
    }
}
