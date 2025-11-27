// Minimal Iterable interface for Java 5 enhanced for-loop support
package java.lang;

public interface Iterable<T> {
    java.util.Iterator<T> iterator();
}
