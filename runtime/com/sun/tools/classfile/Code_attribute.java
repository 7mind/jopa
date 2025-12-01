package com.sun.tools.classfile;

public class Code_attribute extends Attribute {
    public int max_stack;
    public int max_locals;
    public byte[] code = new byte[0];
    public Exception_data[] exception_table = new Exception_data[0];
    public Attributes attributes = new Attributes();

    public Code_attribute() {
        super(Attribute.Code);
    }

    public Iterable<Instruction> getInstructions() {
        return java.util.Collections.<Instruction>emptyList();
    }

    public static class Exception_data {
        public int start_pc;
        public int end_pc;
        public int handler_pc;
        public int catch_type;
    }

    public static class InvalidIndex extends Exception {
        private static final long serialVersionUID = 1L;

        public InvalidIndex(String message) {
            super(message);
        }
    }
}
