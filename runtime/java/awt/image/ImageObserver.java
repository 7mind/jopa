package java.awt.image;

public interface ImageObserver {
    int ABORT = 128;
    int ALLBITS = 32;
    int ERROR = 64;
    int FRAMEBITS = 16;
    int HEIGHT = 2;
    int PROPERTIES = 4;
    int SOMEBITS = 8;
    int WIDTH = 1;

    boolean imageUpdate(java.awt.Image img, int infoflags, int x, int y, int width, int height);
}
