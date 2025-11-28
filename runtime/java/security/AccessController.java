package java.security;

public final class AccessController {
    private AccessController() {}

    public static <T> T doPrivileged(PrivilegedAction<T> action) {
        return action.run();
    }
}
