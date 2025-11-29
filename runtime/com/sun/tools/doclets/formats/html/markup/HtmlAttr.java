package com.sun.tools.doclets.formats.html.markup;

public enum HtmlAttr {
    BORDER, CELLPADDING, CELLSPACING, CHARSET, CLASS, CLEAR, COLS,
    COLSPAN, CONTENT, DISABLED, HREF, HTTP_EQUIV, ID, LANG, NAME,
    ONLOAD, REL, ROWS, ROWSPAN, SCOPE, SCROLLING, SRC, STYLE,
    SUMMARY, TARGET, TITLE, TYPE, VALUE, WIDTH;

    public String toString() {
        return name().toLowerCase().replace('_', '-');
    }
}
