// Test: type parameter propagation through inheritance chain
// All classes in same file to ensure proper processing

class Base<T> {
    public T get() { return null; }
}

class Middle<U> extends Base<U> {
}

class Leaf extends Middle<String> {
}

public class LocalInheritTest {
    public static void main(String[] args) {
        Leaf leaf = new Leaf();
        String s = leaf.get();  // Should return String through inheritance chain
        System.out.println("LocalInheritTest: PASS");
    }
}
