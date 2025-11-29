package javax.lang.model.element;

import java.util.List;
import javax.lang.model.type.TypeMirror;

public interface TypeElement extends Element {
    Name getQualifiedName();
    TypeMirror getSuperclass();
    List<? extends TypeMirror> getInterfaces();
    List<? extends TypeParameterElement> getTypeParameters();
    NestingKind getNestingKind();
}
