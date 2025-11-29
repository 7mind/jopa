package javax.annotation.processing;

import java.lang.annotation.*;
import javax.lang.model.SourceVersion;

@Documented
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
public @interface SupportedSourceVersion {
    SourceVersion value();
}
