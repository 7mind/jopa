// Stub for java.lang.Enum (Java 5)
package java.lang;

public abstract class Enum implements Comparable {
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

    public final int compareTo(Object o) {
        Enum other = (Enum) o;
        return ordinal - other.ordinal;
    }

    public static Enum valueOf(Class enumType, String name) {
        // Stub implementation
        return null;
    }
}
