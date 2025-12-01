package java.util;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.io.Reader;
import java.io.Writer;

public class Properties extends Hashtable<Object, Object> {
    protected Properties defaults;

    public Properties() {}
    public Properties(Properties defaults) { this.defaults = defaults; }

    public String getProperty(String key) { return null; }
    public String getProperty(String key, String defaultValue) { return null; }
    public Object setProperty(String key, String value) { return null; }
    public Enumeration<?> propertyNames() { return null; }
    public Set<String> stringPropertyNames() { return null; }
    public void load(InputStream inStream) throws java.io.IOException {}
    public void load(Reader reader) throws java.io.IOException {}
    public void store(OutputStream out, String comments) throws java.io.IOException {}
    public void store(Writer writer, String comments) throws java.io.IOException {}
    public void loadFromXML(InputStream in) throws java.io.IOException {}
    public void storeToXML(OutputStream os, String comment) throws java.io.IOException {}
    public void storeToXML(OutputStream os, String comment, String encoding) throws java.io.IOException {}
    public void list(PrintStream out) {}
    public void list(PrintWriter out) {}
}
