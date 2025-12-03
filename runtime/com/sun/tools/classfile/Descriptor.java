package com.sun.tools.classfile;

public class Descriptor {
    public Descriptor() {}

    public String getReturnType(ConstantPool constant_pool) throws ConstantPoolException, InvalidDescriptor {
        return null;
    }

    public String getParameterTypes(ConstantPool constant_pool) throws ConstantPoolException, InvalidDescriptor {
        return null;
    }

    public static class InvalidDescriptor extends Exception {
        private static final long serialVersionUID = 1L;

        public InvalidDescriptor(String message) {
            super(message);
        }
    }
}
