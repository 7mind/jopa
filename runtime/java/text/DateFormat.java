package java.text;

import java.util.Date;
import java.util.Calendar;
import java.util.TimeZone;
import java.util.Locale;

public abstract class DateFormat extends Format {
    public static final int FULL = 0;
    public static final int LONG = 1;
    public static final int MEDIUM = 2;
    public static final int SHORT = 3;
    public static final int DEFAULT = MEDIUM;

    protected Calendar calendar;
    protected NumberFormat numberFormat;

    protected DateFormat() {}

    public final String format(Date date) { return null; }
    public abstract StringBuffer format(Date date, StringBuffer toAppendTo, FieldPosition fieldPosition);
    public StringBuffer format(Object obj, StringBuffer toAppendTo, FieldPosition fieldPosition) { return null; }
    public Date parse(String source) throws ParseException { return null; }
    public abstract Date parse(String source, ParsePosition pos);
    public Object parseObject(String source, ParsePosition pos) { return null; }

    public static final DateFormat getTimeInstance() { return null; }
    public static final DateFormat getTimeInstance(int style) { return null; }
    public static final DateFormat getTimeInstance(int style, Locale aLocale) { return null; }
    public static final DateFormat getDateInstance() { return null; }
    public static final DateFormat getDateInstance(int style) { return null; }
    public static final DateFormat getDateInstance(int style, Locale aLocale) { return null; }
    public static final DateFormat getDateTimeInstance() { return null; }
    public static final DateFormat getDateTimeInstance(int dateStyle, int timeStyle) { return null; }
    public static final DateFormat getDateTimeInstance(int dateStyle, int timeStyle, Locale aLocale) { return null; }
    public static final DateFormat getInstance() { return null; }

    public void setCalendar(Calendar newCalendar) {}
    public Calendar getCalendar() { return null; }
    public void setNumberFormat(NumberFormat newNumberFormat) {}
    public NumberFormat getNumberFormat() { return null; }
    public void setTimeZone(TimeZone zone) {}
    public TimeZone getTimeZone() { return null; }
    public void setLenient(boolean lenient) {}
    public boolean isLenient() { return false; }
}
