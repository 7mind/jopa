package java.util;

public class Collections {
    private Collections() {}

    public static <T> Set<T> emptySet() {
        return null;
    }

    public static <T> List<T> emptyList() {
        return null;
    }

    public static <K, V> Map<K, V> emptyMap() {
        return null;
    }

    public static <T> boolean addAll(Collection<? super T> c, T... elements) {
        return false;
    }

    public static <T> List<T> unmodifiableList(List<? extends T> list) {
        return null;
    }

    public static <T> Set<T> unmodifiableSet(Set<? extends T> s) {
        return null;
    }

    public static <K, V> Map<K, V> unmodifiableMap(Map<? extends K, ? extends V> m) {
        return null;
    }

    public static <T> Collection<T> unmodifiableCollection(Collection<? extends T> c) {
        return null;
    }

    public static <T> List<T> singletonList(T o) {
        return null;
    }

    public static <T> Set<T> singleton(T o) {
        return null;
    }

    public static <K, V> Map<K, V> singletonMap(K key, V value) {
        return null;
    }

    public static <T extends Comparable<? super T>> void sort(List<T> list) {}

    public static <T> void sort(List<T> list, Comparator<? super T> c) {}

    public static void reverse(List<?> list) {}

    public static void shuffle(List<?> list) {}

    public static void shuffle(List<?> list, Random rnd) {}

    public static <T> void fill(List<? super T> list, T obj) {}

    public static <T> void copy(List<? super T> dest, List<? extends T> src) {}

    public static <T extends Object & Comparable<? super T>> T min(Collection<? extends T> coll) {
        return null;
    }

    public static <T extends Object & Comparable<? super T>> T max(Collection<? extends T> coll) {
        return null;
    }

    public static <T> Enumeration<T> enumeration(Collection<T> c) {
        return null;
    }

    public static <T> ArrayList<T> list(Enumeration<T> e) {
        return null;
    }
}
