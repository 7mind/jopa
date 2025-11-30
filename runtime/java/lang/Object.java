package java.lang;

public class Object {
    public Object() {}

    public boolean equals(Object obj) {
        return this == obj;
    }

    public int hashCode() {
        return 0;
    }

    public String toString() {
        return "";
    }

    protected native Object clone() throws CloneNotSupportedException;

    public final native Class<?> getClass();

    protected void finalize() throws Throwable {}

    public final native void notify();

    public final native void notifyAll();

    public final native void wait() throws InterruptedException;

    public final native void wait(long timeout) throws InterruptedException;

    public final native void wait(long timeout, int nanos) throws InterruptedException;
}
