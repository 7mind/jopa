package java.util;

import java.io.Serializable;

public final class Currency implements Serializable {
    private Currency() {}

    public static Currency getInstance(String currencyCode) { return null; }
    public static Currency getInstance(Locale locale) { return null; }
    public static Set<Currency> getAvailableCurrencies() { return null; }
    public String getCurrencyCode() { return null; }
    public String getSymbol() { return null; }
    public String getSymbol(Locale locale) { return null; }
    public int getDefaultFractionDigits() { return 0; }
    public int getNumericCode() { return 0; }
    public String getDisplayName() { return null; }
    public String getDisplayName(Locale locale) { return null; }
    public String toString() { return null; }
}
