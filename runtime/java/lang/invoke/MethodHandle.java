package java.lang.invoke;

public abstract class MethodHandle {
    MethodHandle() {}

    public MethodType type() { return null; }
    public Object invokeExact(Object... args) throws Throwable { return null; }
    public Object invoke(Object... args) throws Throwable { return null; }
    public Object invokeWithArguments(Object... arguments) throws Throwable { return null; }
    public Object invokeWithArguments(java.util.List<?> arguments) throws Throwable { return null; }
    public MethodHandle asType(MethodType newType) { return null; }
    public MethodHandle asSpreader(Class<?> arrayType, int arrayLength) { return null; }
    public MethodHandle asCollector(Class<?> arrayType, int arrayLength) { return null; }
    public MethodHandle asVarargsCollector(Class<?> arrayType) { return null; }
    public boolean isVarargsCollector() { return false; }
    public MethodHandle asFixedArity() { return null; }
    public MethodHandle bindTo(Object x) { return null; }
}
