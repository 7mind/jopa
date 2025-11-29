package java.io;

public class DataInputStream extends FilterInputStream implements DataInput {
    public DataInputStream(InputStream in) { super(in); }

    public final int read(byte[] b) throws IOException { return 0; }
    public final int read(byte[] b, int off, int len) throws IOException { return 0; }
    public final void readFully(byte[] b) throws IOException {}
    public final void readFully(byte[] b, int off, int len) throws IOException {}
    public final int skipBytes(int n) throws IOException { return 0; }
    public final boolean readBoolean() throws IOException { return false; }
    public final byte readByte() throws IOException { return 0; }
    public final int readUnsignedByte() throws IOException { return 0; }
    public final short readShort() throws IOException { return 0; }
    public final int readUnsignedShort() throws IOException { return 0; }
    public final char readChar() throws IOException { return 0; }
    public final int readInt() throws IOException { return 0; }
    public final long readLong() throws IOException { return 0; }
    public final float readFloat() throws IOException { return 0; }
    public final double readDouble() throws IOException { return 0; }
    public final String readLine() throws IOException { return null; }
    public final String readUTF() throws IOException { return null; }
    public static final String readUTF(DataInput in) throws IOException { return null; }
}
