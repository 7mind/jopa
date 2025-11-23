// Test covariant return types (Java 5 feature)
class Parent {
    Number getValue() {
        return new Number();
    }

    Number getAnotherValue() {
        return new Number();
    }
}

class Child extends Parent {
    // Covariant return - override with more specific return type
    Integer getValue() {
        return new Integer(42);
    }

    // Another covariant return
    Integer getAnotherValue() {
        return new Integer(100);
    }
}

public class CovariantReturnTest {
    public static void main(String[] args) {
        Child child = new Child();

        // These should work with covariant returns
        Integer i1 = child.getValue();
        Integer i2 = child.getAnotherValue();

        // Polymorphism should also work
        Parent p = child;
        Number n1 = p.getValue();
        Number n2 = p.getAnotherValue();

        System.out.println("Covariant return tests passed!");
        System.out.println("i1 = " + i1);
        System.out.println("i2 = " + i2);
        System.out.println("n1 = " + n1);
        System.out.println("n2 = " + n2);
    }
}

class Number {
    int value;

    Number() {
        value = 0;
    }
}

class Integer extends Number {
    Integer(int v) {
        value = v;
    }
}
