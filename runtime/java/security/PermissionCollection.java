package java.security;

import java.util.Enumeration;

public abstract class PermissionCollection implements java.io.Serializable {
    public abstract void add(Permission permission);
    public abstract boolean implies(Permission permission);
    public abstract Enumeration<Permission> elements();
    public void setReadOnly() {}
    public boolean isReadOnly() { return false; }
}
