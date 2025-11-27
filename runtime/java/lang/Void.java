package java.lang;

public final class Void {
    public static final Class TYPE = getPrimitiveClass("void");
    private static native Class getPrimitiveClass(String name);

    private Void() {}
}
