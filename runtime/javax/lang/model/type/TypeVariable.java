package javax.lang.model.type;

import javax.lang.model.element.Element;

public interface TypeVariable extends ReferenceType {
    Element asElement();
    TypeMirror getUpperBound();
    TypeMirror getLowerBound();
}
