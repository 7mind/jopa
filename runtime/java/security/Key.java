package java.security;

public interface Key extends java.io.Serializable {
    String getAlgorithm();
    String getFormat();
    byte[] getEncoded();
}
