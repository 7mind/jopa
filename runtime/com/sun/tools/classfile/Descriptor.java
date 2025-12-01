package com.sun.tools.classfile;

public class Descriptor {
    public static class InvalidDescriptor extends Exception {
        private static final long serialVersionUID = 1L;

        public InvalidDescriptor(String message) {
            super(message);
        }
    }
}
