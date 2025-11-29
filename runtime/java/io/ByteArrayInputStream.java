package java.io;

public class ByteArrayInputStream extends InputStream {
    protected byte[] buf;
    protected int pos;
    protected int mark;
    protected int count;

    public ByteArrayInputStream(byte[] buf) {}
    public ByteArrayInputStream(byte[] buf, int offset, int length) {}
    public synchronized int read() { return 0; }
    public synchronized int read(byte[] b, int off, int len) { return 0; }
    public synchronized long skip(long n) { return 0; }
    public synchronized int available() { return 0; }
    public boolean markSupported() { return true; }
    public void mark(int readAheadLimit) {}
    public synchronized void reset() {}
    public void close() throws IOException {}
}
