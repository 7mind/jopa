package com.sun.tools.classfile;

public class Attribute {
    public static final String Code = "Code";
    public static final String LineNumberTable = "LineNumberTable";
    public static final String LocalVariableTypeTable = "LocalVariableTypeTable";
    public static final String RuntimeVisibleAnnotations = "RuntimeVisibleAnnotations";
    public static final String RuntimeVisibleParameterAnnotations = "RuntimeVisibleParameterAnnotations";
    public static final String BootstrapMethods = "BootstrapMethods";

    public final String name;

    public Attribute(String name) {
        this.name = name;
    }
}
