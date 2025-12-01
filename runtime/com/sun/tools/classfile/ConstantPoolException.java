package com.sun.tools.classfile;

public class ConstantPoolException extends Exception {
    private static final long serialVersionUID = 1L;

    public ConstantPoolException() {
    }

    public ConstantPoolException(String message) {
        super(message);
    }

    public ConstantPoolException(String message, Throwable cause) {
        super(message, cause);
    }
}
