package com.sun.source.tree;
import java.util.List;
public interface MethodInvocationTree extends ExpressionTree { List<? extends Tree> getTypeArguments(); ExpressionTree getMethodSelect(); List<? extends ExpressionTree> getArguments(); }
