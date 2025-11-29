package java.io;

public class BufferedInputStream extends InputStream {
    public BufferedInputStream(InputStream in) {}

    public BufferedInputStream(InputStream in, int size) {}

    public int read() throws IOException {
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

    public void mark(int readlimit) {}

    public void reset() throws IOException {}

    public boolean markSupported() {
        return true;
    }

    public void close() throws IOException {}
}
