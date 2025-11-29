package com.sun.mirror.declaration;

public interface AnnotationTypeElementDeclaration extends MethodDeclaration {
    AnnotationValue getDefaultValue();
    AnnotationTypeDeclaration getDeclaringType();
}
