package java.io;

public abstract class Writer implements Closeable, Flushable, Appendable {
    protected Writer() {}

    protected Writer(Object lock) {}

    public void write(int c) throws IOException {}

    public void write(char[] cbuf) throws IOException {}

    public abstract void write(char[] cbuf, int off, int len) throws IOException;

    public void write(String str) throws IOException {}

    public void write(String str, int off, int len) throws IOException {}

    public Writer append(CharSequence csq) throws IOException {
        return this;
    }

    public Writer append(CharSequence csq, int start, int end) throws IOException {
        return this;
    }

    public Writer append(char c) throws IOException {
        return this;
    }

    public abstract void flush() throws IOException;

    public abstract void close() throws IOException;
}
