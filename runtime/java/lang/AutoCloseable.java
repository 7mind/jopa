package java.lang;

/**
 * An object that may hold resources (such as file or socket handles)
 * until it is closed. The close() method of an AutoCloseable object is
 * called automatically when exiting a try-with-resources block for which
 * the object has been declared in the resource specification header.
 * This construction ensures prompt release, avoiding resource exhaustion
 * exceptions and errors that may otherwise occur.
 *
 * @since 1.7
 */
public interface AutoCloseable {
    /**
     * Closes this resource, relinquishing any underlying resources.
     * This method is invoked automatically on objects managed by the
     * try-with-resources statement.
     *
     * @throws Exception if this resource cannot be closed
     */
    void close() throws Exception;
}
