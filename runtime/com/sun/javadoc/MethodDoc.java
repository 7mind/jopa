package com.sun.javadoc;

public interface MethodDoc extends ExecutableMemberDoc {
    boolean isAbstract();
    Type returnType();
    ClassDoc overriddenClass();
    Type overriddenType();
    MethodDoc overriddenMethod();
    boolean overrides(MethodDoc meth);
}