package java.security;

public class InvalidKeyException extends java.security.spec.InvalidKeySpecException {
    public InvalidKeyException() {}
    public InvalidKeyException(String msg) { super(msg); }
    public InvalidKeyException(String message, Throwable cause) { super(message, cause); }
    public InvalidKeyException(Throwable cause) { super(cause); }
}
