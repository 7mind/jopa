package java.io;

public abstract class Reader implements Closeable {
    protected Reader() {}

    protected Reader(Object lock) {}

    public int read() throws IOException {
        return 0;
    }

    public int read(char[] cbuf) throws IOException {
        return 0;
    }

    public abstract int read(char[] cbuf, int off, int len) throws IOException;

    public long skip(long n) throws IOException {
        return 0L;
    }

    public boolean ready() throws IOException {
        return false;
    }

    public boolean markSupported() {
        return false;
    }

    public void mark(int readAheadLimit) throws IOException {}

    public void reset() throws IOException {}

    public abstract void close() throws IOException;
}
