package javax.lang.model.type;

import java.util.List;

public class MirroredTypesException extends RuntimeException {
    private List<? extends TypeMirror> types;

    public MirroredTypesException(List<? extends TypeMirror> types) {
        super("Attempt to access Class objects for TypeMirrors");
        this.types = types;
    }

    public List<? extends TypeMirror> getTypeMirrors() { return types; }
}
