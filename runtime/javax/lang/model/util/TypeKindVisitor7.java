package javax.lang.model.util;

import javax.lang.model.type.IntersectionType;
import javax.lang.model.type.NoType;
import javax.lang.model.type.TypeKind;
import javax.lang.model.type.TypeMirror;
import javax.lang.model.type.UnionType;

public class TypeKindVisitor7<R, P> extends TypeKindVisitor6<R, P> {
    protected TypeKindVisitor7() {
        this(null);
    }

    protected TypeKindVisitor7(R defaultValue) {
        super(defaultValue);
    }

    @Override
    public R visitNoType(NoType t, P p) {
        if (t == null) {
            return defaultValue;
        }
        TypeKind kind = t.getKind();
        if (kind == TypeKind.VOID) {
            return visitNoTypeAsVoid(t, p);
        }
        if (kind == TypeKind.PACKAGE) {
            return visitNoTypeAsPackage(t, p);
        }
        if (kind == TypeKind.NONE) {
            return visitNoTypeAsNone(t, p);
        }
        return defaultAction(t, p);
    }

    public R visitNoTypeAsVoid(NoType t, P p) {
        return defaultAction(t, p);
    }

    public R visitNoTypeAsPackage(NoType t, P p) {
        return defaultAction(t, p);
    }

    public R visitNoTypeAsNone(NoType t, P p) {
        return defaultAction(t, p);
    }

    @Override
    public R visitUnion(UnionType t, P p) {
        return defaultAction(t, p);
    }

    @Override
    public R visitIntersection(IntersectionType t, P p) {
        return defaultAction(t, p);
    }
}
