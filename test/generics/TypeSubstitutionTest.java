// Test type substitution - method return types should be substituted
class Box<T> {
    public T value;  // Made public for direct access

    void set(T v) {
        value = v;
    }

    T get() {
        return value;
    }
}

public class TypeSubstitutionTest {
    public static void main(String[] args) {
        Box<String> stringBox = new Box<String>();

        // Method call - this should work
        String s = stringBox.get();

        // Field access - this should ALSO work if type substitution is implemented
        String s2 = stringBox.value;

        stringBox.set("hello");

        System.out.println("Type substitution works!");
    }
}

class String {
    // Stub for compilation
}
