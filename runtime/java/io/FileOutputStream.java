package java.io;

public class FileOutputStream extends OutputStream {
    public FileOutputStream(String name) throws FileNotFoundException {}

    public FileOutputStream(String name, boolean append) throws FileNotFoundException {}

    public FileOutputStream(File file) throws FileNotFoundException {}

    public FileOutputStream(File file, boolean append) throws FileNotFoundException {}

    public void write(int b) throws IOException {}

    public void write(byte[] b) throws IOException {}

    public void write(byte[] b, int off, int len) throws IOException {}

    public void close() throws IOException {}

    public java.nio.channels.FileChannel getChannel() {
        return null;
    }
}
