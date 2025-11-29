package java.util;

import java.io.Serializable;

public abstract class TimeZone implements Serializable, Cloneable {
    public static final int SHORT = 0;
    public static final int LONG = 1;

    public TimeZone() {}

    public abstract int getOffset(int era, int year, int month, int day, int dayOfWeek, int milliseconds);
    public int getOffset(long date) { return 0; }
    public abstract void setRawOffset(int offsetMillis);
    public abstract int getRawOffset();
    public String getID() { return null; }
    public void setID(String ID) {}
    public final String getDisplayName() { return null; }
    public final String getDisplayName(Locale locale) { return null; }
    public final String getDisplayName(boolean daylight, int style) { return null; }
    public String getDisplayName(boolean daylight, int style, Locale locale) { return null; }
    public int getDSTSavings() { return 0; }
    public abstract boolean useDaylightTime();
    public boolean observesDaylightTime() { return false; }
    public abstract boolean inDaylightTime(Date date);

    public static synchronized TimeZone getTimeZone(String ID) { return null; }
    public static synchronized String[] getAvailableIDs(int rawOffset) { return null; }
    public static synchronized String[] getAvailableIDs() { return null; }
    public static TimeZone getDefault() { return null; }
    public static void setDefault(TimeZone zone) {}
    public boolean hasSameRules(TimeZone other) { return false; }
    public Object clone() { return null; }
}
