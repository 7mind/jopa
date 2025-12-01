package com.sun.tools.classfile;

public class LineNumberTable_attribute extends Attribute {
    public Entry[] line_number_table = new Entry[0];

    public LineNumberTable_attribute() {
        super(Attribute.LineNumberTable);
    }

    public static class Entry {
        public int start_pc;
        public int line_number;
    }
}
