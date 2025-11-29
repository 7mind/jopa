package com.sun.tools.doclets.formats.html.markup;

public class DocType extends Content {
    private final String docType;

    public static DocType Frameset() {
        return new DocType("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" \"http://www.w3.org/TR/html4/frameset.dtd\">");
    }

    public static DocType Transitional() {
        return new DocType("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">");
    }

    private DocType(String docType) {
        this.docType = docType;
    }

    public void addContent(Content content) {}
    public void addContent(String stringContent) {}
    public boolean isEmpty() { return docType.isEmpty(); }
    public void write(java.io.Writer writer) throws java.io.IOException {
        writer.write(docType);
    }
    public String toString() { return docType; }
}
