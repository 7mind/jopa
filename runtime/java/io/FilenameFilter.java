package java.io;

public interface FilenameFilter {
    boolean accept(File dir, String name);
}
