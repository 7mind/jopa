package java.util.zip;

import java.io.IOException;
import java.io.InputStream;

public class ZipInputStream extends InputStream {
    public ZipInputStream(InputStream in) {}

    public ZipEntry getNextEntry() throws IOException {
        return null;
    }

    public void closeEntry() throws IOException {}

    public int available() throws IOException {
        return 0;
    }

    public int read() throws IOException {
        return 0;
    }

    public int read(byte[] b, int off, int len) throws IOException {
        return 0;
    }

    public long skip(long n) throws IOException {
        return 0L;
    }

    public void close() throws IOException {}
}
