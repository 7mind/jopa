package java.security;

import java.net.URL;
import java.security.cert.Certificate;

public class CodeSource implements java.io.Serializable {
    public CodeSource(URL url, Certificate[] certs) {}
    public CodeSource(URL url, CodeSigner[] signers) {}
    public URL getLocation() { return null; }
    public Certificate[] getCertificates() { return null; }
    public CodeSigner[] getCodeSigners() { return null; }
    public boolean implies(CodeSource codesource) { return false; }
}
