package java.text;

import java.util.Locale;
import java.util.Currency;

public abstract class NumberFormat extends Format {
    public static final int INTEGER_FIELD = 0;
    public static final int FRACTION_FIELD = 1;

    protected NumberFormat() {}

    public StringBuffer format(Object number, StringBuffer toAppendTo, FieldPosition pos) { return null; }
    public Object parseObject(String source, ParsePosition pos) { return null; }
    public final String format(double number) { return null; }
    public final String format(long number) { return null; }
    public abstract StringBuffer format(double number, StringBuffer toAppendTo, FieldPosition pos);
    public abstract StringBuffer format(long number, StringBuffer toAppendTo, FieldPosition pos);
    public abstract Number parse(String source, ParsePosition parsePosition);
    public Number parse(String source) throws ParseException { return null; }
    public boolean isParseIntegerOnly() { return false; }
    public void setParseIntegerOnly(boolean value) {}

    public static final NumberFormat getInstance() { return null; }
    public static NumberFormat getInstance(Locale inLocale) { return null; }
    public static final NumberFormat getNumberInstance() { return null; }
    public static NumberFormat getNumberInstance(Locale inLocale) { return null; }
    public static final NumberFormat getIntegerInstance() { return null; }
    public static NumberFormat getIntegerInstance(Locale inLocale) { return null; }
    public static final NumberFormat getCurrencyInstance() { return null; }
    public static NumberFormat getCurrencyInstance(Locale inLocale) { return null; }
    public static final NumberFormat getPercentInstance() { return null; }
    public static NumberFormat getPercentInstance(Locale inLocale) { return null; }
    public static Locale[] getAvailableLocales() { return null; }

    public boolean isGroupingUsed() { return false; }
    public void setGroupingUsed(boolean newValue) {}
    public int getMaximumIntegerDigits() { return 0; }
    public void setMaximumIntegerDigits(int newValue) {}
    public int getMinimumIntegerDigits() { return 0; }
    public void setMinimumIntegerDigits(int newValue) {}
    public int getMaximumFractionDigits() { return 0; }
    public void setMaximumFractionDigits(int newValue) {}
    public int getMinimumFractionDigits() { return 0; }
    public void setMinimumFractionDigits(int newValue) {}
    public Currency getCurrency() { return null; }
    public void setCurrency(Currency currency) {}
}
