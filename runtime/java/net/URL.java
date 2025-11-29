package java.net;

import java.io.IOException;
import java.io.InputStream;

public final class URL implements java.io.Serializable {
    public URL(String spec) throws MalformedURLException {}

    public URL(String protocol, String host, String file) throws MalformedURLException {}

    public URL(String protocol, String host, int port, String file) throws MalformedURLException {}

    public URL(URL context, String spec) throws MalformedURLException {}

    public String getProtocol() {
        return null;
    }

    public String getHost() {
        return null;
    }

    public int getPort() {
        return 0;
    }

    public int getDefaultPort() {
        return 0;
    }

    public String getFile() {
        return null;
    }

    public String getPath() {
        return null;
    }

    public String getQuery() {
        return null;
    }

    public String getRef() {
        return null;
    }

    public String getAuthority() {
        return null;
    }

    public String getUserInfo() {
        return null;
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

    public String toExternalForm() {
        return null;
    }

    public URI toURI() throws URISyntaxException {
        return null;
    }

    public URLConnection openConnection() throws IOException {
        return null;
    }

    public InputStream openStream() throws IOException {
        return null;
    }

    public Object getContent() throws IOException {
        return null;
    }
}
