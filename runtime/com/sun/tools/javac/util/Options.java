package com.sun.tools.javac.util;

import java.util.Map;
import java.util.Set;

public class Options {
    public static Options instance(Context context) { return null; }
    public String get(String name) { return null; }
    public String get(OptionName name) { return null; }
    public boolean getBoolean(String name) { return false; }
    public boolean getBoolean(String name, boolean defaultValue) { return defaultValue; }
    public boolean isSet(String name) { return false; }
    public boolean isSet(OptionName name) { return false; }
    public boolean isUnset(String name) { return true; }
    public boolean isUnset(OptionName name) { return true; }
    public void put(String name, String value) {}
    public void put(OptionName name, String value) {}
    public void putAll(Options options) {}
    public void remove(String name) {}
    public Set<String> keySet() { return null; }
    public int size() { return 0; }

    public static final Context.Key<Options> optionsKey = new Context.Key<Options>();
}
