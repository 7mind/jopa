package com.sun.mirror.apt;

import java.util.Collection;
import java.util.Map;
import com.sun.mirror.declaration.*;
import com.sun.mirror.util.*;

public interface AnnotationProcessorEnvironment {
    Map<String, String> getOptions();
    Messager getMessager();
    Filer getFiler();
    Collection<TypeDeclaration> getSpecifiedTypeDeclarations();
    PackageDeclaration getPackage(String name);
    TypeDeclaration getTypeDeclaration(String name);
    Collection<TypeDeclaration> getTypeDeclarations();
    Collection<Declaration> getDeclarationsAnnotatedWith(AnnotationTypeDeclaration a);
    Declarations getDeclarationUtils();
    Types getTypeUtils();
    void addListener(AnnotationProcessorListener listener);
    void removeListener(AnnotationProcessorListener listener);
}
