package com.sun.tools.doclets.formats.html.markup;

import java.io.IOException;
import java.io.Writer;

public class RawHtml extends Content {
    private String rawHtmlContent;

    public RawHtml(String rawHtml) {
        rawHtmlContent = rawHtml;
    }

    public void addContent(Content content) {
        throw new UnsupportedOperationException();
    }

    public void addContent(String stringContent) {
        throw new UnsupportedOperationException();
    }

    public boolean isEmpty() {
        return rawHtmlContent.isEmpty();
    }

    public void write(Writer out) throws IOException {
        out.write(rawHtmlContent);
    }

    public String toString() {
        return rawHtmlContent;
    }
}
