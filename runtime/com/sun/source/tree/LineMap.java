package com.sun.source.tree;

public interface LineMap {
    long getStartPosition(long line);
    long getPosition(long line, long column);
    long getLineNumber(long pos);
    long getColumnNumber(long pos);
}
