package java.lang;

public interface CharSequence {
    int length();
    char charAt(int index);
    CharSequence subSequence(int start, int end);
    String toString();
}
