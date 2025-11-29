package java.security;

public class NoSuchAlgorithmException extends GeneralSecurityException {
    public NoSuchAlgorithmException() {}
    public NoSuchAlgorithmException(String msg) { super(msg); }
    public NoSuchAlgorithmException(String message, Throwable cause) { super(message, cause); }
    public NoSuchAlgorithmException(Throwable cause) { super(cause); }
}
