package java.security;

import java.net.URL;

public class ProtectionDomain {
    public ProtectionDomain(CodeSource codesource, PermissionCollection permissions) {}
    public ProtectionDomain(CodeSource codesource, PermissionCollection permissions, ClassLoader classloader, Principal[] principals) {}
    public CodeSource getCodeSource() { return null; }
    public ClassLoader getClassLoader() { return null; }
    public Principal[] getPrincipals() { return null; }
    public PermissionCollection getPermissions() { return null; }
    public boolean implies(Permission permission) { return false; }
}
