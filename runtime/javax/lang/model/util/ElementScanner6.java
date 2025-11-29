package javax.lang.model.util;

import javax.lang.model.element.*;

public class ElementScanner6<R, P> extends AbstractElementVisitor6<R, P> {
    protected R DEFAULT_VALUE;

    protected ElementScanner6() { DEFAULT_VALUE = null; }
    protected ElementScanner6(R defaultValue) { DEFAULT_VALUE = defaultValue; }

    public final R scan(Iterable<? extends Element> iterable, P p) {
        R result = DEFAULT_VALUE;
        for (Element e : iterable) result = scan(e, p);
        return result;
    }

    public R scan(Element e, P p) { return e.accept(this, p); }
    public final R scan(Element e) { return scan(e, null); }

    public R visitPackage(PackageElement e, P p) { return scan(e.getEnclosedElements(), p); }
    public R visitType(TypeElement e, P p) { return scan(e.getEnclosedElements(), p); }
    public R visitVariable(VariableElement e, P p) { return scan(e.getEnclosedElements(), p); }
    public R visitExecutable(ExecutableElement e, P p) { return scan(e.getParameters(), p); }
    public R visitTypeParameter(TypeParameterElement e, P p) { return scan(e.getEnclosedElements(), p); }
}
