package com.sun.mirror.declaration;

import java.util.Collection;

public interface PackageDeclaration extends Declaration {
    String getQualifiedName();
    Collection<ClassDeclaration> getClasses();
    Collection<EnumDeclaration> getEnums();
    Collection<InterfaceDeclaration> getInterfaces();
    Collection<AnnotationTypeDeclaration> getAnnotationTypes();
}
