package java.nio.file.spi;

import java.io.IOException;
import java.net.URI;
import java.nio.file.FileSystem;
import java.nio.file.Path;
import java.util.List;
import java.util.Map;

public abstract class FileSystemProvider {
    protected FileSystemProvider() {}

    public static List<FileSystemProvider> installedProviders() {
        return null;
    }

    public abstract String getScheme();

    public abstract FileSystem newFileSystem(URI uri, Map<String,?> env) throws IOException;

    public abstract FileSystem getFileSystem(URI uri);

    public abstract Path getPath(URI uri);
}
