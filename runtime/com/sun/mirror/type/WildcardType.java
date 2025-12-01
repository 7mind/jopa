package com.sun.mirror.type; import java.util.Collection; public interface WildcardType extends TypeMirror { Collection<ReferenceType> getUpperBounds(); Collection<ReferenceType> getLowerBounds(); }
