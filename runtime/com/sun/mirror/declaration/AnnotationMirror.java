package com.sun.mirror.declaration;

import java.util.Map;
import com.sun.mirror.type.AnnotationType;
import com.sun.mirror.util.SourcePosition;

public interface AnnotationMirror {
    AnnotationType getAnnotationType();
    Map<AnnotationTypeElementDeclaration, AnnotationValue> getElementValues();
    SourcePosition getPosition();
}
