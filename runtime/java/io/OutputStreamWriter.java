package java.io;

public class OutputStreamWriter extends Writer {
    public OutputStreamWriter(OutputStream out) {}
    public OutputStreamWriter(OutputStream out, String charsetName) throws UnsupportedEncodingException {}

    public String getEncoding() { return null; }

    public void write(int c) throws IOException {}
    public void write(char[] cbuf, int off, int len) throws IOException {}
    public void write(String str, int off, int len) throws IOException {}
    public void flush() throws IOException {}
    public void close() throws IOException {}
}
