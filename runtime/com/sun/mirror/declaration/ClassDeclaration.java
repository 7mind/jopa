package com.sun.mirror.declaration;

import java.util.Collection;
import com.sun.mirror.type.ClassType;

public interface ClassDeclaration extends TypeDeclaration {
    ClassType getSuperclass();
    Collection<ConstructorDeclaration> getConstructors();
    Collection<? extends MethodDeclaration> getMethods();
}
