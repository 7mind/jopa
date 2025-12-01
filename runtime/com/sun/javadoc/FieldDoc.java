package com.sun.javadoc;

public interface FieldDoc extends MemberDoc {
    Type type();
    boolean isTransient();
    boolean isVolatile();
    SerialFieldTag[] serialFieldTags();
    Object constantValue();
    String constantValueExpression();
}