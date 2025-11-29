package java.lang;

public class Thread implements Runnable {
    public Thread() {}

    public Thread(Runnable target) {}

    public Thread(String name) {}

    public Thread(Runnable target, String name) {}

    public Thread(ThreadGroup group, Runnable target) {}

    public Thread(ThreadGroup group, String name) {}

    public Thread(ThreadGroup group, Runnable target, String name) {}

    public void start() {}

    public void run() {}

    public static Thread currentThread() {
        return null;
    }

    public static void sleep(long millis) throws InterruptedException {}

    public static void sleep(long millis, int nanos) throws InterruptedException {}

    public static void yield() {}

    public final String getName() {
        return null;
    }

    public final void setName(String name) {}

    public final int getPriority() {
        return 0;
    }

    public final void setPriority(int newPriority) {}

    public final boolean isDaemon() {
        return false;
    }

    public final void setDaemon(boolean on) {}

    public final boolean isAlive() {
        return false;
    }

    public final void join() throws InterruptedException {}

    public final void join(long millis) throws InterruptedException {}

    public final void join(long millis, int nanos) throws InterruptedException {}

    public void interrupt() {}

    public static boolean interrupted() {
        return false;
    }

    public boolean isInterrupted() {
        return false;
    }

    public long getId() {
        return 0L;
    }

    public StackTraceElement[] getStackTrace() {
        return null;
    }

    public ClassLoader getContextClassLoader() {
        return null;
    }

    public void setContextClassLoader(ClassLoader cl) {}

    public static int activeCount() {
        return 0;
    }
}
