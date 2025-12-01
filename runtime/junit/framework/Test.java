package junit.framework;
public interface Test {
    public abstract int countTestCases();
    public abstract void run(TestResult result);
}
