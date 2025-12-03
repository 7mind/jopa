package java.lang;

public class Throwable {
    private String detailMessage;
    private Throwable cause;
    private Throwable[] suppressedExceptions;

    public Throwable() {
        this.cause = this;
    }

    public Throwable(String message) {
        this.detailMessage = message;
        this.cause = this;
    }

    public Throwable(String message, Throwable cause) {
        this.detailMessage = message;
        this.cause = cause;
    }

    public Throwable(Throwable cause) {
        this.detailMessage = (cause == null) ? null : cause.toString();
        this.cause = cause;
    }

    public String getMessage() {
        return detailMessage;
    }

    public String getLocalizedMessage() {
        return getMessage();
    }

    public Throwable getCause() {
        return (cause == this) ? null : cause;
    }

    public String toString() {
        String message = getMessage();
        String className = getClass().getName();
        return (message != null) ? (className + ": " + message) : className;
    }

    public void printStackTrace() {}

    /**
     * Appends the specified exception to the exceptions that were
     * suppressed in order to deliver this exception. This method is
     * thread-safe and typically called (automatically and implicitly)
     * by the try-with-resources statement.
     *
     * @param exception the exception to be added to the list of
     *        suppressed exceptions
     * @since 1.7
     */
    public final void addSuppressed(Throwable exception) {
        if (exception == this) {
            throw new IllegalArgumentException("Self-suppression not permitted");
        }
        if (exception == null) {
            throw new NullPointerException("Cannot suppress a null exception");
        }
        // Simplified implementation - in real JDK this would use atomic operations
        if (suppressedExceptions == null) {
            suppressedExceptions = new Throwable[1];
            suppressedExceptions[0] = exception;
        } else {
            Throwable[] newArray = new Throwable[suppressedExceptions.length + 1];
            for (int i = 0; i < suppressedExceptions.length; i++) {
                newArray[i] = suppressedExceptions[i];
            }
            newArray[suppressedExceptions.length] = exception;
            suppressedExceptions = newArray;
        }
    }

    /**
     * Returns an array containing all of the exceptions that were
     * suppressed, typically by the try-with-resources statement, in
     * order to deliver this exception.
     *
     * @return an array containing all of the exceptions that were
     *         suppressed to deliver this exception. The array is empty
     *         if no exceptions were suppressed or suppression is disabled.
     * @since 1.7
     */
    public final Throwable[] getSuppressed() {
        if (suppressedExceptions == null) {
            return new Throwable[0];
        } else {
            Throwable[] result = new Throwable[suppressedExceptions.length];
            for (int i = 0; i < suppressedExceptions.length; i++) {
                result[i] = suppressedExceptions[i];
            }
            return result;
        }
    }
}
