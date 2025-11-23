package java.lang.annotation;

/**
 * The constants of this enumerated type provide a simple classification of the
 * syntactic locations where annotations may appear in a Java program.
 *
 * @since 1.5
 */
public enum ElementType {
    TYPE,
    FIELD,
    METHOD,
    PARAMETER,
    CONSTRUCTOR,
    LOCAL_VARIABLE,
    ANNOTATION_TYPE,
    PACKAGE
}
