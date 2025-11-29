package javax.annotation.processing;

import javax.lang.model.element.AnnotationMirror;
import javax.lang.model.element.AnnotationValue;
import javax.lang.model.element.Element;
import javax.tools.Diagnostic;

public interface Messager {
    void printMessage(Diagnostic.Kind kind, CharSequence msg);
    void printMessage(Diagnostic.Kind kind, CharSequence msg, Element e);
    void printMessage(Diagnostic.Kind kind, CharSequence msg, Element e, AnnotationMirror a);
    void printMessage(Diagnostic.Kind kind, CharSequence msg, Element e, AnnotationMirror a, AnnotationValue v);
}
