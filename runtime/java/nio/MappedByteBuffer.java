package java.nio;

public abstract class MappedByteBuffer extends ByteBuffer {
    public final boolean isLoaded() { return true; }
    public final MappedByteBuffer load() { return this; }
    public final MappedByteBuffer force() { return this; }
}
