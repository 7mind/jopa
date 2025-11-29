package java.util.jar;

import java.io.IOException;
import java.io.InputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class JarInputStream extends ZipInputStream {
    public JarInputStream(InputStream in) throws IOException {
        super(in);
    }

    public JarInputStream(InputStream in, boolean verify) throws IOException {
        super(in);
    }

    public Manifest getManifest() {
        return null;
    }

    public ZipEntry getNextEntry() throws IOException {
        return null;
    }

    public JarEntry getNextJarEntry() throws IOException {
        return null;
    }

    public int read(byte[] b, int off, int len) throws IOException {
        return 0;
    }
}
