package com.sun.mirror.type;

import com.sun.mirror.declaration.EnumDeclaration;

public interface EnumType extends ClassType {
    EnumDeclaration getDeclaration();
}
