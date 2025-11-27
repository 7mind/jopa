// Test recursive/self-referential generics like Enum<E extends Enum<E>>
// These require proper handling of type parameters within their own bounds

public class RecursiveGenerics {
    // Simple self-referential generic
    public static class SelfBounded<T extends SelfBounded<T>> {
        T self;

        public T getSelf() {
            return self;
        }

        public void setSelf(T value) {
            self = value;
        }
    }

    // Concrete subclass
    public static class Concrete extends SelfBounded<Concrete> {
        String name;

        public Concrete(String name) {
            this.name = name;
            this.self = this;
        }

        public String getName() {
            return name;
        }
    }

    // Test Comparable pattern (like Enum uses)
    public static abstract class ComparableSelf<E extends ComparableSelf<E>>
            implements Comparable<E> {
        private final int value;

        protected ComparableSelf(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }

        public int compareTo(E other) {
            return this.value - other.getValue();
        }
    }

    public static class MyComparable extends ComparableSelf<MyComparable> {
        public MyComparable(int value) {
            super(value);
        }
    }

    // Mutual recursion
    public static class MutualA<T extends MutualB<T, U>, U extends MutualA<T, U>> {
        T other;
    }

    public static class MutualB<T extends MutualB<T, U>, U extends MutualA<T, U>> {
        U other;
    }

    public static void main(String[] args) {
        // Test SelfBounded
        Concrete c1 = new Concrete("first");
        Concrete c2 = c1.getSelf();
        if (!c2.getName().equals("first")) {
            System.out.println("FAIL: SelfBounded.getSelf() returned wrong object");
            System.exit(1);
        }

        // Test ComparableSelf
        MyComparable mc1 = new MyComparable(10);
        MyComparable mc2 = new MyComparable(20);

        if (mc1.compareTo(mc2) >= 0) {
            System.out.println("FAIL: compareTo should be negative");
            System.exit(1);
        }

        if (mc2.compareTo(mc1) <= 0) {
            System.out.println("FAIL: compareTo should be positive");
            System.exit(1);
        }

        if (mc1.compareTo(mc1) != 0) {
            System.out.println("FAIL: compareTo self should be zero");
            System.exit(1);
        }

        // Test that bridge method works (Comparable<Object> interface)
        Comparable<MyComparable> comp = mc1;
        if (comp.compareTo(mc2) >= 0) {
            System.out.println("FAIL: Comparable interface not working");
            System.exit(1);
        }

        System.out.println("PASS: RecursiveGenerics works");
    }
}
