package java.lang;

// Stub Integer class for testing
public final class Integer extends Number implements Comparable<Integer>, java.io.Serializable {
    public static final Class TYPE = getPrimitiveClass("int");
    private static native Class getPrimitiveClass(String name);

    private final int value;

    public Integer(int value) {
        this.value = value;
    }

    public byte byteValue() {
        return (byte) value;
    }

    public short shortValue() {
        return (short) value;
    }

    public int intValue() {
        return value;
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

    public static Integer valueOf(int i) {
        return new Integer(i);
    }

    public String toString() {
        return String.valueOf(value);
    }

    public boolean equals(Object obj) {
        if (obj instanceof Integer) {
            Integer other = (Integer) obj;
            return value == other.value;
        }
        return false;
    }

    @Override
    public int compareTo(Integer anotherInteger) {
        return compare(this.value, anotherInteger.value);
    }

    public static int compare(int x, int y) {
        return (x < y) ? -1 : ((x == y) ? 0 : 1);
    }
}
