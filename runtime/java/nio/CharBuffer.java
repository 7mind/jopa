package java.nio;

public abstract class CharBuffer extends Buffer implements Comparable<CharBuffer>, Appendable, CharSequence {

    // Required by DocRootSlash.java
    public static CharBuffer wrap(CharSequence csq) {
        throw new UnsupportedOperationException();
    }

    // Minimal stubs for CharSequence methods, required for compilation
    @Override
    public int length() {
        throw new UnsupportedOperationException();
    }

    @Override
    public char charAt(int index) {
        throw new UnsupportedOperationException();
    }

    @Override
    public CharSequence subSequence(int start, int end) {
        throw new UnsupportedOperationException();
    }

    // Minimal stub for Appendable method
    @Override
    public Appendable append(char c) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Appendable append(CharSequence csq) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Appendable append(CharSequence csq, int start, int end) {
        throw new UnsupportedOperationException();
    }

    // Minimal stub for Comparable
    @Override
    public int compareTo(CharBuffer that) {
        throw new UnsupportedOperationException();
    }
}
