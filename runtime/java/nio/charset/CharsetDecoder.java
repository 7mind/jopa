package java.nio.charset;

public abstract class CharsetDecoder {
    protected CharsetDecoder(Charset cs, float averageCharsPerByte, float maxCharsPerByte) {}
    public final Charset charset() { return null; }
    public final float averageCharsPerByte() { return 0; }
    public final float maxCharsPerByte() { return 0; }
}
