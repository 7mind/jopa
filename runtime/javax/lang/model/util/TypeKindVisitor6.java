package javax.lang.model.util;

import javax.lang.model.type.ArrayType;
import javax.lang.model.type.DeclaredType;
import javax.lang.model.type.ExecutableType;
import javax.lang.model.type.IntersectionType;
import javax.lang.model.type.NoType;
import javax.lang.model.type.NullType;
import javax.lang.model.type.PrimitiveType;
import javax.lang.model.type.TypeKind;
import javax.lang.model.type.TypeMirror;
import javax.lang.model.type.TypeVariable;
import javax.lang.model.type.UnionType;
import javax.lang.model.type.UnknownTypeException;
import javax.lang.model.type.WildcardType;

public class TypeKindVisitor6<R, P> implements javax.lang.model.type.TypeVisitor<R, P> {
    protected final R defaultValue;

    protected TypeKindVisitor6() {
        this(null);
    }

    protected TypeKindVisitor6(R defaultValue) {
        this.defaultValue = defaultValue;
    }

    public R visit(TypeMirror t, P p) {
        if (t == null) {
            return defaultValue;
        }
        TypeKind kind = t.getKind();
        switch (kind) {
            case BOOLEAN:
            case BYTE:
            case SHORT:
            case INT:
            case LONG:
            case CHAR:
            case FLOAT:
            case DOUBLE:
                return visitPrimitive((PrimitiveType) t, p);
            case VOID:
            case NONE:
            case PACKAGE:
                return visitNoType((NoType) t, p);
            case NULL:
                return visitNull((NullType) t, p);
            case ARRAY:
                return visitArray((ArrayType) t, p);
            case DECLARED:
            case ERROR:
                return visitDeclared((DeclaredType) t, p);
            case TYPEVAR:
                return visitTypeVariable((TypeVariable) t, p);
            case WILDCARD:
                return visitWildcard((WildcardType) t, p);
            case EXECUTABLE:
                return visitExecutable((ExecutableType) t, p);
            case INTERSECTION:
                return visitIntersection((IntersectionType) t, p);
            case UNION:
                return visitUnion((UnionType) t, p);
            default:
                return visitUnknown(t, p);
        }
    }

    public R visit(TypeMirror t) {
        return visit(t, null);
    }

    protected R defaultAction(TypeMirror t, P p) {
        return defaultValue;
    }

    public R visitPrimitive(PrimitiveType t, P p) {
        return defaultAction(t, p);
    }

    public R visitNull(NullType t, P p) {
        return defaultAction(t, p);
    }

    public R visitArray(ArrayType t, P p) {
        return defaultAction(t, p);
    }

    public R visitDeclared(DeclaredType t, P p) {
        return defaultAction(t, p);
    }

    public R visitError(DeclaredType t, P p) {
        return visitDeclared(t, p);
    }

    public R visitTypeVariable(TypeVariable t, P p) {
        return defaultAction(t, p);
    }

    public R visitWildcard(WildcardType t, P p) {
        return defaultAction(t, p);
    }

    public R visitExecutable(ExecutableType t, P p) {
        return defaultAction(t, p);
    }

    public R visitNoType(NoType t, P p) {
        return defaultAction(t, p);
    }

    public R visitIntersection(IntersectionType t, P p) {
        throw new UnknownTypeException(t, p);
    }

    public R visitUnion(UnionType t, P p) {
        throw new UnknownTypeException(t, p);
    }

    public R visitUnknown(TypeMirror t, P p) {
        throw new UnknownTypeException(t, p);
    }
}
