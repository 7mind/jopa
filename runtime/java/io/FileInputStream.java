package java.io;

public class FileInputStream extends InputStream {
    public FileInputStream(String name) throws FileNotFoundException {}

    public FileInputStream(File file) throws FileNotFoundException {}

    public FileInputStream(FileDescriptor fdObj) {}

    public int read() throws IOException {
        return 0;
    }

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

    public java.nio.channels.FileChannel getChannel() {
        return null;
    }
}
