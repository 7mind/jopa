package javax.lang.model;

public enum SourceVersion {
    RELEASE_0,
    RELEASE_1,
    RELEASE_2,
    RELEASE_3,
    RELEASE_4,
    RELEASE_5,
    RELEASE_6,
    RELEASE_7,
    RELEASE_8;

    public static SourceVersion latest() { return RELEASE_8; }
    public static SourceVersion latestSupported() { return RELEASE_8; }
    public static boolean isIdentifier(CharSequence name) { return true; }
    public static boolean isName(CharSequence name) { return true; }
    public static boolean isKeyword(CharSequence s) { return false; }
}
