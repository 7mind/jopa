package com.sun.mirror.util;

import com.sun.mirror.type.*;

public class SimpleTypeVisitor implements TypeVisitor {
    public SimpleTypeVisitor() {}
    public void visitTypeMirror(TypeMirror t) {}
    public void visitPrimitiveType(PrimitiveType t) { visitTypeMirror(t); }
    public void visitVoidType(VoidType t) { visitTypeMirror(t); }
    public void visitReferenceType(ReferenceType t) { visitTypeMirror(t); }
    public void visitDeclaredType(DeclaredType t) { visitReferenceType(t); }
    public void visitClassType(ClassType t) { visitDeclaredType(t); }
    public void visitEnumType(EnumType t) { visitClassType(t); }
    public void visitInterfaceType(InterfaceType t) { visitDeclaredType(t); }
    public void visitAnnotationType(AnnotationType t) { visitInterfaceType(t); }
    public void visitArrayType(ArrayType t) { visitReferenceType(t); }
    public void visitTypeVariable(TypeVariable t) { visitReferenceType(t); }
    public void visitWildcardType(WildcardType t) { visitTypeMirror(t); }
}
