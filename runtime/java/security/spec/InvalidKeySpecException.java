package java.security.spec;

import java.security.GeneralSecurityException;

public class InvalidKeySpecException extends GeneralSecurityException {
    public InvalidKeySpecException() {}
    public InvalidKeySpecException(String msg) { super(msg); }
    public InvalidKeySpecException(String message, Throwable cause) { super(message, cause); }
    public InvalidKeySpecException(Throwable cause) { super(cause); }
}
