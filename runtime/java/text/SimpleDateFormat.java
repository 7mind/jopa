package java.text;

import java.util.Date;
import java.util.Locale;

public class SimpleDateFormat extends DateFormat {
    public SimpleDateFormat() {}
    public SimpleDateFormat(String pattern) {}
    public SimpleDateFormat(String pattern, Locale locale) {}
    public SimpleDateFormat(String pattern, DateFormatSymbols formatSymbols) {}

    public void set2DigitYearStart(Date startDate) {}
    public Date get2DigitYearStart() { return null; }

    public StringBuffer format(Date date, StringBuffer toAppendTo, FieldPosition pos) { return null; }
    public Date parse(String text, ParsePosition pos) { return null; }
    public String toPattern() { return null; }
    public String toLocalizedPattern() { return null; }
    public void applyPattern(String pattern) {}
    public void applyLocalizedPattern(String pattern) {}
    public DateFormatSymbols getDateFormatSymbols() { return null; }
    public void setDateFormatSymbols(DateFormatSymbols newFormatSymbols) {}
    public Object clone() { return null; }
}
