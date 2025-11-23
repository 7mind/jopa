// Test covariant return types (Java 5 feature)
class Parent {
    TestNumber getValue() {
        return new TestNumber();
    }

    TestNumber getAnotherValue() {
        return new TestNumber();
    }
}

class Child extends Parent {
    // Covariant return - override with more specific return type
    TestInteger getValue() {
        return new TestInteger(42);
    }

    // Another covariant return
    TestInteger getAnotherValue() {
        return new TestInteger(100);
    }
}

public class CovariantReturnTest {
    public static void main(String[] args) {
        Child child = new Child();

        // These should work with covariant returns
        TestInteger i1 = child.getValue();
        TestInteger i2 = child.getAnotherValue();

        // Polymorphism should also work
        Parent p = child;
        TestNumber n1 = p.getValue();
        TestNumber n2 = p.getAnotherValue();

        System.out.println("Covariant return tests passed!");
        System.out.println("i1 = " + i1);
        System.out.println("i2 = " + i2);
        System.out.println("n1 = " + n1);
        System.out.println("n2 = " + n2);
    }
}

class TestNumber {
    int value;

    TestNumber() {
        value = 0;
    }
}

class TestInteger extends TestNumber {
    TestInteger(int v) {
        value = v;
    }
}
