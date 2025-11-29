package com.sun.source.tree;

import java.util.List;
import javax.lang.model.element.Name;

public interface ClassTree extends StatementTree {
    ModifiersTree getModifiers();
    Name getSimpleName();
    List<? extends TypeParameterTree> getTypeParameters();
    Tree getExtendsClause();
    List<? extends Tree> getImplementsClause();
    List<? extends Tree> getMembers();
}
