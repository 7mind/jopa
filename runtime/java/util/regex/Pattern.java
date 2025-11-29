package java.util.regex;

import java.io.Serializable;

public final class Pattern implements Serializable {
    public static final int UNIX_LINES = 0x01;
    public static final int CASE_INSENSITIVE = 0x02;
    public static final int COMMENTS = 0x04;
    public static final int MULTILINE = 0x08;
    public static final int LITERAL = 0x10;
    public static final int DOTALL = 0x20;
    public static final int UNICODE_CASE = 0x40;
    public static final int CANON_EQ = 0x80;
    public static final int UNICODE_CHARACTER_CLASS = 0x100;

    private Pattern() {}

    public static Pattern compile(String regex) { return null; }
    public static Pattern compile(String regex, int flags) { return null; }

    public String pattern() { return null; }
    public String toString() { return null; }
    public Matcher matcher(CharSequence input) { return null; }
    public int flags() { return 0; }
    public static boolean matches(String regex, CharSequence input) { return false; }
    public String[] split(CharSequence input, int limit) { return null; }
    public String[] split(CharSequence input) { return null; }
    public static String quote(String s) { return null; }
}
