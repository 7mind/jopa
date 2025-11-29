package java.io;

import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;

public class InputStreamReader extends Reader {
    public InputStreamReader(InputStream in) {}
    public InputStreamReader(InputStream in, String charsetName) throws UnsupportedEncodingException {}
    public InputStreamReader(InputStream in, Charset cs) {}
    public InputStreamReader(InputStream in, CharsetDecoder dec) {}
    public String getEncoding() { return null; }
    public int read() throws IOException { return 0; }
    public int read(char[] cbuf, int offset, int length) throws IOException { return 0; }
    public boolean ready() throws IOException { return false; }
    public void close() throws IOException {}
}
