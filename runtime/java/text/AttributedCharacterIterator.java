package java.text;

import java.util.Map;
import java.util.Set;
import java.io.Serializable;

public interface AttributedCharacterIterator extends CharacterIterator {
    int getRunStart();
    int getRunStart(Attribute attribute);
    int getRunStart(Set<? extends Attribute> attributes);
    int getRunLimit();
    int getRunLimit(Attribute attribute);
    int getRunLimit(Set<? extends Attribute> attributes);
    Map<Attribute, Object> getAttributes();
    Object getAttribute(Attribute attribute);
    Set<Attribute> getAllAttributeKeys();

    public static class Attribute implements Serializable {
        public static final Attribute LANGUAGE = new Attribute("language");
        public static final Attribute READING = new Attribute("reading");
        public static final Attribute INPUT_METHOD_SEGMENT = new Attribute("input_method_segment");

        private String name;

        protected Attribute(String name) { this.name = name; }
        public final boolean equals(Object obj) { return super.equals(obj); }
        public final int hashCode() { return super.hashCode(); }
        public String toString() { return getClass().getName() + "(" + name + ")"; }
        protected String getName() { return name; }
    }
}
