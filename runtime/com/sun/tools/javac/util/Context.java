package com.sun.tools.javac.util;

public class Context {
    public Context() {}
    public <T> T get(Key<T> key) { return null; }
    public <T> void put(Key<T> key, T data) {}
    public <T> void put(Key<T> key, Factory<T> fac) {}

    public static class Key<T> {
        public Key() {}
    }

    public static interface Factory<T> {
        T make(Context c);
    }
}
