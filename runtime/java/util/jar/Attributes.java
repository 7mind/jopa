package java.util.jar;

import java.util.Collection;
import java.util.Map;
import java.util.Set;

public class Attributes implements Map<Object, Object>, Cloneable {
    public Attributes() {}

    public Attributes(int size) {}

    public Attributes(Attributes attr) {}

    public Object get(Object name) {
        return null;
    }

    public String getValue(String name) {
        return null;
    }

    public String getValue(Name name) {
        return null;
    }

    public Object put(Object name, Object value) {
        return null;
    }

    public String putValue(String name, String value) {
        return null;
    }

    public Object remove(Object name) {
        return null;
    }

    public boolean containsValue(Object value) {
        return false;
    }

    public boolean containsKey(Object name) {
        return false;
    }

    public void putAll(Map<?, ?> attr) {}

    public void clear() {}

    public int size() {
        return 0;
    }

    public boolean isEmpty() {
        return false;
    }

    public Set<Object> keySet() {
        return null;
    }

    public Collection<Object> values() {
        return null;
    }

    public Set<Map.Entry<Object, Object>> entrySet() {
        return null;
    }

    public boolean equals(Object o) {
        return false;
    }

    public int hashCode() {
        return 0;
    }

    public Object clone() {
        return null;
    }

    public static class Name {
        public static final Name MANIFEST_VERSION = new Name("Manifest-Version");
        public static final Name MAIN_CLASS = new Name("Main-Class");
        public static final Name CLASS_PATH = new Name("Class-Path");

        public Name(String name) {}

        public boolean equals(Object o) {
            return false;
        }

        public int hashCode() {
            return 0;
        }

        public String toString() {
            return null;
        }
    }
}
