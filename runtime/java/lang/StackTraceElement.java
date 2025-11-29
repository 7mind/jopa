package java.lang;

public final class StackTraceElement implements java.io.Serializable {
    public StackTraceElement(String declaringClass, String methodName, String fileName, int lineNumber) {}

    public String getFileName() {
        return null;
    }

    public int getLineNumber() {
        return 0;
    }

    public String getClassName() {
        return null;
    }

    public String getMethodName() {
        return null;
    }

    public boolean isNativeMethod() {
        return false;
    }

    public String toString() {
        return null;
    }

    public boolean equals(Object obj) {
        return false;
    }

    public int hashCode() {
        return 0;
    }
}
