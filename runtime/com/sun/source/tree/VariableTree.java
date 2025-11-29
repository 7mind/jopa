package com.sun.source.tree;

import javax.lang.model.element.Name;

public interface VariableTree extends StatementTree {
    ModifiersTree getModifiers();
    Name getName();
    Tree getType();
    ExpressionTree getInitializer();
}
