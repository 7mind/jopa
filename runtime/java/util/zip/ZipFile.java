package java.util.zip;

import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.Enumeration;

public class ZipFile implements ZipConstants, Closeable {
    public static final int OPEN_READ = 1;
    public static final int OPEN_DELETE = 4;

    public ZipFile(String name) throws IOException {}
    public ZipFile(File file, int mode) throws IOException {}
    public ZipFile(File file) throws ZipException, IOException {}

    public ZipEntry getEntry(String name) { return null; }
    public InputStream getInputStream(ZipEntry entry) throws IOException { return null; }
    public String getName() { return null; }
    public Enumeration<? extends ZipEntry> entries() { return null; }
    public int size() { return 0; }
    public void close() throws IOException {}

    protected void finalize() throws IOException {}
}
