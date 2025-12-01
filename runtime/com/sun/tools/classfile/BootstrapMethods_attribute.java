package com.sun.tools.classfile;

public class BootstrapMethods_attribute extends Attribute {
    public BootstrapMethodSpecifier[] bootstrap_methods = new BootstrapMethodSpecifier[0];

    public BootstrapMethods_attribute() {
        super(Attribute.BootstrapMethods);
    }

    public static class BootstrapMethodSpecifier {
        public int bootstrap_method_ref;
        public int[] bootstrap_arguments = new int[0];
    }
}
