package javax.tools;

import java.io.File;
import java.io.IOException;

public interface StandardJavaFileManager extends JavaFileManager {
    boolean isSameFile(FileObject a, FileObject b);
    Iterable<? extends JavaFileObject> getJavaFileObjectsFromFiles(Iterable<? extends File> files);
    Iterable<? extends JavaFileObject> getJavaFileObjects(File... files);
    Iterable<? extends JavaFileObject> getJavaFileObjectsFromStrings(Iterable<String> names);
    Iterable<? extends JavaFileObject> getJavaFileObjects(String... names);
    void setLocation(JavaFileManager.Location location, Iterable<? extends File> path) throws IOException;
    Iterable<? extends File> getLocation(JavaFileManager.Location location);
}
