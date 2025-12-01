package javax.lang.model.util;

public class SimpleTypeVisitor8<R, P> extends TypeKindVisitor8<R, P> {
    protected SimpleTypeVisitor8() {
        this(null);
    }

    protected SimpleTypeVisitor8(R defaultValue) {
        super(defaultValue);
    }

    @Override
    protected R defaultAction(javax.lang.model.type.TypeMirror t, P p) {
        return defaultValue;
    }
}
