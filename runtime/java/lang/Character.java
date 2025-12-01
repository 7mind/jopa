package java.lang;

public final class Character implements Comparable<Character>, java.io.Serializable {
    public static final char MIN_VALUE = '\u0000';
    public static final char MAX_VALUE = '\uffff';
    public static final int SIZE = 16;
    public static final int BYTES = 2;
    public static final Class TYPE = getPrimitiveClass("char");
    private static native Class getPrimitiveClass(String name);

    private final char value;

    public Character(char value) {
        this.value = value;
    }

    public char charValue() {
        return value;
    }

    public static Character valueOf(char c) {
        return new Character(c);
    }

    public String toString() {
        return String.valueOf(value);
    }

    @Override
    public int compareTo(Character anotherCharacter) {
        return this.value - anotherCharacter.value;
    }
}
