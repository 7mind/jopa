package javax.lang.model.type;

public class UnknownTypeException extends RuntimeException {
    private static final long serialVersionUID = 1L;
    private final transient TypeMirror type;
    private final transient Object argument;

    public UnknownTypeException(TypeMirror t, Object p) {
        this.type = t;
        this.argument = p;
    }

    public TypeMirror getUnknownType() {
        return type;
    }

    public Object getArgument() {
        return argument;
    }
}
