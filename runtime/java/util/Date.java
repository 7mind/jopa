package java.util;

import java.io.Serializable;

public class Date implements Serializable, Cloneable, Comparable<Date> {
    public Date() {}
    public Date(long date) {}
    public Date(int year, int month, int date) {}
    public Date(int year, int month, int date, int hrs, int min) {}
    public Date(int year, int month, int date, int hrs, int min, int sec) {}
    public Date(String s) {}

    public Object clone() { return null; }
    public static long UTC(int year, int month, int date, int hrs, int min, int sec) { return 0L; }
    public static long parse(String s) { return 0L; }

    public int getYear() { return 0; }
    public void setYear(int year) {}
    public int getMonth() { return 0; }
    public void setMonth(int month) {}
    public int getDate() { return 0; }
    public void setDate(int date) {}
    public int getDay() { return 0; }
    public int getHours() { return 0; }
    public void setHours(int hours) {}
    public int getMinutes() { return 0; }
    public void setMinutes(int minutes) {}
    public int getSeconds() { return 0; }
    public void setSeconds(int seconds) {}
    public long getTime() { return 0L; }
    public void setTime(long time) {}
    public boolean before(Date when) { return false; }
    public boolean after(Date when) { return false; }
    public boolean equals(Object obj) { return false; }
    public int compareTo(Date anotherDate) { return 0; }
    public int hashCode() { return 0; }
    public String toString() { return ""; }
    public String toLocaleString() { return ""; }
    public String toGMTString() { return ""; }
    public int getTimezoneOffset() { return 0; }
}
