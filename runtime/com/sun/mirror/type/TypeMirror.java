package com.sun.mirror.type;

import com.sun.mirror.util.TypeVisitor;

public interface TypeMirror {
    void accept(TypeVisitor v);
}
