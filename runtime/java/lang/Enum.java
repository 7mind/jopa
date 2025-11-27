// Stub for java.lang.Enum (Java 5)
package java.lang;

@SuppressWarnings({"unchecked", "rawtypes"})
public abstract class Enum<E extends Enum<E>> implements Comparable<E> {
    private final String name;
    private final int ordinal;

    protected Enum(String name, int ordinal) {
        this.name = name;
        this.ordinal = ordinal;
    }

    public final String name() {
        return name;
    }

    public final int ordinal() {
        return ordinal;
    }

    public String toString() {
        return name;
    }

    public final boolean equals(Object other) {
        return this == other;
    }

    public final int hashCode() {
        return ordinal;
    }

    public final int compareTo(E o) {
        return ordinal - o.ordinal();
    }

    public final Class<E> getDeclaringClass() {
        return null;
    }

    public static <T extends Enum<T>> T valueOf(Class<T> enumType, String name) {
        // Stub implementation
        return null;
    }
}
