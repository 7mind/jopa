package com.sun.mirror.util;

import com.sun.mirror.declaration.*;

public class SimpleDeclarationVisitor implements DeclarationVisitor {
    public SimpleDeclarationVisitor() {}
    public void visitDeclaration(Declaration d) {}
    public void visitPackageDeclaration(PackageDeclaration d) { visitDeclaration(d); }
    public void visitMemberDeclaration(MemberDeclaration d) { visitDeclaration(d); }
    public void visitTypeDeclaration(TypeDeclaration d) { visitMemberDeclaration(d); }
    public void visitClassDeclaration(ClassDeclaration d) { visitTypeDeclaration(d); }
    public void visitEnumDeclaration(EnumDeclaration d) { visitClassDeclaration(d); }
    public void visitInterfaceDeclaration(InterfaceDeclaration d) { visitTypeDeclaration(d); }
    public void visitAnnotationTypeDeclaration(AnnotationTypeDeclaration d) { visitInterfaceDeclaration(d); }
    public void visitFieldDeclaration(FieldDeclaration d) { visitMemberDeclaration(d); }
    public void visitEnumConstantDeclaration(EnumConstantDeclaration d) { visitFieldDeclaration(d); }
    public void visitExecutableDeclaration(ExecutableDeclaration d) { visitMemberDeclaration(d); }
    public void visitConstructorDeclaration(ConstructorDeclaration d) { visitExecutableDeclaration(d); }
    public void visitMethodDeclaration(MethodDeclaration d) { visitExecutableDeclaration(d); }
    public void visitAnnotationTypeElementDeclaration(AnnotationTypeElementDeclaration d) { visitMethodDeclaration(d); }
    public void visitParameterDeclaration(ParameterDeclaration d) { visitDeclaration(d); }
    public void visitTypeParameterDeclaration(TypeParameterDeclaration d) { visitDeclaration(d); }
}
