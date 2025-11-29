package com.sun.tools.doclets.formats.html.markup;

import java.io.IOException;
import java.io.Writer;
import java.util.List;
import java.util.ArrayList;

public class HtmlDocument extends Content {
    private List<Content> docContent = new ArrayList<Content>();

    public HtmlDocument(Content docType, Content htmlTree) {
        docContent.add(docType);
        docContent.add(htmlTree);
    }

    public void addContent(Content c) {
        docContent.add(c);
    }

    public void addContent(String stringContent) {}
    public boolean isEmpty() { return docContent.isEmpty(); }

    public void write(Writer out) throws IOException {
        for (Content c : docContent) {
            c.write(out);
        }
    }
}
