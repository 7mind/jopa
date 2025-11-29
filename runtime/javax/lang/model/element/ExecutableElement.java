package javax.lang.model.element;

import java.util.List;
import javax.lang.model.type.TypeMirror;

public interface ExecutableElement extends Element {
    List<? extends TypeParameterElement> getTypeParameters();
    TypeMirror getReturnType();
    List<? extends VariableElement> getParameters();
    boolean isVarArgs();
    List<? extends TypeMirror> getThrownTypes();
    AnnotationValue getDefaultValue();
}
