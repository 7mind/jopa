package com.sun.mirror.apt;

import java.util.Collection;
import java.util.Set;
import com.sun.mirror.declaration.AnnotationTypeDeclaration;

public interface AnnotationProcessorFactory {
    Collection<String> supportedOptions();
    Collection<String> supportedAnnotationTypes();
    AnnotationProcessor getProcessorFor(Set<AnnotationTypeDeclaration> atds, AnnotationProcessorEnvironment env);
}
