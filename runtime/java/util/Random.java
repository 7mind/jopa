package java.util;

import java.io.Serializable;

public class Random implements Serializable {
    public Random() {}
    public Random(long seed) {}

    public void setSeed(long seed) {}
    protected int next(int bits) { return 0; }
    public void nextBytes(byte[] bytes) {}
    public int nextInt() { return 0; }
    public int nextInt(int bound) { return 0; }
    public long nextLong() { return 0L; }
    public boolean nextBoolean() { return false; }
    public float nextFloat() { return 0.0f; }
    public double nextDouble() { return 0.0; }
    public double nextGaussian() { return 0.0; }
}
