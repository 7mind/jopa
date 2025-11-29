package com.sun.javadoc;

public interface MethodDoc extends ExecutableMemberDoc {
    boolean isAbstract();
    boolean isDefault();
    Type returnType();
    ClassDoc overriddenClass();
    Type overriddenType();
    MethodDoc overriddenMethod();
    boolean overrides(MethodDoc meth);
}
