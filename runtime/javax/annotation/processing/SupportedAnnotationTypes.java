package javax.annotation.processing;

import java.lang.annotation.*;

@Documented
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
public @interface SupportedAnnotationTypes {
    String[] value();
}
