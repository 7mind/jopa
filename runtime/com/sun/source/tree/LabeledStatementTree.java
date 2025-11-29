package com.sun.source.tree;
import javax.lang.model.element.Name;
public interface LabeledStatementTree extends StatementTree { Name getLabel(); StatementTree getStatement(); }
