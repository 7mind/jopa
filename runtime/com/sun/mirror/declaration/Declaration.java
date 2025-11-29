package com.sun.mirror.declaration;

import java.util.Collection;
import com.sun.mirror.util.SourcePosition;

public interface Declaration {
    String getDocComment();
    Collection<AnnotationMirror> getAnnotationMirrors();
    <A extends java.lang.annotation.Annotation> A getAnnotation(Class<A> annotationType);
    Collection<Modifier> getModifiers();
    String getSimpleName();
    SourcePosition getPosition();
}
