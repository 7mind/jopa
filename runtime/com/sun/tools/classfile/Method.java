package com.sun.tools.classfile;

public class Method {
    public final AccessFlags access_flags = new AccessFlags(0);
    public final int name_index = 0;
    public final Attributes attributes = new Attributes();

    public String getName(ConstantPool constant_pool) throws ConstantPoolException {
        return constant_pool.getUTF8Value(name_index);
    }
}
