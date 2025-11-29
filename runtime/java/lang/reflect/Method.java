package java.lang.reflect;

import java.lang.annotation.Annotation;

public class Method implements AnnotatedElement {
    private Method() {}

    public native String getName();
    public native int getModifiers();
    public native Class getReturnType();
    public native Class[] getParameterTypes();
    public native Class[] getExceptionTypes();
    public native Class getDeclaringClass();
    public native Object invoke(Object obj, Object... args) throws IllegalAccessException;
    public native boolean isAccessible();
    public native void setAccessible(boolean flag);
    public native Annotation[] getDeclaredAnnotations();
    public native Annotation[] getAnnotations();
    public native <T extends Annotation> T getAnnotation(Class<T> annotationType);
    public native boolean isAnnotationPresent(Class<? extends Annotation> annotationClass);
}
