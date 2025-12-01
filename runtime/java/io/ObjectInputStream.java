package java.io;
public class ObjectInputStream extends InputStream implements ObjectInput, ObjectStreamConstants {
    public ObjectInputStream(InputStream in) throws IOException {}
    public final Object readObject() throws IOException, ClassNotFoundException { return null; }
    public void defaultReadObject() throws IOException, ClassNotFoundException {}
    public int read() throws IOException { return -1; }
    public int read(byte[] b, int off, int len) throws IOException { return -1; }
    public void readFully(byte[] b) throws IOException {}
    public void readFully(byte[] b, int off, int len) throws IOException {}
    public int skipBytes(int n) throws IOException { return 0; }
    public boolean readBoolean() throws IOException { return false; }
    public byte readByte() throws IOException { return 0; }
    public int readUnsignedByte() throws IOException { return 0; }
    public short readShort() throws IOException { return 0; }
    public int readUnsignedShort() throws IOException { return 0; }
    public char readChar() throws IOException { return 0; }
    public int readInt() throws IOException { return 0; }
    public long readLong() throws IOException { return 0; }
    public float readFloat() throws IOException { return 0; }
    public double readDouble() throws IOException { return 0; }
    public String readLine() throws IOException { return null; }
    public String readUTF() throws IOException { return null; }
}
