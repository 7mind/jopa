package com.sun.tools.classfile;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

public class ClassFile {
    public final ConstantPool constant_pool;
    public final Method[] methods;
    public final Attributes attributes;

    public ClassFile() {
        this.constant_pool = new ConstantPool();
        this.methods = new Method[0];
        this.attributes = new Attributes();
    }

    public static ClassFile read(File file) throws IOException, ConstantPoolException {
        return new ClassFile();
    }

    public static ClassFile read(InputStream in) throws IOException, ConstantPoolException {
        return new ClassFile();
    }
}
