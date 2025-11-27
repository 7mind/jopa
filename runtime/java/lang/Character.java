package java.lang;

public final class Character {
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
}
