package java.util.jar;

import java.io.IOException;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public class JarOutputStream extends ZipOutputStream {
    public JarOutputStream(OutputStream out) throws IOException {
        super(out);
    }

    public JarOutputStream(OutputStream out, Manifest man) throws IOException {
        super(out);
    }

    public void putNextEntry(ZipEntry ze) throws IOException {}
}
