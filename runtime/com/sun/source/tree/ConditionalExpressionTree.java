package com.sun.source.tree;
public interface ConditionalExpressionTree extends ExpressionTree { ExpressionTree getCondition(); ExpressionTree getTrueExpression(); ExpressionTree getFalseExpression(); }
