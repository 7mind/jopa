package java.io;

public class BufferedReader extends Reader {
    public BufferedReader(Reader in) {}

    public BufferedReader(Reader in, int sz) {}

    public int read() throws IOException {
        return 0;
    }

    public int read(char[] cbuf, int off, int len) throws IOException {
        return 0;
    }

    public String readLine() throws IOException {
        return null;
    }

    public long skip(long n) throws IOException {
        return 0L;
    }

    public boolean ready() throws IOException {
        return false;
    }

    public boolean markSupported() {
        return true;
    }

    public void mark(int readAheadLimit) throws IOException {}

    public void reset() throws IOException {}

    public void close() throws IOException {}
}
