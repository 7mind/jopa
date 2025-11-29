package java.io;

public class ByteArrayOutputStream extends OutputStream {
    public ByteArrayOutputStream() {}
    public ByteArrayOutputStream(int size) {}
    public synchronized void write(int b) {}
    public synchronized void write(byte[] b, int off, int len) {}
    public synchronized void writeTo(OutputStream out) throws IOException {}
    public synchronized void reset() {}
    public synchronized byte[] toByteArray() { return new byte[0]; }
    public synchronized int size() { return 0; }
    public synchronized String toString() { return ""; }
    public synchronized String toString(String charsetName) throws java.io.UnsupportedEncodingException { return ""; }
    public void close() throws IOException {}
}
