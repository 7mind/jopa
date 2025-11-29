package java.util.zip;

public class ZipEntry implements Cloneable {
    public static final int STORED = 0;
    public static final int DEFLATED = 8;

    public ZipEntry(String name) {}

    public ZipEntry(ZipEntry e) {}

    public String getName() {
        return null;
    }

    public void setTime(long time) {}

    public long getTime() {
        return 0L;
    }

    public void setSize(long size) {}

    public long getSize() {
        return 0L;
    }

    public long getCompressedSize() {
        return 0L;
    }

    public void setCompressedSize(long csize) {}

    public void setCrc(long crc) {}

    public long getCrc() {
        return 0L;
    }

    public void setMethod(int method) {}

    public int getMethod() {
        return 0;
    }

    public void setExtra(byte[] extra) {}

    public byte[] getExtra() {
        return null;
    }

    public void setComment(String comment) {}

    public String getComment() {
        return null;
    }

    public boolean isDirectory() {
        return false;
    }

    public String toString() {
        return null;
    }

    public int hashCode() {
        return 0;
    }

    public Object clone() {
        return null;
    }
}
