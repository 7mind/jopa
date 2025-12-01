package java.nio.file;

import java.io.IOException;
import java.nio.charset.Charset;
import java.util.List;

public final class Files {
    private Files() {}

    public static Path createFile(Path path, java.nio.file.attribute.FileAttribute<?>... attrs) throws IOException { return null; }
    public static void delete(Path path) throws IOException {}
    public static boolean deleteIfExists(Path path) throws IOException { return false; }
    public static boolean exists(Path path, LinkOption... options) { return false; }
    public static Path createDirectories(Path dir, java.nio.file.attribute.FileAttribute<?>... attrs) throws IOException { return null; }
    public static Path copy(Path source, Path target, CopyOption... options) throws IOException { return null; }
    public static Path move(Path source, Path target, CopyOption... options) throws IOException { return null; }
    public static boolean isSameFile(Path path, Path path2) throws IOException { return false; }
    public static FileStore getFileStore(Path path) throws IOException { return null; }
}
