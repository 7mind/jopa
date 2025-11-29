package java.lang.reflect;

import java.lang.annotation.Annotation;

public interface AnnotatedElement {
    <T extends Annotation> T getAnnotation(Class<T> annotationClass);
    Annotation[] getAnnotations();
    Annotation[] getDeclaredAnnotations();
    boolean isAnnotationPresent(Class<? extends Annotation> annotationClass);
}
