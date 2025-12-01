package com.sun.javadoc;

public interface Tag {
    String name();
    Doc holder();
    String kind();
    String text();
    String toString();
    Tag[] inlineTags();
    Tag[] firstSentenceTags();
    SourcePosition position();
}