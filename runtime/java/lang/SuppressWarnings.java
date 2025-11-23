package java.lang;

import java.lang.annotation.Annotation;

/**
 * Indicates that the named compiler warnings should be suppressed
 * in the annotated element (and in all program elements contained
 * in the annotated element).
 */
public @interface SuppressWarnings {
    /**
     * The set of warnings that are to be suppressed by the compiler
     * in the annotated element.
     */
    String[] value();
}
