package java.lang.annotation;

/**
 * Indicates that annotations with a type are to be documented by javadoc
 * and similar tools by default.
 *
 * @since 1.5
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.ANNOTATION_TYPE)
public @interface Documented {}
