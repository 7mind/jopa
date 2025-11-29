package com.sun.tools.doclets.formats.html.markup;

import java.io.IOException;
import java.io.Writer;
import java.util.ArrayList;
import java.util.List;

public class ContentBuilder extends Content {
    private List<Content> contents = new ArrayList<Content>();

    public ContentBuilder() {}

    public ContentBuilder(Content... contents) {
        for (Content c : contents) {
            addContent(c);
        }
    }

    public void addContent(Content content) {
        if (content != null) {
            contents.add(content);
        }
    }

    public void addContent(String text) {
        if (text != null && !text.isEmpty()) {
            contents.add(new StringContent(text));
        }
    }

    public boolean isEmpty() {
        return contents.isEmpty();
    }

    public void write(Writer out) throws IOException {
        for (Content c : contents) {
            c.write(out);
        }
    }
}
