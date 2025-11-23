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

    protected Object clone() {
        return this;
    }

    public final Class getClass() {
        return null;
    }

    protected void finalize() {}

    public final void notify() {}

    public final void notifyAll() {}

    public final void wait() {}

    public final void wait(long timeout) {}
}
