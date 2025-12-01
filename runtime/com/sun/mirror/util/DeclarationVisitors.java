package com.sun.mirror.util;
public class DeclarationVisitors {
    public static final DeclarationVisitor NO_OP = new SimpleDeclarationVisitor();
    public static DeclarationVisitor getSourceOrderDeclarationScanner(DeclarationVisitor v, DeclarationVisitor p) {
        return NO_OP;
    }
}
