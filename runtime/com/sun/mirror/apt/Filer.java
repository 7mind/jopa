package com.sun.mirror.apt;

import java.io.*;

public interface Filer {
    PrintWriter createSourceFile(String name) throws IOException;
    OutputStream createClassFile(String name) throws IOException;
    PrintWriter createTextFile(Filer.Location loc, String pkg, File relPath, String charsetName) throws IOException;
    OutputStream createBinaryFile(Filer.Location loc, String pkg, File relPath) throws IOException;

    enum Location {
        SOURCE_TREE,
        CLASS_TREE
    }
}
