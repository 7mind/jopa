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
    boolean isField();
    boolean isMethod();
    boolean isConstructor();
    boolean isInterface();
    boolean isClass();
    boolean isOrdinaryClass();
    boolean isAnnotationType();
    boolean isAnnotationTypeElement();
    boolean isEnum();
    boolean isEnumConstant();
    boolean isError();
    boolean isException();
    SourcePosition position();
}