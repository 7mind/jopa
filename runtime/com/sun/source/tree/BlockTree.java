package com.sun.source.tree;

import java.util.List;

public interface BlockTree extends StatementTree {
    boolean isStatic();
    List<? extends StatementTree> getStatements();
}
