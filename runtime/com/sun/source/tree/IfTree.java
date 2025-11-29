package com.sun.source.tree;
public interface IfTree extends StatementTree { ExpressionTree getCondition(); StatementTree getThenStatement(); StatementTree getElseStatement(); }
