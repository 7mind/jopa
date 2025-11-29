package java.io;

public class StringWriter extends Writer {
    public StringWriter() {}
    public StringWriter(int initialSize) {}

    public void write(int c) {}
    public void write(char[] cbuf, int off, int len) {}
    public void write(String str) {}
    public void write(String str, int off, int len) {}

    public StringWriter append(CharSequence csq) { return this; }
    public StringWriter append(CharSequence csq, int start, int end) { return this; }
    public StringWriter append(char c) { return this; }

    public String toString() { return ""; }
    public StringBuffer getBuffer() { return null; }

    public void flush() {}
    public void close() {}
}
