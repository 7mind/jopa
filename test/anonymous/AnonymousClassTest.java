// Test anonymous classes

interface Greeting {
    String greet(String name);
}

interface Counter {
    int count();
}

abstract class Shape {
    abstract double area();
    public String describe() {
        return "A shape";
    }
}

public class AnonymousClassTest {
    static int passed = 0;
    static int failed = 0;

    static void test(String name, boolean condition) {
        if (condition) {
            passed++;
            System.out.println("PASS: " + name);
        } else {
            failed++;
            System.out.println("FAIL: " + name);
        }
    }

    // Instance field for capturing
    private String instancePrefix = "Instance: ";

    static void testAnonymousInterfaceImplementation() {
        System.out.println("\n--- Anonymous Interface Implementation ---");

        // Test 1: Anonymous class implementing interface
        Greeting hello = new Greeting() {
            public String greet(String name) {
                return "Hello, " + name;
            }
        };
        test("1.1 Anonymous interface impl", hello.greet("World").equals("Hello, World"));

        // Test 2: Multiple anonymous instances of same interface
        Greeting goodbye = new Greeting() {
            public String greet(String name) {
                return "Goodbye, " + name;
            }
        };
        test("1.2 Multiple anonymous impls", goodbye.greet("World").equals("Goodbye, World"));

        // Test 3: Anonymous class with state
        final int[] counter = new int[1];
        counter[0] = 0;
        Counter c = new Counter() {
            public int count() {
                counter[0]++;
                return counter[0];
            }
        };
        test("1.3 Anonymous with captured array", c.count() == 1);
        test("1.4 Anonymous counter second call", c.count() == 2);
    }

    static void testAnonymousAbstractClassExtension() {
        System.out.println("\n--- Anonymous Abstract Class Extension ---");

        // Test 1: Anonymous class extending abstract class
        Shape circle = new Shape() {
            private double radius = 5.0;

            double area() {
                return 3.14159 * radius * radius;
            }
        };
        double area = circle.area();
        test("2.1 Anonymous extends abstract class", area > 78.0 && area < 79.0);

        // Test 2: Inherited method works
        test("2.2 Inherited method", circle.describe().equals("A shape"));

        // Test 3: Override both abstract and concrete
        Shape rect = new Shape() {
            private double width = 4.0;
            private double height = 5.0;

            double area() {
                return width * height;
            }

            public String describe() {
                return "A rectangle";
            }
        };
        test("2.3 Anonymous abstract impl", rect.area() == 20.0);
        test("2.4 Anonymous override concrete", rect.describe().equals("A rectangle"));
    }

    static void testCapturingLocalVariables() {
        System.out.println("\n--- Capturing Local Variables ---");

        // Test 1: Capture effectively final local
        final String prefix = "Prefix: ";
        Greeting g = new Greeting() {
            public String greet(String name) {
                return prefix + name;
            }
        };
        test("3.1 Capture final local", g.greet("test").equals("Prefix: test"));

        // Test 2: Capture multiple locals
        final int x = 10;
        final int y = 20;
        Counter c = new Counter() {
            public int count() {
                return x + y;
            }
        };
        test("3.2 Capture multiple locals", c.count() == 30);
    }

    void testCapturingInstanceVariables() {
        System.out.println("\n--- Capturing Instance Variables ---");

        // Test 1: Access instance field from anonymous class
        Greeting g = new Greeting() {
            public String greet(String name) {
                return instancePrefix + name;
            }
        };
        test("4.1 Access instance field", g.greet("test").equals("Instance: test"));
    }

    static void testNestedAnonymousClasses() {
        System.out.println("\n--- Nested Anonymous Classes ---");

        // Test 1: Anonymous class creating another anonymous class
        Greeting outer = new Greeting() {
            public String greet(String name) {
                Greeting inner = new Greeting() {
                    public String greet(String n) {
                        return "Inner: " + n;
                    }
                };
                return "Outer(" + inner.greet(name) + ")";
            }
        };
        test("5.1 Nested anonymous classes", outer.greet("test").equals("Outer(Inner: test)"));
    }

    public static void main(String[] args) {
        System.out.println("=== Anonymous Class Test ===");

        testAnonymousInterfaceImplementation();
        testAnonymousAbstractClassExtension();
        testCapturingLocalVariables();

        AnonymousClassTest instance = new AnonymousClassTest();
        instance.testCapturingInstanceVariables();

        testNestedAnonymousClasses();

        System.out.println("\n=== RESULTS ===");
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);

        if (failed > 0) {
            System.exit(1);
        }
    }
}
