package com.sun.mirror.util;
import com.sun.mirror.declaration.*;
import com.sun.mirror.type.*;
import java.util.Collection;
public interface Types {
    ArrayType getArrayType(TypeMirror componentType);
    VoidType getVoidType();
    PrimitiveType getPrimitiveType(PrimitiveType.Kind kind);
    DeclaredType getDeclaredType(TypeDeclaration decl, TypeMirror... args);
    DeclaredType getDeclaredType(DeclaredType containing, TypeDeclaration decl, TypeMirror... args);
    WildcardType getWildcardType(Collection<ReferenceType> upperBounds, Collection<ReferenceType> lowerBounds);
}

