package com.sun.mirror.type;

public interface ArrayType extends ReferenceType {
    TypeMirror getComponentType();
}
