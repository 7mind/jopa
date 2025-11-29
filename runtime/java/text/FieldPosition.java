package java.text;

public class FieldPosition {
    private int field = 0;
    private int beginIndex = 0;
    private int endIndex = 0;

    public FieldPosition(int field) { this.field = field; }
    public FieldPosition(Format.Field attribute) {}
    public FieldPosition(Format.Field attribute, int fieldID) {}

    public Format.Field getFieldAttribute() { return null; }
    public int getField() { return field; }
    public int getBeginIndex() { return beginIndex; }
    public int getEndIndex() { return endIndex; }
    public void setBeginIndex(int bi) { beginIndex = bi; }
    public void setEndIndex(int ei) { endIndex = ei; }
}
