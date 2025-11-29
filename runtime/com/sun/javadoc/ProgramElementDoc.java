package com.sun.javadoc;

public interface ProgramElementDoc extends Doc {
    ClassDoc containingClass();
    PackageDoc containingPackage();
    String qualifiedName();
    int modifierSpecifier();
    String modifiers();
    AnnotationDesc[] annotations();
    boolean isPublic();
    boolean isProtected();
    boolean isPrivate();
    boolean isPackagePrivate();
    boolean isStatic();
    boolean isFinal();
}
