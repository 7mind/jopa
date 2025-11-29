package com.sun.source.tree;
import java.util.List;
public interface CaseTree extends Tree { ExpressionTree getExpression(); List<? extends StatementTree> getStatements(); }
