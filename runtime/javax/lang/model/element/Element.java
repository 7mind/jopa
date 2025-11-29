package javax.lang.model.element;

import java.lang.annotation.Annotation;
import java.util.List;
import java.util.Set;
import javax.lang.model.type.TypeMirror;

public interface Element {
    TypeMirror asType();
    ElementKind getKind();
    Set<Modifier> getModifiers();
    Name getSimpleName();
    Element getEnclosingElement();
    List<? extends Element> getEnclosedElements();
    List<? extends AnnotationMirror> getAnnotationMirrors();
    <A extends Annotation> A getAnnotation(Class<A> annotationType);
    <R, P> R accept(ElementVisitor<R, P> v, P p);
}
