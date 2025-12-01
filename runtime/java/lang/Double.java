package java.lang;

public final class Double extends Number implements Comparable<Double> {
    public static final Class TYPE = getPrimitiveClass("double");
    private static native Class getPrimitiveClass(String name);
    public static final double POSITIVE_INFINITY = 1.0 / 0.0;
    public static final double NEGATIVE_INFINITY = -1.0 / 0.0;
    public static final double NaN = 0.0d / 0.0;
    public static final double MAX_VALUE = 1.7976931348623157e308;
    public static final double MIN_NORMAL = 2.2250738585072014e-308;
    public static final double MIN_VALUE = 4.9e-324;
    public static final int MAX_EXPONENT = 1023;
    public static final int MIN_EXPONENT = -1022;
    public static final int SIZE = 64;
    public static final int BYTES = 8;

    private final double value;

    public Double(double value) {
        this.value = value;
    }

    public Double(String s) throws NumberFormatException {
        this.value = parseDouble(s);
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

    public static Double valueOf(String s) throws NumberFormatException {
        return new Double(parseDouble(s));
    }

    public static double parseDouble(String s) throws NumberFormatException {
        return 0.0;
    }

    public static native long doubleToLongBits(double value);
    public static native long doubleToRawLongBits(double value);
    public static native double longBitsToDouble(long bits);

    public static boolean isNaN(double v) { return v != v; }
    public static boolean isInfinite(double v) { return v == POSITIVE_INFINITY || v == NEGATIVE_INFINITY; }

    public String toString() {
        return String.valueOf(value);
    }

    public int compareTo(Double anotherDouble) {
        return compare(this.value, anotherDouble.value);
    }

    public static int compare(double d1, double d2) {
        if (d1 < d2) return -1;
        if (d1 > d2) return 1;
        return 0;
    }

    public boolean equals(Object obj) {
        return (obj instanceof Double) && (doubleToLongBits(((Double)obj).value) == doubleToLongBits(value));
    }

    public int hashCode() {
        long bits = doubleToLongBits(value);
        return (int)(bits ^ (bits >>> 32));
    }
}
