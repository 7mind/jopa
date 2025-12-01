package java.util;

public class StringTokenizer implements Enumeration<Object> {
    private String str;
    private String delimiters;
    private boolean retTokens;
    private int cursor;

    public StringTokenizer(String str, String delimiters, boolean retTokens) {
        this.str = str;
        this.delimiters = delimiters;
        this.retTokens = retTokens;
        this.cursor = 0;
    }

    public StringTokenizer(String str, String delimiters) {
        this(str, delimiters, false);
    }

    public StringTokenizer(String str) {
        this(str, " 	\n\r\f", false);
    }

    public boolean hasMoreTokens() {
        return false; // Stub
    }

    public String nextToken() {
        return null; // Stub
    }

    public String nextToken(String delim) {
        return null; // Stub
    }

    public boolean hasMoreElements() {
        return hasMoreTokens();
    }

    public Object nextElement() {
        return nextToken();
    }

    public int countTokens() {
        return 0; // Stub
    }
}
