package java.nio;

public abstract class ByteBuffer extends Buffer implements Comparable<ByteBuffer> {
    public static ByteBuffer allocate(int capacity) {
        return null;
    }

    public static ByteBuffer allocateDirect(int capacity) {
        return null;
    }

    public static ByteBuffer wrap(byte[] array) {
        return null;
    }

    public static ByteBuffer wrap(byte[] array, int offset, int length) {
        return null;
    }

    public abstract ByteBuffer slice();

    public abstract ByteBuffer duplicate();

    public abstract ByteBuffer asReadOnlyBuffer();

    public abstract byte get();

    public abstract ByteBuffer put(byte b);

    public abstract byte get(int index);

    public abstract ByteBuffer put(int index, byte b);

    public ByteBuffer get(byte[] dst) {
        return this;
    }

    public ByteBuffer get(byte[] dst, int offset, int length) {
        return this;
    }

    public ByteBuffer put(ByteBuffer src) {
        return this;
    }

    public ByteBuffer put(byte[] src) {
        return this;
    }

    public ByteBuffer put(byte[] src, int offset, int length) {
        return this;
    }

    public final boolean hasArray() {
        return false;
    }

    public final byte[] array() {
        return null;
    }

    public final int arrayOffset() {
        return 0;
    }

    public abstract ByteBuffer compact();

    public abstract boolean isDirect();

    public boolean isReadOnly() {
        return false;
    }

    public abstract char getChar();
    public abstract ByteBuffer putChar(char value);
    public abstract char getChar(int index);
    public abstract ByteBuffer putChar(int index, char value);

    public abstract short getShort();
    public abstract ByteBuffer putShort(short value);
    public abstract short getShort(int index);
    public abstract ByteBuffer putShort(int index, short value);

    public abstract int getInt();
    public abstract ByteBuffer putInt(int value);
    public abstract int getInt(int index);
    public abstract ByteBuffer putInt(int index, int value);

    public abstract long getLong();
    public abstract ByteBuffer putLong(long value);
    public abstract long getLong(int index);
    public abstract ByteBuffer putLong(int index, long value);

    public abstract float getFloat();
    public abstract ByteBuffer putFloat(float value);
    public abstract float getFloat(int index);
    public abstract ByteBuffer putFloat(int index, float value);

    public abstract double getDouble();
    public abstract ByteBuffer putDouble(double value);
    public abstract double getDouble(int index);
    public abstract ByteBuffer putDouble(int index, double value);

    public int compareTo(ByteBuffer that) {
        return 0;
    }

    public boolean equals(Object obj) {
        return false;
    }

    public int hashCode() {
        return 0;
    }

    public String toString() {
        return null;
    }
}
