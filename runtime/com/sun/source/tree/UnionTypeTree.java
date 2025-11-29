package com.sun.source.tree;
import java.util.List;
public interface UnionTypeTree extends Tree { List<? extends Tree> getTypeAlternatives(); }
