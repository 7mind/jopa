// Test for Java 7 diamond operator
public class DiamondTest {
    // Custom generic class for testing
    static class Box<T> {
        private T value;

        public Box() {
            this.value = null;
        }

        public Box(T value) {
            this.value = value;
        }

        public T get() {
            return value;
        }

        public void set(T value) {
            this.value = value;
        }
    }

    // Generic class with two type parameters
    static class Pair<K, V> {
        private K first;
        private V second;

        public Pair() {
            this.first = null;
            this.second = null;
        }

        public Pair(K first, V second) {
            this.first = first;
            this.second = second;
        }

        public K getFirst() {
            return first;
        }

        public V getSecond() {
            return second;
        }
    }

    public static void main(String[] args) {
        boolean passed = true;

        // Test 1: Diamond with default constructor
        Box<String> box1 = new Box<>();
        box1.set("Hello");
        if (!"Hello".equals(box1.get())) {
            System.out.println("FAIL: Box default constructor diamond");
            passed = false;
        }

        // Test 2: Diamond with parameterized constructor
        Box<Integer> box2 = new Box<>(Integer.valueOf(42));
        if (!box2.get().equals(Integer.valueOf(42))) {
            System.out.println("FAIL: Box parameterized constructor diamond");
            passed = false;
        }

        // Test 3: Diamond with two type parameters
        Pair<String, Integer> pair = new Pair<>();
        if (pair.getFirst() != null || pair.getSecond() != null) {
            System.out.println("FAIL: Pair default constructor diamond");
            passed = false;
        }

        // Test 4: Diamond with two type parameter constructor
        Pair<String, Double> pair2 = new Pair<>("key", Double.valueOf(3.14));
        if (!"key".equals(pair2.getFirst())) {
            System.out.println("FAIL: Pair parameterized constructor first");
            passed = false;
        }
        if (!pair2.getSecond().equals(Double.valueOf(3.14))) {
            System.out.println("FAIL: Pair parameterized constructor second");
            passed = false;
        }

        if (passed) {
            System.out.println("All diamond operator tests passed!");
        } else {
            System.exit(1);
        }
    }
}
