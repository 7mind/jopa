package java.util;

import java.io.Serializable;

public abstract class Calendar implements Serializable, Cloneable, Comparable<Calendar> {
    public static final int ERA = 0;
    public static final int YEAR = 1;
    public static final int MONTH = 2;
    public static final int WEEK_OF_YEAR = 3;
    public static final int WEEK_OF_MONTH = 4;
    public static final int DATE = 5;
    public static final int DAY_OF_MONTH = 5;
    public static final int DAY_OF_YEAR = 6;
    public static final int DAY_OF_WEEK = 7;
    public static final int DAY_OF_WEEK_IN_MONTH = 8;
    public static final int AM_PM = 9;
    public static final int HOUR = 10;
    public static final int HOUR_OF_DAY = 11;
    public static final int MINUTE = 12;
    public static final int SECOND = 13;
    public static final int MILLISECOND = 14;
    public static final int ZONE_OFFSET = 15;
    public static final int DST_OFFSET = 16;
    public static final int FIELD_COUNT = 17;
    public static final int SUNDAY = 1;
    public static final int MONDAY = 2;
    public static final int TUESDAY = 3;
    public static final int WEDNESDAY = 4;
    public static final int THURSDAY = 5;
    public static final int FRIDAY = 6;
    public static final int SATURDAY = 7;
    public static final int JANUARY = 0;
    public static final int FEBRUARY = 1;
    public static final int MARCH = 2;
    public static final int APRIL = 3;
    public static final int MAY = 4;
    public static final int JUNE = 5;
    public static final int JULY = 6;
    public static final int AUGUST = 7;
    public static final int SEPTEMBER = 8;
    public static final int OCTOBER = 9;
    public static final int NOVEMBER = 10;
    public static final int DECEMBER = 11;
    public static final int UNDECIMBER = 12;
    public static final int AM = 0;
    public static final int PM = 1;

    protected Calendar() {}
    protected Calendar(TimeZone zone, Locale aLocale) {}

    public static Calendar getInstance() { return null; }
    public static Calendar getInstance(TimeZone zone) { return null; }
    public static Calendar getInstance(Locale aLocale) { return null; }
    public static Calendar getInstance(TimeZone zone, Locale aLocale) { return null; }
    public static synchronized Locale[] getAvailableLocales() { return null; }

    protected abstract void computeTime();
    protected abstract void computeFields();

    public final Date getTime() { return null; }
    public final void setTime(Date date) {}
    public long getTimeInMillis() { return 0; }
    public void setTimeInMillis(long millis) {}
    public int get(int field) { return 0; }
    public void set(int field, int value) {}
    public void set(int year, int month, int date) {}
    public void set(int year, int month, int date, int hourOfDay, int minute) {}
    public void set(int year, int month, int date, int hourOfDay, int minute, int second) {}
    public final void clear() {}
    public final void clear(int field) {}
    public final boolean isSet(int field) { return false; }
    public void add(int field, int amount) {}
    public void roll(int field, boolean up) {}
    public void roll(int field, int amount) {}
    public void setTimeZone(TimeZone value) {}
    public TimeZone getTimeZone() { return null; }
    public void setLenient(boolean lenient) {}
    public boolean isLenient() { return false; }
    public void setFirstDayOfWeek(int value) {}
    public int getFirstDayOfWeek() { return 0; }
    public void setMinimalDaysInFirstWeek(int value) {}
    public int getMinimalDaysInFirstWeek() { return 0; }
    public boolean before(Object when) { return false; }
    public boolean after(Object when) { return false; }
    public int compareTo(Calendar anotherCalendar) { return 0; }
    public abstract int getMinimum(int field);
    public abstract int getMaximum(int field);
    public abstract int getGreatestMinimum(int field);
    public abstract int getLeastMaximum(int field);
    public int getActualMinimum(int field) { return 0; }
    public int getActualMaximum(int field) { return 0; }
    public Object clone() { return null; }
}
