package javax.lang.model.type;

public class MirroredTypeException extends RuntimeException {
    private TypeMirror type;

    public MirroredTypeException(TypeMirror type) {
        super("Attempt to access Class object for TypeMirror");
        this.type = type;
    }

    public TypeMirror getTypeMirror() { return type; }
}
