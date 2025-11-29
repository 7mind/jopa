package java.text;

import java.util.Locale;
import java.io.Serializable;

public class DateFormatSymbols implements Serializable, Cloneable {
    public DateFormatSymbols() {}
    public DateFormatSymbols(Locale locale) {}

    public static Locale[] getAvailableLocales() { return null; }
    public static final DateFormatSymbols getInstance() { return null; }
    public static final DateFormatSymbols getInstance(Locale locale) { return null; }

    public String[] getEras() { return null; }
    public void setEras(String[] newEras) {}
    public String[] getMonths() { return null; }
    public void setMonths(String[] newMonths) {}
    public String[] getShortMonths() { return null; }
    public void setShortMonths(String[] newShortMonths) {}
    public String[] getWeekdays() { return null; }
    public void setWeekdays(String[] newWeekdays) {}
    public String[] getShortWeekdays() { return null; }
    public void setShortWeekdays(String[] newShortWeekdays) {}
    public String[] getAmPmStrings() { return null; }
    public void setAmPmStrings(String[] newAmpms) {}
    public String[][] getZoneStrings() { return null; }
    public void setZoneStrings(String[][] newZoneStrings) {}
    public String getLocalPatternChars() { return null; }
    public void setLocalPatternChars(String newLocalPatternChars) {}
    public Object clone() { return null; }
}
