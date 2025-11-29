package java.util;

public class Arrays {
    private Arrays() {}

    public static <T> List<T> asList(T... a) { return null; }

    public static void sort(int[] a) {}
    public static void sort(long[] a) {}
    public static void sort(short[] a) {}
    public static void sort(char[] a) {}
    public static void sort(byte[] a) {}
    public static void sort(float[] a) {}
    public static void sort(double[] a) {}
    public static void sort(Object[] a) {}
    public static <T> void sort(T[] a, Comparator<? super T> c) {}

    public static void sort(int[] a, int fromIndex, int toIndex) {}
    public static void sort(long[] a, int fromIndex, int toIndex) {}
    public static void sort(short[] a, int fromIndex, int toIndex) {}
    public static void sort(char[] a, int fromIndex, int toIndex) {}
    public static void sort(byte[] a, int fromIndex, int toIndex) {}
    public static void sort(float[] a, int fromIndex, int toIndex) {}
    public static void sort(double[] a, int fromIndex, int toIndex) {}
    public static void sort(Object[] a, int fromIndex, int toIndex) {}
    public static <T> void sort(T[] a, int fromIndex, int toIndex, Comparator<? super T> c) {}

    public static int binarySearch(int[] a, int key) { return 0; }
    public static int binarySearch(long[] a, long key) { return 0; }
    public static int binarySearch(short[] a, short key) { return 0; }
    public static int binarySearch(char[] a, char key) { return 0; }
    public static int binarySearch(byte[] a, byte key) { return 0; }
    public static int binarySearch(float[] a, float key) { return 0; }
    public static int binarySearch(double[] a, double key) { return 0; }
    public static int binarySearch(Object[] a, Object key) { return 0; }
    public static <T> int binarySearch(T[] a, T key, Comparator<? super T> c) { return 0; }

    public static boolean equals(int[] a, int[] a2) { return false; }
    public static boolean equals(long[] a, long[] a2) { return false; }
    public static boolean equals(short[] a, short[] a2) { return false; }
    public static boolean equals(char[] a, char[] a2) { return false; }
    public static boolean equals(byte[] a, byte[] a2) { return false; }
    public static boolean equals(boolean[] a, boolean[] a2) { return false; }
    public static boolean equals(float[] a, float[] a2) { return false; }
    public static boolean equals(double[] a, double[] a2) { return false; }
    public static boolean equals(Object[] a, Object[] a2) { return false; }

    public static void fill(int[] a, int val) {}
    public static void fill(long[] a, long val) {}
    public static void fill(short[] a, short val) {}
    public static void fill(char[] a, char val) {}
    public static void fill(byte[] a, byte val) {}
    public static void fill(boolean[] a, boolean val) {}
    public static void fill(float[] a, float val) {}
    public static void fill(double[] a, double val) {}
    public static void fill(Object[] a, Object val) {}

    public static <T> T[] copyOf(T[] original, int newLength) { return null; }
    public static <T,U> T[] copyOf(U[] original, int newLength, Class<? extends T[]> newType) { return null; }
    public static byte[] copyOf(byte[] original, int newLength) { return null; }
    public static short[] copyOf(short[] original, int newLength) { return null; }
    public static int[] copyOf(int[] original, int newLength) { return null; }
    public static long[] copyOf(long[] original, int newLength) { return null; }
    public static char[] copyOf(char[] original, int newLength) { return null; }
    public static float[] copyOf(float[] original, int newLength) { return null; }
    public static double[] copyOf(double[] original, int newLength) { return null; }
    public static boolean[] copyOf(boolean[] original, int newLength) { return null; }

    public static <T> T[] copyOfRange(T[] original, int from, int to) { return null; }
    public static <T,U> T[] copyOfRange(U[] original, int from, int to, Class<? extends T[]> newType) { return null; }
    public static byte[] copyOfRange(byte[] original, int from, int to) { return null; }
    public static short[] copyOfRange(short[] original, int from, int to) { return null; }
    public static int[] copyOfRange(int[] original, int from, int to) { return null; }
    public static long[] copyOfRange(long[] original, int from, int to) { return null; }
    public static char[] copyOfRange(char[] original, int from, int to) { return null; }
    public static float[] copyOfRange(float[] original, int from, int to) { return null; }
    public static double[] copyOfRange(double[] original, int from, int to) { return null; }
    public static boolean[] copyOfRange(boolean[] original, int from, int to) { return null; }

    public static int hashCode(int[] a) { return 0; }
    public static int hashCode(long[] a) { return 0; }
    public static int hashCode(short[] a) { return 0; }
    public static int hashCode(char[] a) { return 0; }
    public static int hashCode(byte[] a) { return 0; }
    public static int hashCode(boolean[] a) { return 0; }
    public static int hashCode(float[] a) { return 0; }
    public static int hashCode(double[] a) { return 0; }
    public static int hashCode(Object[] a) { return 0; }

    public static int deepHashCode(Object[] a) { return 0; }
    public static boolean deepEquals(Object[] a1, Object[] a2) { return false; }

    public static String toString(int[] a) { return ""; }
    public static String toString(long[] a) { return ""; }
    public static String toString(short[] a) { return ""; }
    public static String toString(char[] a) { return ""; }
    public static String toString(byte[] a) { return ""; }
    public static String toString(boolean[] a) { return ""; }
    public static String toString(float[] a) { return ""; }
    public static String toString(double[] a) { return ""; }
    public static String toString(Object[] a) { return ""; }
    public static String deepToString(Object[] a) { return ""; }
}
