package java.lang.annotation;

/**
 * Indicates how long annotations with the annotated type are to be retained.
 *
 * @since 1.5
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.ANNOTATION_TYPE)
public @interface Retention {
    RetentionPolicy value();
}
