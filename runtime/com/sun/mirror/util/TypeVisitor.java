package com.sun.mirror.util;

import com.sun.mirror.type.*;

public interface TypeVisitor {
    void visitTypeMirror(TypeMirror t);
    void visitPrimitiveType(PrimitiveType t);
    void visitVoidType(VoidType t);
    void visitReferenceType(ReferenceType t);
    void visitDeclaredType(DeclaredType t);
    void visitClassType(ClassType t);
    void visitEnumType(EnumType t);
    void visitInterfaceType(InterfaceType t);
    void visitAnnotationType(AnnotationType t);
    void visitArrayType(ArrayType t);
    void visitTypeVariable(TypeVariable t);
    void visitWildcardType(WildcardType t);
}
