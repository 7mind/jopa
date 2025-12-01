package com.sun.mirror.type;
public class MirroredTypeException extends RuntimeException {
    public MirroredTypeException(TypeMirror type) {}
    public TypeMirror getTypeMirror() { return null; }
    public String getQualifiedName() { return null; }
}
