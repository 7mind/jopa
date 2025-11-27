// Test anonymous classes with generic type parameters (type tags pattern)

interface Processor<T> {
    T process(T input);
    String getTypeName();
}

interface Transformer<S, T> {
    T transform(S input);
}

abstract class TypedContainer<T> {
    abstract T getValue();
    abstract void setValue(T value);
}

abstract class BoundedContainer<T extends Number> {
    abstract T getValue();
    abstract double asDouble();
}

class Wrapper<T> {
    private T value;

    Wrapper(T v) {
        this.value = v;
    }

    T get() {
        return value;
    }

    void set(T v) {
        this.value = v;
    }
}

public class AnonymousGenericTest {
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

    static void testAnonymousGenericInterface() {
        System.out.println("\n--- Anonymous Generic Interface Implementation ---");

        // Test 1: Anonymous class implementing generic interface with String
        Processor<String> stringProcessor = new Processor<String>() {
            public String process(String input) {
                return input.toUpperCase();
            }
            public String getTypeName() {
                return "String";
            }
        };
        test("1.1 Anonymous generic interface (String)",
             stringProcessor.process("hello").equals("HELLO"));
        test("1.2 Type name accessor",
             stringProcessor.getTypeName().equals("String"));

        // Test 2: Anonymous class implementing generic interface with Integer
        Processor<Integer> intProcessor = new Processor<Integer>() {
            public Integer process(Integer input) {
                return Integer.valueOf(input.intValue() * 2);
            }
            public String getTypeName() {
                return "Integer";
            }
        };
        int result = intProcessor.process(Integer.valueOf(21)).intValue();
        test("1.3 Anonymous generic interface (Integer)", result == 42);

        // Test 3: Multiple type parameters
        Transformer<String, Integer> lengthTransformer = new Transformer<String, Integer>() {
            public Integer transform(String input) {
                return Integer.valueOf(input.length());
            }
        };
        int len = lengthTransformer.transform("hello").intValue();
        test("1.4 Anonymous with multiple type params", len == 5);
    }

    static void testAnonymousGenericAbstractClass() {
        System.out.println("\n--- Anonymous Generic Abstract Class Extension ---");

        // Test 1: Anonymous class extending generic abstract class
        TypedContainer<String> stringContainer = new TypedContainer<String>() {
            private String value = "initial";

            String getValue() {
                return value;
            }

            void setValue(String v) {
                this.value = v;
            }
        };
        test("2.1 Anonymous generic abstract class",
             stringContainer.getValue().equals("initial"));

        stringContainer.setValue("updated");
        test("2.2 Generic setter via abstract class",
             stringContainer.getValue().equals("updated"));

        // Test 2: With Integer type parameter
        TypedContainer<Integer> intContainer = new TypedContainer<Integer>() {
            private Integer value = Integer.valueOf(100);

            Integer getValue() {
                return value;
            }

            void setValue(Integer v) {
                this.value = v;
            }
        };
        test("2.3 Anonymous generic abstract (Integer)",
             intContainer.getValue().intValue() == 100);
    }

    static void testAnonymousBoundedGeneric() {
        System.out.println("\n--- Anonymous Bounded Generic ---");

        // Test 1: Anonymous class with bounded type parameter
        BoundedContainer<Integer> intBounded = new BoundedContainer<Integer>() {
            private Integer value = Integer.valueOf(42);

            Integer getValue() {
                return value;
            }

            double asDouble() {
                return value.doubleValue();
            }
        };
        test("3.1 Bounded generic getValue",
             intBounded.getValue().intValue() == 42);
        test("3.2 Bounded generic asDouble",
             intBounded.asDouble() == 42.0);

        // Test 2: Different Number subtype
        BoundedContainer<Double> doubleBounded = new BoundedContainer<Double>() {
            private Double value = Double.valueOf(3.14);

            Double getValue() {
                return value;
            }

            double asDouble() {
                return value.doubleValue();
            }
        };
        test("3.3 Bounded generic (Double)",
             doubleBounded.asDouble() > 3.13 && doubleBounded.asDouble() < 3.15);
    }

