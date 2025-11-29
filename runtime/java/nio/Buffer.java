package java.nio;

public abstract class Buffer {
    public final int capacity() {
        return 0;
    }

    public final int position() {
        return 0;
    }

    public final Buffer position(int newPosition) {
        return this;
    }

    public final int limit() {
        return 0;
    }

    public final Buffer limit(int newLimit) {
        return this;
    }

    public final Buffer mark() {
        return this;
    }

    public final Buffer reset() {
        return this;
    }

    public final Buffer clear() {
        return this;
    }

    public final Buffer flip() {
        return this;
    }

    public final Buffer rewind() {
        return this;
    }

    public final int remaining() {
        return 0;
    }

    public final boolean hasRemaining() {
        return false;
    }

    public abstract boolean isReadOnly();

    public abstract boolean hasArray();

    public abstract Object array();

    public abstract int arrayOffset();

    public abstract boolean isDirect();
}
