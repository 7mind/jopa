package javax.lang.model.type;

public interface TypeMirror {
    TypeKind getKind();
    <R, P> R accept(TypeVisitor<R, P> v, P p);
}
