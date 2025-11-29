package java.util.zip;

public class CRC32 implements Checksum {
    private int crc;

    public CRC32() {}

    public void update(int b) {}
    public void update(byte[] b, int off, int len) {}
    public void update(byte[] b) { update(b, 0, b.length); }
    public void reset() { crc = 0; }
    public long getValue() { return crc & 0xffffffffL; }
}
