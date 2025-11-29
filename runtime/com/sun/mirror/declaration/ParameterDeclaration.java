package com.sun.mirror.declaration;

import com.sun.mirror.type.TypeMirror;

public interface ParameterDeclaration extends Declaration {
    TypeMirror getType();
}
