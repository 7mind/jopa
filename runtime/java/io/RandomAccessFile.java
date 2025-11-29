package java.io;

public class RandomAccessFile implements Closeable {
    public RandomAccessFile(String name, String mode) throws FileNotFoundException {}

    public RandomAccessFile(File file, String mode) throws FileNotFoundException {}

    public int read() throws IOException {
        return 0;
    }

    public int read(byte[] b) throws IOException {
        return 0;
    }

    public int read(byte[] b, int off, int len) throws IOException {
        return 0;
    }

    public void write(int b) throws IOException {}

    public void write(byte[] b) throws IOException {}

    public void write(byte[] b, int off, int len) throws IOException {}

    public void seek(long pos) throws IOException {}

    public long getFilePointer() throws IOException {
        return 0L;
    }

    public long length() throws IOException {
        return 0L;
    }

    public void setLength(long newLength) throws IOException {}

    public void close() throws IOException {}

    public java.nio.channels.FileChannel getChannel() {
        return null;
    }
}
