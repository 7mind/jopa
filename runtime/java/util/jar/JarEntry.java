package java.util.jar;

import java.util.zip.ZipEntry;

public class JarEntry extends ZipEntry {
    public JarEntry(String name) {
        super(name);
    }

    public JarEntry(ZipEntry ze) {
        super(ze);
    }

    public JarEntry(JarEntry je) {
        super(je);
    }

    public Attributes getAttributes() throws java.io.IOException {
        return null;
    }
}
