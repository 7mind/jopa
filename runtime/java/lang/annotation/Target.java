package java.lang.annotation;

/**
 * Indicates the contexts in which an annotation type is applicable.
 *
 * @since 1.5
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.ANNOTATION_TYPE)
public @interface Target {
    ElementType[] value();
}
