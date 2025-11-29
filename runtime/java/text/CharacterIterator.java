package java.text;

public interface CharacterIterator extends Cloneable {
    public static final char DONE = '\uFFFF';

    char first();
    char last();
    char current();
    char next();
    char previous();
    char setIndex(int position);
    int getBeginIndex();
    int getEndIndex();
    int getIndex();
    Object clone();
}
