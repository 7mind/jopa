package com.sun.source.tree;
import java.util.List;
public interface SwitchTree extends StatementTree { ExpressionTree getExpression(); List<? extends CaseTree> getCases(); }
