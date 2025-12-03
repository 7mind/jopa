package com.sun.tools.javac.nio;

import java.io.IOException;
import java.nio.file.Path;
import javax.tools.JavaFileManager;
import javax.tools.JavaFileObject;

public interface PathFileManager extends JavaFileManager {
    Iterable<? extends JavaFileObject> getJavaFileObjectsFromPaths(Iterable<? extends Path> paths);
    Iterable<? extends JavaFileObject> getJavaFileObjects(Path... paths);
    Path getPath(JavaFileManager.Location location, String path);
    void setDefaultFileSystem(java.nio.file.FileSystem fs);
}
