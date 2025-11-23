// Minimal Iterator interface for Java 5 enhanced for-loop support
package java.util;

public interface Iterator {
    boolean hasNext();
    Object next();
    void remove();
}
