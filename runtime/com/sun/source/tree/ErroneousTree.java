package com.sun.source.tree;
import java.util.List;
public interface ErroneousTree extends ExpressionTree { List<? extends Tree> getErrorTrees(); }
