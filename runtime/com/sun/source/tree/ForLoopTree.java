package com.sun.source.tree;
import java.util.List;
public interface ForLoopTree extends StatementTree { List<? extends StatementTree> getInitializer(); ExpressionTree getCondition(); List<? extends ExpressionStatementTree> getUpdate(); StatementTree getStatement(); }
