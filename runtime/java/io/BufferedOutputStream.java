package java.io;

public class BufferedOutputStream extends OutputStream {
    public BufferedOutputStream(OutputStream out) {}

    public BufferedOutputStream(OutputStream out, int size) {}

    public void write(int b) throws IOException {}

    public void write(byte[] b, int off, int len) throws IOException {}

    public void flush() throws IOException {}
}
