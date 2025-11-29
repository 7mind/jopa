package com.sun.source.tree;
import java.util.List;
public interface ParameterizedTypeTree extends Tree { Tree getType(); List<? extends Tree> getTypeArguments(); }
