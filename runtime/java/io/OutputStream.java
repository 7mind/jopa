package java.io;

public abstract class OutputStream implements Closeable, Flushable {
    public abstract void write(int b) throws IOException;

    public void write(byte[] b) throws IOException {}

    public void write(byte[] b, int off, int len) throws IOException {}

    public void flush() throws IOException {}

    public void close() throws IOException {}
}
