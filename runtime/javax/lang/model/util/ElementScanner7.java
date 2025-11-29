package javax.lang.model.util;

import javax.lang.model.element.*;

public class ElementScanner7<R, P> extends ElementScanner6<R, P> {
    protected ElementScanner7() { super(); }
    protected ElementScanner7(R defaultValue) { super(defaultValue); }

    public R visitVariable(VariableElement e, P p) { return scan(e.getEnclosedElements(), p); }
}
