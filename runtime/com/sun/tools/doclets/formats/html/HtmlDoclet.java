package com.sun.tools.doclets.formats.html;

import com.sun.javadoc.RootDoc;
import com.sun.javadoc.LanguageVersion;

public class HtmlDoclet {
    public HtmlDoclet() {}
    public static boolean start(RootDoc root) { return true; }
    public static int optionLength(String option) { return 0; }
    public static LanguageVersion languageVersion() { return LanguageVersion.JAVA_1_5; }
}
