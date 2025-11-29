package java.security;

public abstract class Permission implements java.io.Serializable {
    private String name;

    public Permission(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public abstract boolean implies(Permission permission);
    public abstract boolean equals(Object obj);
    public abstract int hashCode();
    public abstract String getActions();

    public PermissionCollection newPermissionCollection() {
        return null;
    }

    public void checkGuard(Object object) throws java.lang.SecurityException {}
}
