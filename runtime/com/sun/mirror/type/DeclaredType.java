package com.sun.mirror.type;

import java.util.Collection;
import com.sun.mirror.declaration.TypeDeclaration;

public interface DeclaredType extends ReferenceType {
    TypeDeclaration getDeclaration();
    DeclaredType getContainingType();
    Collection<TypeMirror> getActualTypeArguments();
    Collection<InterfaceType> getSuperinterfaces();
}
