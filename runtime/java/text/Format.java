package java.text;

import java.io.Serializable;

public abstract class Format implements Serializable, Cloneable {
    protected Format() {}
    public final String format(Object obj) { return null; }
    public abstract StringBuffer format(Object obj, StringBuffer toAppendTo, FieldPosition pos);
    public Object parseObject(String source) throws ParseException { return null; }
    public abstract Object parseObject(String source, ParsePosition pos);
    public Object clone() { return null; }

    public static class Field extends java.text.AttributedCharacterIterator.Attribute {
        protected Field(String name) { super(name); }
    }
}
