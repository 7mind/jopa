package java.util.jar;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.zip.ZipFile;

public class JarFile extends ZipFile {
    public static final String MANIFEST_NAME = "META-INF/MANIFEST.MF";

    public JarFile(String name) throws IOException {
        super(name);
    }

    public JarFile(String name, boolean verify) throws IOException {
        super(name);
    }

    public JarFile(File file) throws IOException {
        super(file);
    }

    public JarFile(File file, boolean verify) throws IOException {
        super(file);
    }

    public JarFile(File file, boolean verify, int mode) throws IOException {
        super(file, mode);
    }

    public Manifest getManifest() throws IOException {
        return null;
    }

    public JarEntry getJarEntry(String name) {
        return null;
    }

    public Enumeration<JarEntry> entries() {
        return null;
    }

    public InputStream getInputStream(java.util.zip.ZipEntry ze) throws IOException {
        return null;
    }
}
