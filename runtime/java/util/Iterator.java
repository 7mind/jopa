// Minimal Iterator interface for Java 5 enhanced for-loop support
package java.util;

public interface Iterator<E> {
    boolean hasNext();
    E next();
    void remove();
}
