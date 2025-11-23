// Test type substitution - method return types should be substituted
class Box<T> {
    T value;

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

        // This should work - get() should return String, not Object
        String s = stringBox.get();

        // This should work - set() should take String, not Object
        stringBox.set("hello");

        System.out.println("Type substitution works!");
    }
}

class String {
    // Stub for compilation
}
