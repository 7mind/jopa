public class GenericMethodTest {
    // Simple generic method: T identity(T arg)
    public static <T> T identity(T arg) {
        return arg;
    }

    // Generic method with parameterized argument: T extract(Wrapper<T> wrapper)
    public static <T> T extract(Wrapper<T> wrapper) {
        return wrapper.get();
    }

    public static void main(String[] args) {
        // Test simple generic method
        String s1 = identity("hello");
        Integer i1 = identity(Integer.valueOf(42));

        // Test with anonymous class creation
        String s2 = extract(new Wrapper<String>("anon") {});

        if ("hello".equals(s1) && i1.intValue() == 42 && "anon".equals(s2)) {
            System.out.println("GenericMethodTest: PASS");
        } else {
            System.out.println("GenericMethodTest: FAIL");
        }
    }
}

class Wrapper<T> {
    private T value;

    public Wrapper(T value) {
        this.value = value;
    }

    public T get() {
        return value;
    }
}
