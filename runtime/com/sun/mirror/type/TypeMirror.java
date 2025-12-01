package com.sun.mirror.type; public interface TypeMirror { String toString(); boolean equals(Object obj); void accept(TypeVisitor v); }
