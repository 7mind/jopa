package javax.annotation.processing;

import java.lang.annotation.Annotation;
import java.util.Set;
import javax.lang.model.element.Element;
import javax.lang.model.element.TypeElement;

public interface RoundEnvironment {
    boolean processingOver();
    boolean errorRaised();
    Set<? extends Element> getRootElements();
    Set<? extends Element> getElementsAnnotatedWith(TypeElement a);
    Set<? extends Element> getElementsAnnotatedWith(Class<? extends Annotation> a);
}
