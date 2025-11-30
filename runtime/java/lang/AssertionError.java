package java.lang;

public class AssertionError extends Error {
    public AssertionError() {
        super();
    }

    public AssertionError(String message) {
        super(message);
    }

    public AssertionError(Object detailMessage) {
        super(String.valueOf(detailMessage));
    }

    public AssertionError(boolean detailMessage) {
        this(String.valueOf(detailMessage));
    }

    public AssertionError(char detailMessage) {
        this(String.valueOf(detailMessage));
    }

    public AssertionError(int detailMessage) {
        this(String.valueOf(detailMessage));
    }

    public AssertionError(long detailMessage) {
        this(String.valueOf(detailMessage));
    }

    public AssertionError(float detailMessage) {
        this(String.valueOf(detailMessage));
    }

    public AssertionError(double detailMessage) {
        this(String.valueOf(detailMessage));
    }
}
