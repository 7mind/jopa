package java.nio.channels;

import java.io.IOException;
import java.nio.ByteBuffer;

public abstract class FileChannel implements Channel {
    protected FileChannel() {}

    public abstract int read(ByteBuffer dst) throws IOException;

    public abstract long read(ByteBuffer[] dsts, int offset, int length) throws IOException;

    public final long read(ByteBuffer[] dsts) throws IOException {
        return 0L;
    }

    public abstract int write(ByteBuffer src) throws IOException;

    public abstract long write(ByteBuffer[] srcs, int offset, int length) throws IOException;

    public final long write(ByteBuffer[] srcs) throws IOException {
        return 0L;
    }

    public abstract long position() throws IOException;

    public abstract FileChannel position(long newPosition) throws IOException;

    public abstract long size() throws IOException;

    public abstract FileChannel truncate(long size) throws IOException;

    public abstract void force(boolean metaData) throws IOException;

    public abstract long transferTo(long position, long count, WritableByteChannel target) throws IOException;

    public abstract long transferFrom(ReadableByteChannel src, long position, long count) throws IOException;

    public abstract int read(ByteBuffer dst, long position) throws IOException;

    public abstract int write(ByteBuffer src, long position) throws IOException;

    public boolean isOpen() {
        return false;
    }

    public void close() throws IOException {}
}
