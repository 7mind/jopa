// Test anonymous class using type parameter from enclosing generic method

interface Processor<T> {
    void process(T item);
}

public class AnonClassTypeParamTest {
    // Generic method returning anonymous class that uses method's type parameter
    public static <T> Processor<T> createProcessor() {
        return new Processor<T>() {
            public void process(T item) {
                System.out.println("Processing: " + item);
            }
        };
    }

    public static void main(String[] args) {
        Processor<String> p = createProcessor();
        p.process("hello");
        System.out.println("AnonClassTypeParamTest: PASS");
    }
}
