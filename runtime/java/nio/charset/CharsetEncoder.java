package java.nio.charset;

public abstract class CharsetEncoder {
    protected CharsetEncoder(Charset cs, float averageBytesPerChar, float maxBytesPerChar) {}
    public final Charset charset() { return null; }
    public final float averageBytesPerChar() { return 0; }
    public final float maxBytesPerChar() { return 0; }
}
