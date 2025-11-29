package java.util;

public abstract class EnumSet<E extends Enum<E>> extends AbstractSet<E> implements Cloneable, java.io.Serializable {
    public static <E extends Enum<E>> EnumSet<E> noneOf(Class<E> elementType) { return null; }
    public static <E extends Enum<E>> EnumSet<E> allOf(Class<E> elementType) { return null; }
    public static <E extends Enum<E>> EnumSet<E> copyOf(EnumSet<E> s) { return null; }
    public static <E extends Enum<E>> EnumSet<E> copyOf(Collection<E> c) { return null; }
    public static <E extends Enum<E>> EnumSet<E> complementOf(EnumSet<E> s) { return null; }
    public static <E extends Enum<E>> EnumSet<E> of(E e) { return null; }
    public static <E extends Enum<E>> EnumSet<E> of(E e1, E e2) { return null; }
    public static <E extends Enum<E>> EnumSet<E> of(E e1, E e2, E e3) { return null; }
    public static <E extends Enum<E>> EnumSet<E> of(E e1, E e2, E e3, E e4) { return null; }
    public static <E extends Enum<E>> EnumSet<E> of(E e1, E e2, E e3, E e4, E e5) { return null; }
    public static <E extends Enum<E>> EnumSet<E> of(E first, E... rest) { return null; }
    public static <E extends Enum<E>> EnumSet<E> range(E from, E to) { return null; }
    public EnumSet<E> clone() { return null; }
}
