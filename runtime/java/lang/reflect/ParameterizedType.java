package java.lang.reflect;

/**
 * ParameterizedType represents a parameterized type such as
 * Collection&lt;String&gt;.
 *
 * A parameterized type is created the first time it is needed by a
 * reflective method. When a parameterized type p is created, the
 * generic type declaration that p instantiates is resolved, and
 * all type arguments of p are created recursively.
 */
public interface ParameterizedType extends Type {
    /**
     * Returns an array of Type objects representing the actual type
     * arguments to this type.
     *
     * Note that in some cases, the returned array be empty. This can occur
     * if this type represents a non-parameterized type nested within
     * a parameterized type.
     *
     * @return an array of Type objects representing the actual type
     *         arguments to this type
     */
    Type[] getActualTypeArguments();

    /**
     * Returns the Type object representing the class or interface
     * that declared this type.
     *
     * @return the Type object representing the class or interface
     *         that declared this type
     */
    Type getRawType();

    /**
     * Returns a Type object representing the type that this type
     * is a member of. For example, if this type is O&lt;T&gt;.I&lt;S&gt;,
     * return a representation of O&lt;T&gt;.
     *
     * If this type is a top-level type, null is returned.
     *
     * @return a Type object representing the type that this type
     *         is a member of. If this type is a top-level type,
     *         null is returned
     */
    Type getOwnerType();
}
