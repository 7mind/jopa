package javax.lang.model.util;

import java.util.ArrayList;
import java.util.List;
import javax.lang.model.element.Element;
import javax.lang.model.element.ElementKind;
import javax.lang.model.element.ExecutableElement;
import javax.lang.model.element.PackageElement;
import javax.lang.model.element.TypeElement;

public class ElementFilter {
    private ElementFilter() {
    }

    public static <E extends Element> List<E> typesIn(Iterable<? extends Element> elements) {
        return filter(elements, ElementKind.CLASS, ElementKind.ENUM, ElementKind.ANNOTATION_TYPE, ElementKind.INTERFACE);
    }

    public static List<ExecutableElement> methodsIn(Iterable<? extends Element> elements) {
        return filter(elements, ElementKind.METHOD);
    }

    public static List<ExecutableElement> constructorsIn(Iterable<? extends Element> elements) {
        return filter(elements, ElementKind.CONSTRUCTOR);
    }

    public static List<PackageElement> packagesIn(Iterable<? extends Element> elements) {
        return filter(elements, ElementKind.PACKAGE);
    }

    private static <E extends Element> List<E> filter(Iterable<? extends Element> elements, ElementKind kind, ElementKind... more) {
        List<E> result = new ArrayList<E>();
        for (Element element : elements) {
            if (element != null && (element.getKind() == kind || matches(element.getKind(), more))) {
                @SuppressWarnings("unchecked")
                E cast = (E) element;
                result.add(cast);
            }
        }
        return result;
    }

    private static boolean matches(ElementKind candidate, ElementKind[] kinds) {
        for (ElementKind k : kinds) {
            if (candidate == k) {
                return true;
            }
        }
        return false;
    }
}
