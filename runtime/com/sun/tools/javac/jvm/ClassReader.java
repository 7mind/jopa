package com.sun.tools.javac.jvm;

import com.sun.tools.javac.util.Context;

public class ClassReader {
    public static ClassReader instance(Context context) { return null; }

    public static class BadClassFile extends RuntimeException {
        public BadClassFile(String msg) { super(msg); }
    }

    public static final Context.Key<ClassReader> classReaderKey = new Context.Key<ClassReader>();

    public static final int INITIAL_BUFFER_SIZE = 0x0fff0;
}
