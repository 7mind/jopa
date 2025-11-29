package com.sun.tools.doclets.formats.html.markup;

import java.io.IOException;
import java.io.Writer;

public class Comment extends Content {
    private String commentText;

    public Comment(String comment) {
        commentText = comment;
    }

    public void addContent(Content content) {
        throw new UnsupportedOperationException();
    }

    public void addContent(String stringContent) {
        throw new UnsupportedOperationException();
    }

    public boolean isEmpty() {
        return commentText.isEmpty();
    }

    public void write(Writer out) throws IOException {
        out.write("<!-- ");
        out.write(commentText);
        out.write(" -->");
    }

    public String toString() {
        return "<!-- " + commentText + " -->";
    }
}
