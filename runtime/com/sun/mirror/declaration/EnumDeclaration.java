package com.sun.mirror.declaration; import java.util.Collection; public interface EnumDeclaration extends ClassDeclaration { Collection<EnumConstantDeclaration> getEnumConstants(); }
