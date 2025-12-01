package com.sun.mirror.declaration; import java.util.Collection; public interface AnnotationTypeDeclaration extends InterfaceDeclaration { Collection<AnnotationTypeElementDeclaration> getMethods(); }
