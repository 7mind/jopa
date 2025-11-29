package com.sun.tools.doclets.formats.html.markup;

import java.io.IOException;
import java.io.Writer;
import java.util.List;
import java.util.ArrayList;

public class HtmlTree extends Content {
    private HtmlTag htmlTag;
    private List<Content> content = new ArrayList<Content>();

    public HtmlTree(HtmlTag tag) {
        this.htmlTag = tag;
    }

    public HtmlTree(HtmlTag tag, Content c) {
        this(tag);
        addContent(c);
    }

    public void addAttr(HtmlAttr attr, String value) {}
    public void addStyle(HtmlStyle style) {}
    public void addContent(Content c) { content.add(c); }
    public void addContent(String s) {}
    public boolean isEmpty() { return content.isEmpty(); }
    public void write(Writer out) throws IOException {}

    public static HtmlTree A(String href, Content body) { return new HtmlTree(HtmlTag.A, body); }
    public static HtmlTree A_NAME(String name, Content body) { return new HtmlTree(HtmlTag.A, body); }
    public static HtmlTree CAPTION(Content body) { return new HtmlTree(HtmlTag.TABLE, body); }
    public static HtmlTree CODE(Content body) { return new HtmlTree(HtmlTag.CODE, body); }
    public static HtmlTree DD(Content body) { return new HtmlTree(HtmlTag.DD, body); }
    public static HtmlTree DIV(Content body) { return new HtmlTree(HtmlTag.DIV, body); }
    public static HtmlTree DIV(HtmlStyle styleClass, Content body) { return new HtmlTree(HtmlTag.DIV, body); }
    public static HtmlTree DL(Content body) { return new HtmlTree(HtmlTag.DL, body); }
    public static HtmlTree DT(Content body) { return new HtmlTree(HtmlTag.DT, body); }
    public static HtmlTree EM(Content body) { return new HtmlTree(HtmlTag.EM, body); }
    public static HtmlTree HEADING(HtmlTag headingTag, Content body) { return new HtmlTree(headingTag, body); }
    public static HtmlTree HEADING(HtmlTag headingTag, boolean printTitle, HtmlStyle styleClass, Content body) { return new HtmlTree(headingTag, body); }
    public static HtmlTree HEADING(HtmlTag headingTag, HtmlStyle styleClass, Content body) { return new HtmlTree(headingTag, body); }
    public static HtmlTree HR() { return new HtmlTree(HtmlTag.HR); }
    public static HtmlTree HTML(String lang, Content head, Content body) { return new HtmlTree(HtmlTag.HTML); }
    public static HtmlTree LI(Content body) { return new HtmlTree(HtmlTag.LI, body); }
    public static HtmlTree LI(HtmlStyle styleClass, Content body) { return new HtmlTree(HtmlTag.LI, body); }
    public static HtmlTree LINK(String rel, String type, String href, String title) { return new HtmlTree(HtmlTag.LINK); }
    public static HtmlTree META(String httpEquiv, String content, String charSet) { return new HtmlTree(HtmlTag.META); }
    public static HtmlTree NOSCRIPT(Content body) { return new HtmlTree(HtmlTag.NOSCRIPT, body); }
    public static HtmlTree OL(Content body) { return new HtmlTree(HtmlTag.OL, body); }
    public static HtmlTree P(Content body) { return new HtmlTree(HtmlTag.P, body); }
    public static HtmlTree P(HtmlStyle styleClass, Content body) { return new HtmlTree(HtmlTag.P, body); }
    public static HtmlTree PRE(Content body) { return new HtmlTree(HtmlTag.PRE, body); }
    public static HtmlTree SCRIPT(String src) { return new HtmlTree(HtmlTag.SCRIPT); }
    public static HtmlTree SMALL(Content body) { return new HtmlTree(HtmlTag.SMALL, body); }
    public static HtmlTree SPAN(Content body) { return new HtmlTree(HtmlTag.SPAN, body); }
    public static HtmlTree SPAN(HtmlStyle styleClass, Content body) { return new HtmlTree(HtmlTag.SPAN, body); }
    public static HtmlTree STRONG(Content body) { return new HtmlTree(HtmlTag.STRONG, body); }
    public static HtmlTree TABLE(int border, int cellPadding, int cellSpacing, String summary, Content body) { return new HtmlTree(HtmlTag.TABLE, body); }
    public static HtmlTree TABLE(HtmlStyle styleClass, int border, int cellPadding, int cellSpacing, String summary, Content body) { return new HtmlTree(HtmlTag.TABLE, body); }
    public static HtmlTree TD(Content body) { return new HtmlTree(HtmlTag.TD, body); }
    public static HtmlTree TD(HtmlStyle styleClass, Content body) { return new HtmlTree(HtmlTag.TD, body); }
    public static HtmlTree TH(Content body) { return new HtmlTree(HtmlTag.TH, body); }
    public static HtmlTree TH(HtmlStyle styleClass, String scope, Content body) { return new HtmlTree(HtmlTag.TH, body); }
    public static HtmlTree TITLE(Content body) { return new HtmlTree(HtmlTag.TITLE, body); }
    public static HtmlTree TR(Content body) { return new HtmlTree(HtmlTag.TR, body); }
    public static HtmlTree UL(Content body) { return new HtmlTree(HtmlTag.UL, body); }
    public static HtmlTree UL(HtmlStyle styleClass, Content body) { return new HtmlTree(HtmlTag.UL, body); }
}
