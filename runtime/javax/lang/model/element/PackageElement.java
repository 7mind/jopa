package javax.lang.model.element;

public interface PackageElement extends Element {
    Name getQualifiedName();
    boolean isUnnamed();
}
