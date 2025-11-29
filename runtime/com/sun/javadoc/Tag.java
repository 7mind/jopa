package com.sun.javadoc;

public interface Tag {
    String name();
    Doc holder();
    String kind();
    String text();
    Tag[] inlineTags();
    Tag[] firstSentenceTags();
    SourcePosition position();
}
