package generics;

// Test case for bug: implementing Comparable<T> and CharSequence
// together causes corrupted class file output
public class MultiInterfaceGenericsTest implements Comparable<MultiInterfaceGenericsTest>, CharSequence {
    private String data;

    public MultiInterfaceGenericsTest(String data) {
        this.data = data;
    }

    // CharSequence methods
    public int length() {
        return data.length();
    }

    public char charAt(int index) {
        return data.charAt(index);
    }

    public CharSequence subSequence(int start, int end) {
        return new MultiInterfaceGenericsTest(data.substring(start, end));
    }

    // Comparable method
    public int compareTo(MultiInterfaceGenericsTest other) {
        return this.data.length() - other.data.length();
    }

    public String toString() {
        return data;
    }

    public static void main(String[] args) {
        MultiInterfaceGenericsTest a = new MultiInterfaceGenericsTest("hello");
        MultiInterfaceGenericsTest b = new MultiInterfaceGenericsTest("hi");

        // Test CharSequence methods
        if (a.length() != 5) {
            System.out.println("FAIL: length() returned " + a.length() + ", expected 5");
            System.exit(1);
        }
        if (a.charAt(0) != 'h') {
            System.out.println("FAIL: charAt(0) returned " + a.charAt(0) + ", expected 'h'");
            System.exit(1);
        }
        CharSequence sub = a.subSequence(0, 2);
        if (sub.length() != 2) {
            System.out.println("FAIL: subSequence length is " + sub.length() + ", expected 2");
            System.exit(1);
        }

        // Test Comparable method
        if (a.compareTo(b) <= 0) {
            System.out.println("FAIL: compareTo returned " + a.compareTo(b) + ", expected > 0");
            System.exit(1);
        }

        System.out.println("PASS");
    }
}
