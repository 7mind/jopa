package java.security;

import java.security.cert.CertPath;

public final class CodeSigner implements java.io.Serializable {
    public CodeSigner(CertPath signerCertPath, Timestamp timestamp) {}
    public CertPath getSignerCertPath() { return null; }
    public Timestamp getTimestamp() { return null; }
    public int hashCode() { return 0; }
    public boolean equals(Object obj) { return false; }
    public String toString() { return ""; }
}
