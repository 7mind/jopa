package com.sun.javadoc;

import java.io.File;

public interface SourcePosition {
    File file();
    int line();
    int column();
}
