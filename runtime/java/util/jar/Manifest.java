package java.util.jar;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Map;

public class Manifest implements Cloneable {
    public Manifest() {}

    public Manifest(InputStream is) throws IOException {}

    public Manifest(Manifest man) {}

    public Attributes getMainAttributes() {
        return null;
    }

    public Map<String, Attributes> getEntries() {
        return null;
    }

    public Attributes getAttributes(String name) {
        return null;
    }

    public void clear() {}

    public void write(OutputStream out) throws IOException {}

    public void read(InputStream is) throws IOException {}

    public boolean equals(Object o) {
        return false;
    }

    public int hashCode() {
        return 0;
    }

    public Object clone() {
        return null;
    }
}
