package java.io;

public class PrintStream extends OutputStream {
    public PrintStream(OutputStream out) {}

    public PrintStream(OutputStream out, boolean autoFlush) {}

    public PrintStream(String fileName) throws FileNotFoundException {}

    public PrintStream(File file) throws FileNotFoundException {}

    public void write(int b) {}

    public void print(String s) {
        // Native implementation
    }

    public void print(int i) {
        // Native implementation
    }

    public void print(Object obj) {
        // Native implementation
    }

    public void println() {
        // Native implementation
    }

    public void println(String s) {
        // Native implementation
    }

    public void println(int i) {
        // Native implementation
    }

    public void println(Object obj) {
        // Native implementation
    }

    public void print(boolean b) {}
    public void print(char c) {}
    public void print(long l) {}
    public void print(float f) {}
    public void print(double d) {}
    public void print(char[] s) {}

    public void println(boolean x) {}
    public void println(char x) {}
    public void println(long x) {}
    public void println(float x) {}
    public void println(double x) {}
    public void println(char[] x) {}

    public PrintStream printf(String format, Object... args) { return this; }
    public PrintStream printf(java.util.Locale l, String format, Object... args) { return this; }
    public PrintStream format(String format, Object... args) { return this; }
    public PrintStream format(java.util.Locale l, String format, Object... args) { return this; }

    public PrintStream append(CharSequence csq) { return this; }
    public PrintStream append(CharSequence csq, int start, int end) { return this; }
    public PrintStream append(char c) { return this; }

    public void flush() {}
    public void close() {}
    public boolean checkError() { return false; }
}
