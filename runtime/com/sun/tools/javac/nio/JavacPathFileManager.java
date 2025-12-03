package com.sun.tools.javac.nio;

import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.file.FileSystem;
import java.nio.file.Path;
import java.util.Iterator;
import java.util.Set;
import javax.tools.FileObject;
import javax.tools.JavaFileManager;
import javax.tools.JavaFileObject;
import com.sun.tools.javac.util.Context;

public class JavacPathFileManager implements PathFileManager {
    public JavacPathFileManager(Context context, boolean register, Charset charset) {}

    public void setDefaultFileSystem(FileSystem fs) {}

    public Path getPath(Location location, String path) { return null; }

    public Iterable<? extends JavaFileObject> getJavaFileObjectsFromPaths(Iterable<? extends Path> paths) { return null; }

    public Iterable<? extends JavaFileObject> getJavaFileObjects(Path... paths) { return null; }

    public ClassLoader getClassLoader(Location location) { return null; }

    public Iterable<JavaFileObject> list(Location location, String packageName, Set<JavaFileObject.Kind> kinds, boolean recurse) throws IOException { return null; }

    public String inferBinaryName(Location location, JavaFileObject file) { return null; }

    public boolean isSameFile(FileObject a, FileObject b) { return false; }

    public boolean handleOption(String current, Iterator<String> remaining) { return false; }

    public boolean hasLocation(Location location) { return false; }

    public JavaFileObject getJavaFileForInput(Location location, String className, JavaFileObject.Kind kind) throws IOException { return null; }

    public JavaFileObject getJavaFileForOutput(Location location, String className, JavaFileObject.Kind kind, FileObject sibling) throws IOException { return null; }

    public FileObject getFileForInput(Location location, String packageName, String relativeName) throws IOException { return null; }

    public FileObject getFileForOutput(Location location, String packageName, String relativeName, FileObject sibling) throws IOException { return null; }

    public void flush() throws IOException {}

    public void close() throws IOException {}

    public int isSupportedOption(String option) { return -1; }
}
