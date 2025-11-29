package java.security.cert;

import java.util.List;
import java.util.Iterator;

public abstract class CertPath implements java.io.Serializable {
    private final String type;

    protected CertPath(String type) {
        this.type = type;
    }

    public String getType() {
        return type;
    }

    public abstract Iterator<String> getEncodings();
    public abstract byte[] getEncoded() throws CertificateEncodingException;
    public abstract byte[] getEncoded(String encoding) throws CertificateEncodingException;
    public abstract List<? extends Certificate> getCertificates();
}