    static void testAnonymousExtendingGenericClass() {
        System.out.println("\n--- Anonymous Extending Generic Class ---");

        // Test 1: Anonymous subclass of Wrapper<String>
        Wrapper<String> customWrapper = new Wrapper<String>("base") {
            public String get() {
                return "custom:" + super.get();
            }
        };
        test("4.1 Anonymous extending generic class",
             customWrapper.get().equals("custom:base"));

        // Test 2: Anonymous subclass of Wrapper<Integer>
        // Note: super.get() returns Object (erased type), so we need to cast
        Wrapper<Integer> intWrapper = new Wrapper<Integer>(Integer.valueOf(10)) {
            public Integer get() {
                return Integer.valueOf(((Integer)super.get()).intValue() + 5);
            }
        };
        test("4.2 Anonymous generic class (Integer)",
             intWrapper.get().intValue() == 15);
    }

    static void testCapturingWithGenerics() {
        System.out.println("\n--- Capturing Variables with Generics ---");

        // Test 1: Capture local variable in generic anonymous class
        final String prefix = "PREFIX:";
        Processor<String> prefixProcessor = new Processor<String>() {
            public String process(String input) {
                return prefix + input;
            }
            public String getTypeName() {
                return "String";
            }
        };
        test("5.1 Capture local in generic anonymous",
             prefixProcessor.process("test").equals("PREFIX:test"));

        // Test 2: Capture multiple locals
        final int multiplier = 3;
        final int offset = 10;
        Processor<Integer> mathProcessor = new Processor<Integer>() {
            public Integer process(Integer input) {
                return Integer.valueOf(input.intValue() * multiplier + offset);
            }
            public String getTypeName() {
                return "Integer";
            }
        };
        test("5.2 Capture multiple locals in generic",
             mathProcessor.process(Integer.valueOf(5)).intValue() == 25);
    }

    static void testNestedGenericAnonymous() {
        System.out.println("\n--- Nested Generic Anonymous Classes ---");

        // Test 1: Generic anonymous class creating another generic anonymous class
        Processor<String> outer = new Processor<String>() {
            public String process(String input) {
                Processor<String> inner = new Processor<String>() {
                    public String process(String s) {
                        return "[" + s + "]";
                    }
                    public String getTypeName() {
                        return "String";
                    }
                };
                return "outer(" + inner.process(input) + ")";
            }
            public String getTypeName() {
                return "String";
            }
        };
        test("6.1 Nested generic anonymous classes",
             outer.process("x").equals("outer([x])"));
    }

    static void testPolymorphicUseOfGenericAnonymous() {
        System.out.println("\n--- Polymorphic Use of Generic Anonymous ---");

        // Test 1: Use anonymous generic through base interface
        Processor<String> proc = new Processor<String>() {
            public String process(String input) {
                return input.toLowerCase();
            }
            public String getTypeName() {
                return "String";
            }
        };

        // Pass to method expecting Processor<String>
        String result = applyProcessor(proc, "HELLO");
        test("7.1 Polymorphic use of generic anonymous", result.equals("hello"));
    }

    static String applyProcessor(Processor<String> p, String value) {
        return p.process(value);
    }

    public static void main(String[] args) {
        System.out.println("=== Anonymous Generic Test (Type Tags) ===");

        testAnonymousGenericInterface();
        testAnonymousGenericAbstractClass();
        testAnonymousBoundedGeneric();
        testAnonymousExtendingGenericClass();
        testCapturingWithGenerics();
        testNestedGenericAnonymous();
        testPolymorphicUseOfGenericAnonymous();

        System.out.println("\n=== RESULTS ===");
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);

        if (failed > 0) {
            System.exit(1);
        }
    }
}
