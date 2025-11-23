package java.util;

/**
 * This class consists of static utility methods for operating on objects.
 * These utilities include null-safe or null-tolerant methods for computing
 * the hash code of an object, returning a string for an object, and comparing
 * two objects.
 *
 * @since 1.7
 */
public final class Objects {
    private Objects() {
        throw new AssertionError("No java.util.Objects instances for you!");
    }

    /**
     * Returns true if the arguments are equal to each other and false otherwise.
     * Consequently, if both arguments are null, true is returned and if exactly
     * one argument is null, false is returned. Otherwise, equality is determined
     * by using the equals method of the first argument.
     *
     * @param a an object
     * @param b an object to be compared with a for equality
     * @return true if the arguments are equal to each other and false otherwise
     */
    public static boolean equals(Object a, Object b) {
        return (a == b) || (a != null && a.equals(b));
    }

    /**
     * Returns true if the arguments are deeply equal to each other and false otherwise.
     *
     * @param a an object
     * @param b an object to be compared with a for deep equality
     * @return true if the arguments are deeply equal to each other and false otherwise
     */
    public static boolean deepEquals(Object a, Object b) {
        if (a == b) {
            return true;
        } else if (a == null || b == null) {
            return false;
        } else {
            return a.equals(b);
        }
    }

    /**
     * Returns the hash code of a non-null argument and 0 for a null argument.
     *
     * @param o an object
     * @return the hash code of a non-null argument and 0 for a null argument
     */
    public static int hashCode(Object o) {
        return o != null ? o.hashCode() : 0;
    }

    /**
     * Generates a hash code for a sequence of input values.
     *
     * @param values the values to be hashed
     * @return a hash value of the sequence of input values
     */
    public static int hash(Object... values) {
        int result = 1;
        if (values != null) {
            for (Object element : values) {
                result = 31 * result + (element == null ? 0 : element.hashCode());
            }
        }
        return result;
    }

    /**
     * Returns the result of calling toString for a non-null argument and "null"
     * for a null argument.
     *
     * @param o an object
     * @return the result of calling toString for a non-null argument and "null"
     *         for a null argument
     */
    public static String toString(Object o) {
        return String.valueOf(o);
    }

    /**
     * Returns the result of calling toString on the first argument if the first
     * argument is not null and returns the second argument otherwise.
     *
     * @param o an object
     * @param nullDefault string to return if the first argument is null
     * @return the result of calling toString on the first argument if it is not
     *         null and the second argument otherwise
     */
    public static String toString(Object o, String nullDefault) {
        return (o != null) ? o.toString() : nullDefault;
    }

    /**
     * Returns 0 if the arguments are identical and c.compare(a, b) otherwise.
     *
     * @param <T> the type of the objects being compared
     * @param a an object
     * @param b an object to be compared with a
     * @param c the Comparator to compare the first two arguments
     * @return 0 if the arguments are identical and c.compare(a, b) otherwise
     */
    public static <T> int compare(T a, T b, Comparable<? super T> c) {
        return (a == b) ? 0 : ((Comparable<T>)c).compareTo(b);
    }

    /**
     * Checks that the specified object reference is not null. This method is
     * designed primarily for doing parameter validation in methods and constructors.
     *
     * @param obj the object reference to check for nullity
     * @param <T> the type of the reference
     * @return obj if not null
     * @throws NullPointerException if obj is null
     */
    public static <T> T requireNonNull(T obj) {
        if (obj == null) {
            throw new NullPointerException();
        }
        return obj;
    }

    /**
     * Checks that the specified object reference is not null and throws a
     * customized NullPointerException if it is.
     *
     * @param obj the object reference to check for nullity
     * @param message detail message to be used in the event that a
     *                NullPointerException is thrown
     * @param <T> the type of the reference
     * @return obj if not null
     * @throws NullPointerException if obj is null
     */
    public static <T> T requireNonNull(T obj, String message) {
        if (obj == null) {
            throw new NullPointerException(message);
        }
        return obj;
    }
}
