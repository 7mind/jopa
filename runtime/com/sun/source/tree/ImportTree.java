package com.sun.source.tree;

public interface ImportTree extends Tree {
    boolean isStatic();
    Tree getQualifiedIdentifier();
}
