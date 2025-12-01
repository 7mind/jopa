package com.sun.tools.classfile;

import java.util.HashMap;
import java.util.Map;

public class Attributes {
    private final Map<String, Object> attributes = new HashMap<String, Object>();

    public Attributes() {
    }

    public void add(String name, Object value) {
        attributes.put(name, value);
    }

    public Object get(String name) {
        return attributes.get(name);
    }
}
