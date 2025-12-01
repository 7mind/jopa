package java.io;

public class File implements Serializable, Comparable<File> {
    public static final String separator = "/";
    public static final char separatorChar = '/';
    public static final String pathSeparator = ":";
    public static final char pathSeparatorChar = ':';

    public File(String pathname) {}

    public File(String parent, String child) {}

        public File(File parent, String child) {}

    

        public File(java.net.URI uri) {}

    

        public String getName() { return null; }

    

    public String getParent() {
        return null;
    }

    public File getParentFile() {
        return null;
    }

    public String getPath() {
        return null;
    }

    public boolean isAbsolute() {
        return false;
    }

    public String getAbsolutePath() {
        return null;
    }

    public File getAbsoluteFile() {
        return null;
    }

    public String getCanonicalPath() throws IOException {
        return null;
    }

    public File getCanonicalFile() throws IOException {
        return null;
    }

    public boolean canRead() {
        return false;
    }

    public boolean canWrite() {
        return false;
    }

    public boolean exists() {
        return false;
    }

    public boolean isDirectory() {
        return false;
    }

    public boolean isFile() {
        return false;
    }

    public boolean isHidden() {
        return false;
    }

    public long lastModified() {
        return 0L;
    }

    public long length() {
        return 0L;
    }

    public boolean createNewFile() throws IOException {
        return false;
    }

        public boolean delete() { return false; }

        public void deleteOnExit() {}

        public String[] list() { return null; }

    

    public String[] list(FilenameFilter filter) {
        return null;
    }

    public File[] listFiles() {
        return null;
    }

    public File[] listFiles(FilenameFilter filter) {
        return null;
    }

    public File[] listFiles(FileFilter filter) {
        return null;
    }

    public boolean mkdir() {
        return false;
    }

    public boolean mkdirs() {
        return false;
    }

    public boolean renameTo(File dest) {
        return false;
    }

    public boolean setLastModified(long time) {
        return false;
    }

    public boolean setReadOnly() {
        return false;
    }

    public boolean setWritable(boolean writable) {
        return false;
    }

    public boolean setReadable(boolean readable) {
        return false;
    }

    public boolean setExecutable(boolean executable) {
        return false;
    }

    public boolean canExecute() {
        return false;
    }

    public static File[] listRoots() {
        return null;
    }

    public long getTotalSpace() {
        return 0L;
    }

    public long getFreeSpace() {
        return 0L;
    }

    public long getUsableSpace() {
        return 0L;
    }

    public static File createTempFile(String prefix, String suffix) throws IOException {
        return null;
    }

    public static File createTempFile(String prefix, String suffix, File directory) throws IOException {
        return null;
    }

    public int compareTo(File pathname) {
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

    public java.nio.file.Path toPath() {
        return null;
    }
}
