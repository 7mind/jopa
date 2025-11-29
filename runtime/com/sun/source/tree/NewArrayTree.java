package com.sun.source.tree;
import java.util.List;
public interface NewArrayTree extends ExpressionTree { Tree getType(); List<? extends ExpressionTree> getDimensions(); List<? extends ExpressionTree> getInitializers(); }
