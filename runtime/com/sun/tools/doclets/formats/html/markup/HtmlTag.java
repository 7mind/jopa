package com.sun.tools.doclets.formats.html.markup;

public enum HtmlTag {
    A, BODY, BR, CODE, DD, DIV, DL, DT, EM, FRAME, FRAMESET,
    H1, H2, H3, H4, H5, H6, HEAD, HR, HTML, I, IFRAME, IMG,
    LI, LINK, META, NOFRAMES, NOSCRIPT, OL, P, PRE, SCRIPT,
    SMALL, SPAN, STRONG, STYLE, TABLE, TBODY, TD, TH, THEAD,
    TITLE, TR, TT, UL;

    public String toString() {
        return name().toLowerCase();
    }
}
