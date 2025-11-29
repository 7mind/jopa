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
}
