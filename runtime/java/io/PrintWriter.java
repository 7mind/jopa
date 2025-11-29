package java.io;

public class PrintWriter extends Writer {
    public PrintWriter(Writer out) {}
    public PrintWriter(Writer out, boolean autoFlush) {}
    public PrintWriter(OutputStream out) {}
    public PrintWriter(OutputStream out, boolean autoFlush) {}
    public PrintWriter(String fileName) throws FileNotFoundException {}
    public PrintWriter(File file) throws FileNotFoundException {}

    public void flush() {}
    public void close() {}
    public boolean checkError() { return false; }
    protected void setError() {}
    protected void clearError() {}

    public void write(int c) {}
    public void write(char[] buf, int off, int len) {}
    public void write(char[] buf) {}
    public void write(String s, int off, int len) {}
    public void write(String s) {}

    public void print(boolean b) {}
    public void print(char c) {}
    public void print(int i) {}
    public void print(long l) {}
    public void print(float f) {}
    public void print(double d) {}
    public void print(char[] s) {}
    public void print(String s) {}
    public void print(Object obj) {}

    public void println() {}
    public void println(boolean x) {}
    public void println(char x) {}
    public void println(int x) {}
    public void println(long x) {}
    public void println(float x) {}
    public void println(double x) {}
    public void println(char[] x) {}
    public void println(String x) {}
    public void println(Object x) {}

    public PrintWriter printf(String format, Object... args) { return this; }
    public PrintWriter format(String format, Object... args) { return this; }

    public PrintWriter append(CharSequence csq) { return this; }
    public PrintWriter append(CharSequence csq, int start, int end) { return this; }
    public PrintWriter append(char c) { return this; }
}
