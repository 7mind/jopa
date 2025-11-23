package java.lang;

import java.lang.annotation.*;

/**
 * A programmer assertion that the body of the annotated method or
 * constructor does not perform potentially unsafe operations on its
 * varargs parameter. Applying this annotation to a method or constructor
 * suppresses unchecked warnings about a non-reifiable variable arity
 * (vararg) type and suppresses unchecked warnings about parameterized
 * array creation at call sites.
 *
 * @since 1.7
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.CONSTRUCTOR, ElementType.METHOD})
public @interface SafeVarargs {}
