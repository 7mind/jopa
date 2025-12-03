package com.sun.tools.classfile;

public class LocalVariableTypeTable_attribute extends Attribute {
    public Entry[] local_variable_table = new Entry[0];
    public int local_variable_table_length = 0;

    public LocalVariableTypeTable_attribute() {
        super(Attribute.LocalVariableTypeTable);
    }

    public static class Entry {
        public int start_pc;
        public int length;
        public int name_index;
        public int signature_index;
        public int index;
    }
}
