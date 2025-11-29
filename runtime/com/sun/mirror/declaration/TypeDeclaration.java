package com.sun.mirror.declaration;

import java.util.Collection;
import com.sun.mirror.type.DeclaredType;
import com.sun.mirror.type.InterfaceType;
import com.sun.mirror.util.DeclarationVisitor;

public interface TypeDeclaration extends MemberDeclaration {
    PackageDeclaration getPackage();
    String getQualifiedName();
    Collection<TypeParameterDeclaration> getFormalTypeParameters();
    Collection<InterfaceType> getSuperinterfaces();
    Collection<FieldDeclaration> getFields();
    Collection<? extends MethodDeclaration> getMethods();
    Collection<TypeDeclaration> getNestedTypes();
    void accept(DeclarationVisitor v);
}
