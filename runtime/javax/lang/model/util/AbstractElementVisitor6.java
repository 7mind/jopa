package javax.lang.model.util;

import javax.lang.model.element.*;

public abstract class AbstractElementVisitor6<R, P> implements ElementVisitor<R, P> {
    protected AbstractElementVisitor6() {}

    public final R visit(Element e, P p) { return e.accept(this, p); }
    public final R visit(Element e) { return e.accept(this, null); }

    public R visitUnknown(Element e, P p) { throw new RuntimeException("Unknown element kind: " + e.getKind()); }
}
