package java.security;

import java.security.cert.CertPath;
import java.util.Date;

public final class Timestamp implements java.io.Serializable {
    public Timestamp(Date timestamp, CertPath signerCertPath) {}
    public Date getTimestamp() { return null; }
    public CertPath getSignerCertPath() { return null; }
    public int hashCode() { return 0; }
    public boolean equals(Object obj) { return false; }
    public String toString() { return ""; }
}
