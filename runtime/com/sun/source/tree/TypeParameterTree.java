package com.sun.source.tree;

import java.util.List;
import javax.lang.model.element.Name;

public interface TypeParameterTree extends Tree {
    Name getName();
    List<? extends Tree> getBounds();
    List<? extends AnnotationTree> getAnnotations();
}
