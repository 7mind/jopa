package javax.tools;

import java.io.*;
import java.net.URI;
import javax.lang.model.element.NestingKind;

public class SimpleJavaFileObject implements JavaFileObject {
    protected URI uri;
    protected Kind kind;

    protected SimpleJavaFileObject(URI uri, Kind kind) {
        this.uri = uri;
        this.kind = kind;
    }

    public URI toUri() { return uri; }
    public String getName() { return uri.getPath(); }
    public InputStream openInputStream() throws IOException { throw new UnsupportedOperationException(); }
    public OutputStream openOutputStream() throws IOException { throw new UnsupportedOperationException(); }
    public Reader openReader(boolean ignoreEncodingErrors) throws IOException { throw new UnsupportedOperationException(); }
    public CharSequence getCharContent(boolean ignoreEncodingErrors) throws IOException { throw new UnsupportedOperationException(); }
    public Writer openWriter() throws IOException { throw new UnsupportedOperationException(); }
    public long getLastModified() { return 0L; }
    public boolean delete() { return false; }
    public Kind getKind() { return kind; }
    public boolean isNameCompatible(String simpleName, Kind kind) { return true; }
    public NestingKind getNestingKind() { return null; }
    public javax.lang.model.element.Modifier getAccessLevel() { return null; }
}
