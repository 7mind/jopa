package com.sun.tools.javac.file;

import javax.tools.*;
import java.io.File;
import java.io.IOException;
import java.util.Set;
import java.util.Iterator;
import java.nio.charset.Charset;
import com.sun.tools.javac.util.Context;

public class JavacFileManager implements StandardJavaFileManager {
    public JavacFileManager(Context context, boolean register, Charset charset) {}

    public static void preRegister(Context context) {}

    public ClassLoader getClassLoader(JavaFileManager.Location location) { return null; }
    public Iterable<JavaFileObject> list(JavaFileManager.Location location, String packageName, Set<JavaFileObject.Kind> kinds, boolean recurse) throws IOException { return null; }
    public String inferBinaryName(JavaFileManager.Location location, JavaFileObject file) { return null; }
    public boolean isSameFile(FileObject a, FileObject b) { return false; }
    public boolean handleOption(String current, Iterator<String> remaining) { return false; }
    public boolean hasLocation(JavaFileManager.Location location) { return false; }
    public JavaFileObject getJavaFileForInput(JavaFileManager.Location location, String className, JavaFileObject.Kind kind) throws IOException { return null; }
    public JavaFileObject getJavaFileForOutput(JavaFileManager.Location location, String className, JavaFileObject.Kind kind, FileObject sibling) throws IOException { return null; }
    public FileObject getFileForInput(JavaFileManager.Location location, String packageName, String relativeName) throws IOException { return null; }
    public FileObject getFileForOutput(JavaFileManager.Location location, String packageName, String relativeName, FileObject sibling) throws IOException { return null; }
    public void flush() throws IOException {}
    public void close() throws IOException {}
    public int isSupportedOption(String option) { return -1; }
    public Iterable<? extends JavaFileObject> getJavaFileObjectsFromFiles(Iterable<? extends File> files) { return null; }
    public Iterable<? extends JavaFileObject> getJavaFileObjects(File... files) { return null; }
    public Iterable<? extends JavaFileObject> getJavaFileObjectsFromStrings(Iterable<String> names) { return null; }
    public Iterable<? extends JavaFileObject> getJavaFileObjects(String... names) { return null; }
    public void setLocation(JavaFileManager.Location location, Iterable<? extends File> path) throws IOException {}
    public Iterable<? extends File> getLocation(JavaFileManager.Location location) { return null; }
}
