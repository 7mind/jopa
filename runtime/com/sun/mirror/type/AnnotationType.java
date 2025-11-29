package com.sun.mirror.type;

import com.sun.mirror.declaration.AnnotationTypeDeclaration;

public interface AnnotationType extends InterfaceType {
    AnnotationTypeDeclaration getDeclaration();
}
