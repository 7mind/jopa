package java.util.zip;

import java.io.IOException;
import java.io.OutputStream;

public class ZipOutputStream extends OutputStream {
    public static final int STORED = 0;
    public static final int DEFLATED = 8;

    public ZipOutputStream(OutputStream out) {}

    public void setComment(String comment) {}

    public void setMethod(int method) {}

    public void setLevel(int level) {}

    public void putNextEntry(ZipEntry e) throws IOException {}

    public void closeEntry() throws IOException {}

    public void write(int b) throws IOException {}

    public void write(byte[] b, int off, int len) throws IOException {}

    public void finish() throws IOException {}

    public void close() throws IOException {}
}
