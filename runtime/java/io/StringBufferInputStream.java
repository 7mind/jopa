package java.io;

/**
 * @deprecated This class does not properly convert characters into bytes.
 */
@Deprecated
public class StringBufferInputStream extends InputStream {
    protected String buffer;
    protected int pos;
    protected int count;

    public StringBufferInputStream(String s) {
        this.buffer = s;
        count = s.length();
    }

    public synchronized int read() {
        return (pos < count) ? (buffer.charAt(pos++) & 0xFF) : -1;
    }

    public synchronized int read(byte[] b, int off, int len) {
        if (b == null) {
            throw new NullPointerException();
        } else if (off < 0 || len < 0 || len > b.length - off) {
            throw new IndexOutOfBoundsException();
        }
        if (pos >= count) {
            return -1;
        }
        int avail = count - pos;
        if (len > avail) {
            len = avail;
        }
        if (len <= 0) {
            return 0;
        }
        String s = buffer;
        int cnt = len;
        while (--cnt >= 0) {
            b[off++] = (byte)s.charAt(pos++);
        }
        return len;
    }

    public synchronized long skip(long n) {
        if (n < 0) {
            return 0;
        }
        if (n > count - pos) {
            n = count - pos;
        }
        pos += n;
        return n;
    }

    public synchronized int available() {
        return count - pos;
    }

    public synchronized void reset() {
        pos = 0;
    }
}
