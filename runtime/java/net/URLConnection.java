package java.net;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public abstract class URLConnection {
    protected URL url;

    protected URLConnection(URL url) {
        this.url = url;
    }

    public URL getURL() {
        return url;
    }

    public abstract void connect() throws IOException;

    public InputStream getInputStream() throws IOException {
        return null;
    }

    public OutputStream getOutputStream() throws IOException {
        return null;
    }

    public String getContentType() {
        return null;
    }

    public int getContentLength() {
        return 0;
    }

    public long getContentLengthLong() {
        return 0L;
    }

    public String getHeaderField(String name) {
        return null;
    }

    public void setRequestProperty(String key, String value) {}

    public String getRequestProperty(String key) {
        return null;
    }
}
