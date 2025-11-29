package com.sun.mirror.util;

import java.util.Collection;
import com.sun.mirror.declaration.*;
import com.sun.mirror.type.*;

public interface Types {
    boolean isSubtype(TypeMirror t1, TypeMirror t2);
    boolean isAssignable(TypeMirror t1, TypeMirror t2);
    TypeMirror getErasure(TypeMirror t);
    PrimitiveType getPrimitiveType(PrimitiveType.Kind kind);
    VoidType getVoidType();
    ArrayType getArrayType(TypeMirror componentType);
    TypeVariable getTypeVariable(TypeParameterDeclaration tparam);
    WildcardType getWildcardType(Collection<ReferenceType> upperBounds, Collection<ReferenceType> lowerBounds);
    DeclaredType getDeclaredType(TypeDeclaration decl, TypeMirror... typeArgs);
    DeclaredType getDeclaredType(DeclaredType containing, TypeDeclaration decl, TypeMirror... typeArgs);
}
