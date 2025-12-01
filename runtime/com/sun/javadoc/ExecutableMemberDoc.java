package com.sun.javadoc;

public interface ExecutableMemberDoc extends MemberDoc {
    boolean isNative();
    boolean isSynchronized();
    boolean isVarArgs();
    Parameter[] parameters();
    ThrowsTag[] throwsTags();
    ParamTag[] paramTags();
    Type[] thrownExceptionTypes();
    ClassDoc[] thrownExceptions();
    TypeVariable[] typeParameters();
    ParamTag[] typeParamTags();
    String signature();
    String flatSignature();
}