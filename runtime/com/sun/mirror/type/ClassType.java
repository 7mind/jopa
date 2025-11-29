package com.sun.mirror.type;

import com.sun.mirror.declaration.ClassDeclaration;

public interface ClassType extends DeclaredType {
    ClassDeclaration getDeclaration();
    ClassType getSuperclass();
}
