package java.io;

public class BufferedWriter extends Writer {
    public BufferedWriter(Writer out) {}

    public BufferedWriter(Writer out, int sz) {}

    public void write(int c) throws IOException {}

    public void write(char[] cbuf, int off, int len) throws IOException {}

    public void write(String s, int off, int len) throws IOException {}

    public void newLine() throws IOException {}

    public void flush() throws IOException {}

    public void close() throws IOException {}
}
