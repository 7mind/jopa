package java.util;

import java.io.Serializable;

public final class Locale implements Cloneable, Serializable {
    public static final Locale ENGLISH = new Locale("en", "");
    public static final Locale US = new Locale("en", "US");
    public static final Locale UK = new Locale("en", "GB");
    public static final Locale ROOT = new Locale("", "");

    public Locale(String language) {}
    public Locale(String language, String country) {}
    public Locale(String language, String country, String variant) {}

    public static Locale getDefault() { return US; }
    public static void setDefault(Locale newLocale) {}

    public String getLanguage() { return ""; }
    public String getCountry() { return ""; }
    public String getVariant() { return ""; }
    public String getDisplayLanguage() { return ""; }
    public String getDisplayCountry() { return ""; }
    public String getDisplayName() { return ""; }
    public String toString() { return ""; }
    public String toLanguageTag() { return ""; }

    public static Locale forLanguageTag(String languageTag) { return US; }
    public static Locale[] getAvailableLocales() { return new Locale[0]; }
}
