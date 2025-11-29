package com.sun.tools.doclets.formats.html.markup;

public abstract class Content {
    public abstract void addContent(Content content);
    public abstract void addContent(String stringContent);
    public abstract boolean isEmpty();
    public abstract void write(java.io.Writer writer) throws java.io.IOException;
}
