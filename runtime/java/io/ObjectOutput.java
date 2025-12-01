package java.io;
public interface ObjectOutput extends DataOutput, AutoCloseable {
    public void writeObject(Object obj) throws IOException;
    public void write(int b) throws IOException;
    public void write(byte b[]) throws IOException;
    public void write(byte b[], int off, int len) throws IOException;
    public void flush() throws IOException;
    public void close() throws IOException;
}
