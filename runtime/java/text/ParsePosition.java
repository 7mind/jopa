package java.text;

public class ParsePosition {
    int index = 0;
    int errorIndex = -1;

    public ParsePosition(int index) { this.index = index; }
    public int getIndex() { return index; }
    public void setIndex(int index) { this.index = index; }
    public void setErrorIndex(int ei) { errorIndex = ei; }
    public int getErrorIndex() { return errorIndex; }
}
