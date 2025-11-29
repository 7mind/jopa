package java.lang;

public class ClassNotFoundException extends ReflectiveOperationException {
    public ClassNotFoundException() {
        super();
    }

    public ClassNotFoundException(String s) {
        super(s);
    }

    public ClassNotFoundException(String s, Throwable ex) {
        super(s, ex);
    }
}
