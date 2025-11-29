package java.lang.invoke;

import java.util.List;

public final class MethodType implements java.io.Serializable {
    private MethodType() {}

    public static MethodType methodType(Class<?> rtype) { return null; }
    public static MethodType methodType(Class<?> rtype, Class<?> ptype0) { return null; }
    public static MethodType methodType(Class<?> rtype, Class<?> ptype0, Class<?>... ptypes) { return null; }
    public static MethodType methodType(Class<?> rtype, Class<?>[] ptypes) { return null; }
    public static MethodType methodType(Class<?> rtype, List<Class<?>> ptypes) { return null; }
    public static MethodType methodType(Class<?> rtype, MethodType ptypes) { return null; }
    public static MethodType genericMethodType(int objectArgCount) { return null; }
    public static MethodType genericMethodType(int objectArgCount, boolean finalArray) { return null; }

    public Class<?> returnType() { return null; }
    public int parameterCount() { return 0; }
    public Class<?> parameterType(int num) { return null; }
    public List<Class<?>> parameterList() { return null; }
    public Class<?>[] parameterArray() { return null; }

    public MethodType changeParameterType(int num, Class<?> nptype) { return null; }
    public MethodType insertParameterTypes(int num, Class<?>... ptypesToInsert) { return null; }
    public MethodType appendParameterTypes(Class<?>... ptypesToInsert) { return null; }
    public MethodType insertParameterTypes(int num, List<Class<?>> ptypesToInsert) { return null; }
    public MethodType appendParameterTypes(List<Class<?>> ptypesToInsert) { return null; }
    public MethodType dropParameterTypes(int start, int end) { return null; }
    public MethodType changeReturnType(Class<?> nrtype) { return null; }

    public boolean hasPrimitives() { return false; }
    public boolean hasWrappers() { return false; }
    public MethodType erase() { return null; }
    public MethodType generic() { return null; }
    public MethodType wrap() { return null; }
    public MethodType unwrap() { return null; }

    public String toMethodDescriptorString() { return null; }
    public static MethodType fromMethodDescriptorString(String descriptor, ClassLoader loader) { return null; }
}
