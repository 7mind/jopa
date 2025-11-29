package java.net;

import java.io.Serializable;

public final class URI implements Comparable<URI>, Serializable {
    public URI(String str) throws URISyntaxException {}

    public URI(String scheme, String ssp, String fragment) throws URISyntaxException {}

    public URI(String scheme, String userInfo, String host, int port, String path, String query, String fragment) throws URISyntaxException {}

    public URI(String scheme, String host, String path, String fragment) throws URISyntaxException {}

    public URI(String scheme, String authority, String path, String query, String fragment) throws URISyntaxException {}

    public static URI create(String str) {
        return null;
    }

    public String getScheme() {
        return null;
    }

    public boolean isAbsolute() {
        return false;
    }

    public boolean isOpaque() {
        return false;
    }

    public String getRawSchemeSpecificPart() {
        return null;
    }

    public String getSchemeSpecificPart() {
        return null;
    }

    public String getRawAuthority() {
        return null;
    }

    public String getAuthority() {
        return null;
    }

    public String getRawUserInfo() {
        return null;
    }

    public String getUserInfo() {
        return null;
    }

    public String getHost() {
        return null;
    }

    public int getPort() {
        return 0;
    }

    public String getRawPath() {
        return null;
    }

    public String getPath() {
        return null;
    }

    public String getRawQuery() {
        return null;
    }

    public String getQuery() {
        return null;
    }

    public String getRawFragment() {
        return null;
    }

    public String getFragment() {
        return null;
    }

    public URI resolve(URI uri) {
        return null;
    }

    public URI resolve(String str) {
        return null;
    }

    public URI relativize(URI uri) {
        return null;
    }

    public URL toURL() throws MalformedURLException {
        return null;
    }

    public URI normalize() {
        return null;
    }

    public int compareTo(URI that) {
        return 0;
    }

    public String toString() {
        return null;
    }

    public String toASCIIString() {
        return null;
    }

    public boolean equals(Object obj) {
        return false;
    }

    public int hashCode() {
        return 0;
    }
}
