package com.sun.mirror.declaration; import com.sun.mirror.util.SourcePosition; public interface AnnotationValue { Object getValue(); SourcePosition getPosition(); String toString(); }
