package javax.lang.model.element;

public interface ElementVisitor<R, P> {
    R visit(Element e, P p);
    R visit(Element e);
    R visitPackage(PackageElement e, P p);
    R visitType(TypeElement e, P p);
    R visitVariable(VariableElement e, P p);
    R visitExecutable(ExecutableElement e, P p);
    R visitTypeParameter(TypeParameterElement e, P p);
    R visitUnknown(Element e, P p);
}
