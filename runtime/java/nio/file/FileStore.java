package java.nio.file;

import java.io.IOException;

public abstract class FileStore {
    protected FileStore() {}
    public abstract String name();
    public abstract String type();
    public abstract boolean isReadOnly();
    public abstract long getTotalSpace() throws IOException;
    public abstract long getUsableSpace() throws IOException;
    public abstract long getUnallocatedSpace() throws IOException;
}
