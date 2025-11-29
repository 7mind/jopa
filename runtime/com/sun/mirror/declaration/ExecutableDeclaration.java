package com.sun.mirror.declaration;

import java.util.Collection;
import com.sun.mirror.type.ReferenceType;

public interface ExecutableDeclaration extends MemberDeclaration {
    boolean isVarArgs();
    Collection<ParameterDeclaration> getParameters();
    Collection<ReferenceType> getThrownTypes();
    Collection<TypeParameterDeclaration> getFormalTypeParameters();
}
