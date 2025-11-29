package java.io;

public abstract class InputStream implements Closeable {
    public abstract int read() throws IOException;

    public int read(byte[] b) throws IOException {
        return 0;
    }

    public int read(byte[] b, int off, int len) throws IOException {
        return 0;
    }

    public long skip(long n) throws IOException {
        return 0L;
    }

    public int available() throws IOException {
        return 0;
    }

    public void close() throws IOException {}

    public void mark(int readlimit) {}

    public void reset() throws IOException {}

    public boolean markSupported() {
        return false;
    }
}
