package java.lang;

public class ThreadGroup {
    public ThreadGroup(String name) {}

    public ThreadGroup(ThreadGroup parent, String name) {}

    public final String getName() {
        return null;
    }

    public final ThreadGroup getParent() {
        return null;
    }

    public final boolean isDaemon() {
        return false;
    }

    public final void setDaemon(boolean daemon) {}

    public int activeCount() {
        return 0;
    }

    public int activeGroupCount() {
        return 0;
    }
}
