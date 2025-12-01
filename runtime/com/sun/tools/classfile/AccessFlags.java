package com.sun.tools.classfile;

public class AccessFlags {
    public static final int ACC_BRIDGE = 0x0040;

    private final int flags;

    public AccessFlags(int flags) {
        this.flags = flags;
    }

    public boolean is(int flag) {
        return (flags & flag) != 0;
    }
}
