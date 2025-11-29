package com.sun.tools.doclets.formats.html.markup;

import java.io.IOException;
import java.io.Writer;

public class StringContent extends Content {
    private StringBuilder stringContent;

    public StringContent() {
        stringContent = new StringBuilder();
    }

    public StringContent(String initialContent) {
        stringContent = new StringBuilder(initialContent);
    }

    public void addContent(Content content) {
        throw new UnsupportedOperationException();
    }

    public void addContent(String strContent) {
        stringContent.append(strContent);
    }

    public boolean isEmpty() {
        return (stringContent.length() == 0);
    }

    public void write(Writer out) throws IOException {
        out.write(stringContent.toString());
    }

    public String toString() {
        return stringContent.toString();
    }
}
