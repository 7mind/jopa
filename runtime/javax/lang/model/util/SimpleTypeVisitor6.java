package javax.lang.model.util;

import javax.lang.model.type.TypeMirror;

public class SimpleTypeVisitor6<R, P> extends TypeKindVisitor6<R, P> {
    protected SimpleTypeVisitor6() {
        this(null);
    }

    protected SimpleTypeVisitor6(R defaultValue) {
        super(defaultValue);
    }

    @Override
    protected R defaultAction(TypeMirror t, P p) {
        return defaultValue;
    }
}
