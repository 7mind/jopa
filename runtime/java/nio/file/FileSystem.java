package java.nio.file;

import java.io.Closeable;
import java.io.IOException;

public abstract class FileSystem implements Closeable {
    protected FileSystem() {}

    public abstract boolean isOpen();

    public abstract boolean isReadOnly();

    public abstract String getSeparator();

    public abstract Iterable<Path> getRootDirectories();

    public abstract Path getPath(String first, String... more);

    public void close() throws IOException {}
}
