package javax.lang.model.type;

import java.util.List;

public interface ExecutableType extends TypeMirror {
    List<? extends TypeVariable> getTypeVariables();
    TypeMirror getReturnType();
    List<? extends TypeMirror> getParameterTypes();
    List<? extends TypeMirror> getThrownTypes();
}
