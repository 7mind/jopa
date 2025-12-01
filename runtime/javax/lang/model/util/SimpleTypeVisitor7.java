package javax.lang.model.util;

public class SimpleTypeVisitor7<R, P> extends TypeKindVisitor7<R, P> {
    protected SimpleTypeVisitor7() {
        this(null);
    }

    protected SimpleTypeVisitor7(R defaultValue) {
        super(defaultValue);
    }

    @Override
    protected R defaultAction(javax.lang.model.type.TypeMirror t, P p) {
        return defaultValue;
    }
}
