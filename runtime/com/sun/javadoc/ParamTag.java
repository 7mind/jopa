package com.sun.javadoc;

public interface ParamTag extends Tag {
    String parameterName();
    String parameterComment();
    boolean isTypeParameter();
}
