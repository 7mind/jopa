package com.sun.mirror.type;

public interface PrimitiveType extends TypeMirror {
    Kind getKind();

    enum Kind {
        BOOLEAN, BYTE, SHORT, INT, LONG, CHAR, FLOAT, DOUBLE
    }
}
