package java.lang.annotation;

/**
 * The common interface extended by all annotation types.
 */
public interface Annotation {
    /**
     * Returns the annotation type of this annotation.
     */
    Class annotationType();

    /**
     * Returns true if the specified object represents an annotation
     * that is logically equivalent to this one.
     */
    boolean equals(Object obj);

    /**
     * Returns the hash code of this annotation.
     */
    int hashCode();

    /**
     * Returns a string representation of this annotation.
     */
    String toString();
}
