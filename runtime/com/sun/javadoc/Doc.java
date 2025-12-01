package com.sun.javadoc;

public interface Doc extends Comparable<Object> {
    String name();
    String commentText();
    Tag[] tags();
    Tag[] tags(String tagname);
    SeeTag[] seeTags();
    Tag[] inlineTags();
    Tag[] firstSentenceTags();
    String getRawCommentText();
    void setRawCommentText(String rawDocumentation);
    boolean isIncluded();
    SourcePosition position();
}