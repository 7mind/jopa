package com.sun.mirror.declaration;

import java.util.Collection;
import com.sun.mirror.type.ReferenceType;

public interface TypeParameterDeclaration extends Declaration {
    Collection<ReferenceType> getBounds();
    Declaration getOwner();
}
