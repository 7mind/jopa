package java.security.cert;

import java.security.PublicKey;

public abstract class Certificate implements java.io.Serializable {
    private final String type;

    protected Certificate(String type) {
        this.type = type;
    }

    public final String getType() {
        return type;
    }

    public abstract byte[] getEncoded() throws CertificateEncodingException;
    public abstract void verify(PublicKey key) throws CertificateException, java.security.NoSuchAlgorithmException, java.security.InvalidKeyException, java.security.NoSuchProviderException, java.security.SignatureException;
    public abstract void verify(PublicKey key, String sigProvider) throws CertificateException, java.security.NoSuchAlgorithmException, java.security.InvalidKeyException, java.security.NoSuchProviderException, java.security.SignatureException;
    public abstract String toString();
    public abstract PublicKey getPublicKey();
}
