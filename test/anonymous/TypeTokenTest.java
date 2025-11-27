// Test type tokens (Gafter's pattern) - using anonymous subclasses to capture
// generic type information at runtime via reflection

import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;

// Helper generic classes for complex nested type tests
class Pair<A, B> {
    public A first;
    public B second;
    public Pair(A a, B b) { first = a; second = b; }
}

class Triple<A, B, C> {
    public A first;
    public B second;
    public C third;
    public Triple(A a, B b, C c) { first = a; second = b; third = c; }
}

class MyObj {
    public String value;
    public MyObj(String v) { value = v; }
}

class MyGeneric<T> {
    public T item;
    public MyGeneric(T t) { item = t; }
}

class Container<T> {
    public java.util.List<T> items;
    public Container() { items = null; }
}

abstract class TypeToken<T> {
    private final Type type;

    protected TypeToken() {
        Type superclass = getClass().getGenericSuperclass();
        if (!(superclass instanceof ParameterizedType)) {
            throw new RuntimeException("Missing type parameter");
        }
        this.type = ((ParameterizedType) superclass).getActualTypeArguments()[0];
    }

    public Type getType() {
        return type;
    }

    public Class<?> getRawType() {
        if (type instanceof Class) {
            return (Class<?>) type;
        } else if (type instanceof ParameterizedType) {
            return (Class<?>) ((ParameterizedType) type).getRawType();
        }
        return Object.class;
    }
}

abstract class TypeReference<T> {
    protected TypeReference() {}

    public Type getType() {
        Type superclass = getClass().getGenericSuperclass();
        return ((ParameterizedType) superclass).getActualTypeArguments()[0];
    }
}

public class TypeTokenTest {
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

    static void testSimpleTypeToken() {
        System.out.println("\n--- Simple Type Token ---");

        // Test 1: Capture String type
        TypeToken<String> stringToken = new TypeToken<String>() {};
        Type stringType = stringToken.getType();
        test("1.1 String type token captures type",
             stringType == String.class);
        test("1.2 String type token getRawType",
             stringToken.getRawType() == String.class);

        // Test 2: Capture Integer type
        TypeToken<Integer> intToken = new TypeToken<Integer>() {};
        test("1.3 Integer type token captures type",
             intToken.getType() == Integer.class);
        test("1.4 Integer type token getRawType",
             intToken.getRawType() == Integer.class);

        // Test 3: Different tokens for different types
        test("1.5 Different types produce different tokens",
             !stringToken.getType().equals(intToken.getType()));
    }

    static void testParameterizedTypeToken() {
        System.out.println("\n--- Parameterized Type Token ---");

        // Test 1: Capture List<String> type
        TypeToken<java.util.List<String>> listStringToken =
            new TypeToken<java.util.List<String>>() {};
        Type listStringType = listStringToken.getType();

        test("2.1 List<String> is ParameterizedType",
             listStringType instanceof ParameterizedType);

        if (listStringType instanceof ParameterizedType) {
            ParameterizedType pt = (ParameterizedType) listStringType;
            test("2.2 List<String> raw type is List",
                 pt.getRawType() == java.util.List.class);
            test("2.3 List<String> type argument is String",
                 pt.getActualTypeArguments()[0] == String.class);
        }

        // Test 2: Capture Map<String, Integer> type
        TypeToken<java.util.Map<String, Integer>> mapToken =
            new TypeToken<java.util.Map<String, Integer>>() {};
        Type mapType = mapToken.getType();

        test("2.4 Map<String,Integer> is ParameterizedType",
             mapType instanceof ParameterizedType);

        if (mapType instanceof ParameterizedType) {
            ParameterizedType pt = (ParameterizedType) mapType;
            Type[] args = pt.getActualTypeArguments();
            test("2.5 Map has two type arguments",
                 args.length == 2);
            test("2.6 Map first type arg is String",
                 args[0] == String.class);
            test("2.7 Map second type arg is Integer",
                 args[1] == Integer.class);
        }
    }

    static void testNestedParameterizedType() {
        System.out.println("\n--- Nested Parameterized Type ---");

        // Test: Capture List<List<String>> type
        TypeToken<java.util.List<java.util.List<String>>> nestedToken =
            new TypeToken<java.util.List<java.util.List<String>>>() {};
        Type nestedType = nestedToken.getType();

        test("3.1 Nested generic is ParameterizedType",
             nestedType instanceof ParameterizedType);

        if (nestedType instanceof ParameterizedType) {
            ParameterizedType outer = (ParameterizedType) nestedType;
            test("3.2 Outer raw type is List",
                 outer.getRawType() == java.util.List.class);

            Type innerType = outer.getActualTypeArguments()[0];
            test("3.3 Inner type is ParameterizedType",
                 innerType instanceof ParameterizedType);

            if (innerType instanceof ParameterizedType) {
                ParameterizedType inner = (ParameterizedType) innerType;
                test("3.4 Inner raw type is List",
                     inner.getRawType() == java.util.List.class);
                test("3.5 Innermost type is String",
                     inner.getActualTypeArguments()[0] == String.class);
            }
        }
    }

    static void testTypeReference() {
        System.out.println("\n--- TypeReference Pattern ---");

        // Alternative pattern used by Jackson and others
        TypeReference<String> stringRef = new TypeReference<String>() {};
        test("4.1 TypeReference captures String",
             stringRef.getType() == String.class);

        TypeReference<java.util.List<Integer>> listRef =
            new TypeReference<java.util.List<Integer>>() {};
        Type listType = listRef.getType();
        test("4.2 TypeReference captures List<Integer>",
             listType instanceof ParameterizedType);

        if (listType instanceof ParameterizedType) {
            ParameterizedType pt = (ParameterizedType) listType;
            test("4.3 TypeReference List raw type",
                 pt.getRawType() == java.util.List.class);
            test("4.4 TypeReference List type arg",
                 pt.getActualTypeArguments()[0] == Integer.class);
        }
    }

    static void testArrayTypeToken() {
        System.out.println("\n--- Array Type Token ---");

        // Test: Capture String[] type
        TypeToken<String[]> arrayToken = new TypeToken<String[]>() {};
        Type arrayType = arrayToken.getType();

        test("5.1 String[] type token",
             arrayType instanceof Class);

        if (arrayType instanceof Class) {
            Class<?> arrayClass = (Class<?>) arrayType;
            test("5.2 String[] is array",
                 arrayClass.isArray());
            test("5.3 String[] component type is String",
                 arrayClass.getComponentType() == String.class);
        }

        // Test: Capture int[] type (primitive array)
        TypeToken<int[]> intArrayToken = new TypeToken<int[]>() {};
        Type intArrayType = intArrayToken.getType();

        if (intArrayType instanceof Class) {
            Class<?> intArrayClass = (Class<?>) intArrayType;
            test("5.4 int[] is array",
                 intArrayClass.isArray());
            test("5.5 int[] component type is int",
                 intArrayClass.getComponentType() == int.class);
        }
    }

    static void testMultipleInstances() {
        System.out.println("\n--- Multiple Instances Same Type ---");

        // Verify that multiple anonymous instances with same type parameter
        // produce equivalent types
        TypeToken<String> token1 = new TypeToken<String>() {};
        TypeToken<String> token2 = new TypeToken<String>() {};

        test("6.1 Same type parameter produces equal types",
             token1.getType().equals(token2.getType()));

        // But they are different anonymous class instances
        test("6.2 Different anonymous class instances",
             token1.getClass() != token2.getClass());

        // Both have same raw type
        test("6.3 Same raw type",
             token1.getRawType() == token2.getRawType());
    }

    static void testGenericSuperclassChain() {
        System.out.println("\n--- Generic Superclass Chain ---");

        // Test that getGenericSuperclass returns ParameterizedType
        TypeToken<Double> token = new TypeToken<Double>() {};
        Type superclass = token.getClass().getGenericSuperclass();

        test("7.1 Superclass is ParameterizedType",
             superclass instanceof ParameterizedType);

        if (superclass instanceof ParameterizedType) {
            ParameterizedType pt = (ParameterizedType) superclass;
            test("7.2 Superclass raw type is TypeToken",
                 pt.getRawType() == TypeToken.class);
            test("7.3 Superclass type argument is Double",
                 pt.getActualTypeArguments()[0] == Double.class);
        }
    }

    static void testComplexNestedGenerics() {
        System.out.println("\n--- Complex Nested Generics ---");

        // Test 1: Pair<List<String>, List<Integer>>
        TypeToken<Pair<java.util.List<String>, java.util.List<Integer>>> pairListsToken =
            new TypeToken<Pair<java.util.List<String>, java.util.List<Integer>>>() {};
        Type pairListsType = pairListsToken.getType();

        test("8.1 Pair<List<String>, List<Integer>> is ParameterizedType",
             pairListsType instanceof ParameterizedType);

        if (pairListsType instanceof ParameterizedType) {
            ParameterizedType pt = (ParameterizedType) pairListsType;
            test("8.2 Raw type is Pair",
                 pt.getRawType() == Pair.class);

            Type[] args = pt.getActualTypeArguments();
            test("8.3 Pair has 2 type arguments", args.length == 2);

            // Check first arg: List<String>
            test("8.4 First arg is ParameterizedType",
                 args[0] instanceof ParameterizedType);
            if (args[0] instanceof ParameterizedType) {
                ParameterizedType listString = (ParameterizedType) args[0];
                test("8.5 First arg raw type is List",
                     listString.getRawType() == java.util.List.class);
                test("8.6 First arg type param is String",
                     listString.getActualTypeArguments()[0] == String.class);
            }

            // Check second arg: List<Integer>
            test("8.7 Second arg is ParameterizedType",
                 args[1] instanceof ParameterizedType);
            if (args[1] instanceof ParameterizedType) {
                ParameterizedType listInt = (ParameterizedType) args[1];
                test("8.8 Second arg raw type is List",
                     listInt.getRawType() == java.util.List.class);
                test("8.9 Second arg type param is Integer",
                     listInt.getActualTypeArguments()[0] == Integer.class);
            }
        }

        // Test 2: Pair<Pair<List<MyObj>, List<Integer>>, MyGeneric<MyObj>>
        TypeToken<Pair<Pair<java.util.List<MyObj>, java.util.List<Integer>>, MyGeneric<MyObj>>> complexToken =
            new TypeToken<Pair<Pair<java.util.List<MyObj>, java.util.List<Integer>>, MyGeneric<MyObj>>>() {};
        Type complexType = complexToken.getType();

        test("8.10 Complex nested type is ParameterizedType",
             complexType instanceof ParameterizedType);

        if (complexType instanceof ParameterizedType) {
            ParameterizedType outerPair = (ParameterizedType) complexType;
            test("8.11 Outer raw type is Pair",
                 outerPair.getRawType() == Pair.class);

            Type[] outerArgs = outerPair.getActualTypeArguments();
            test("8.12 Outer Pair has 2 args", outerArgs.length == 2);

            // First arg: Pair<List<MyObj>, List<Integer>>
            test("8.13 First outer arg is ParameterizedType",
                 outerArgs[0] instanceof ParameterizedType);
            if (outerArgs[0] instanceof ParameterizedType) {
                ParameterizedType innerPair = (ParameterizedType) outerArgs[0];
                test("8.14 Inner pair raw type is Pair",
                     innerPair.getRawType() == Pair.class);

                Type[] innerArgs = innerPair.getActualTypeArguments();
                test("8.15 Inner Pair has 2 args", innerArgs.length == 2);

                // List<MyObj>
                if (innerArgs[0] instanceof ParameterizedType) {
                    ParameterizedType listMyObj = (ParameterizedType) innerArgs[0];
                    test("8.16 List<MyObj> raw type is List",
                         listMyObj.getRawType() == java.util.List.class);
                    test("8.17 List<MyObj> type param is MyObj",
                         listMyObj.getActualTypeArguments()[0] == MyObj.class);
                }

                // List<Integer>
                if (innerArgs[1] instanceof ParameterizedType) {
                    ParameterizedType listInt = (ParameterizedType) innerArgs[1];
                    test("8.18 List<Integer> raw type is List",
                         listInt.getRawType() == java.util.List.class);
                    test("8.19 List<Integer> type param is Integer",
                         listInt.getActualTypeArguments()[0] == Integer.class);
                }
            }

            // Second arg: MyGeneric<MyObj>
            test("8.20 Second outer arg is ParameterizedType",
                 outerArgs[1] instanceof ParameterizedType);
            if (outerArgs[1] instanceof ParameterizedType) {
                ParameterizedType myGeneric = (ParameterizedType) outerArgs[1];
                test("8.21 MyGeneric raw type is MyGeneric",
                     myGeneric.getRawType() == MyGeneric.class);
                test("8.22 MyGeneric type param is MyObj",
                     myGeneric.getActualTypeArguments()[0] == MyObj.class);
            }
        }
    }

    static void testTripleNestedGenerics() {
        System.out.println("\n--- Triple Nested Generics ---");

        // Test: Triple<Map<String, List<Integer>>, Set<MyObj>, Container<Pair<String, Integer>>>
        TypeToken<Triple<java.util.Map<String, java.util.List<Integer>>,
                         java.util.Set<MyObj>,
                         Container<Pair<String, Integer>>>> tripleToken =
            new TypeToken<Triple<java.util.Map<String, java.util.List<Integer>>,
                                 java.util.Set<MyObj>,
                                 Container<Pair<String, Integer>>>>() {};
        Type tripleType = tripleToken.getType();

        test("9.1 Triple type is ParameterizedType",
             tripleType instanceof ParameterizedType);

        if (tripleType instanceof ParameterizedType) {
            ParameterizedType pt = (ParameterizedType) tripleType;
            test("9.2 Raw type is Triple",
                 pt.getRawType() == Triple.class);

            Type[] args = pt.getActualTypeArguments();
            test("9.3 Triple has 3 type arguments", args.length == 3);

            // First arg: Map<String, List<Integer>>
            if (args[0] instanceof ParameterizedType) {
                ParameterizedType mapType = (ParameterizedType) args[0];
                test("9.4 First arg raw type is Map",
                     mapType.getRawType() == java.util.Map.class);

                Type[] mapArgs = mapType.getActualTypeArguments();
                test("9.5 Map first arg is String",
                     mapArgs[0] == String.class);

                if (mapArgs[1] instanceof ParameterizedType) {
                    ParameterizedType listInt = (ParameterizedType) mapArgs[1];
                    test("9.6 Map second arg is List<Integer>",
                         listInt.getRawType() == java.util.List.class &&
                         listInt.getActualTypeArguments()[0] == Integer.class);
                }
            }

            // Second arg: Set<MyObj>
            if (args[1] instanceof ParameterizedType) {
                ParameterizedType setType = (ParameterizedType) args[1];
                test("9.7 Second arg raw type is Set",
                     setType.getRawType() == java.util.Set.class);
                test("9.8 Set type param is MyObj",
                     setType.getActualTypeArguments()[0] == MyObj.class);
            }

            // Third arg: Container<Pair<String, Integer>>
            if (args[2] instanceof ParameterizedType) {
                ParameterizedType containerType = (ParameterizedType) args[2];
                test("9.9 Third arg raw type is Container",
                     containerType.getRawType() == Container.class);

                Type containerArg = containerType.getActualTypeArguments()[0];
                if (containerArg instanceof ParameterizedType) {
                    ParameterizedType pairType = (ParameterizedType) containerArg;
                    test("9.10 Container arg is Pair<String, Integer>",
                         pairType.getRawType() == Pair.class &&
                         pairType.getActualTypeArguments()[0] == String.class &&
                         pairType.getActualTypeArguments()[1] == Integer.class);
                }
            }
        }
    }

    static void testDeeplyNestedGenerics() {
        System.out.println("\n--- Deeply Nested Generics (4 levels) ---");

        // Test: List<Map<String, Pair<Set<Integer>, MyGeneric<List<MyObj>>>>>
        TypeToken<java.util.List<java.util.Map<String,
                    Pair<java.util.Set<Integer>,
                         MyGeneric<java.util.List<MyObj>>>>>> deepToken =
            new TypeToken<java.util.List<java.util.Map<String,
                            Pair<java.util.Set<Integer>,
                                 MyGeneric<java.util.List<MyObj>>>>>>() {};
        Type deepType = deepToken.getType();

        test("10.1 Deeply nested type is ParameterizedType",
             deepType instanceof ParameterizedType);

        if (deepType instanceof ParameterizedType) {
            // Level 1: List<...>
            ParameterizedType level1 = (ParameterizedType) deepType;
            test("10.2 Level 1 is List",
                 level1.getRawType() == java.util.List.class);

            Type level1Arg = level1.getActualTypeArguments()[0];
            if (level1Arg instanceof ParameterizedType) {
                // Level 2: Map<String, ...>
                ParameterizedType level2 = (ParameterizedType) level1Arg;
                test("10.3 Level 2 is Map",
                     level2.getRawType() == java.util.Map.class);
                test("10.4 Map key is String",
                     level2.getActualTypeArguments()[0] == String.class);

                Type level2Val = level2.getActualTypeArguments()[1];
                if (level2Val instanceof ParameterizedType) {
                    // Level 3: Pair<Set<Integer>, MyGeneric<List<MyObj>>>
                    ParameterizedType level3 = (ParameterizedType) level2Val;
                    test("10.5 Level 3 is Pair",
                         level3.getRawType() == Pair.class);

                    Type[] level3Args = level3.getActualTypeArguments();

                    // Pair first: Set<Integer>
                    if (level3Args[0] instanceof ParameterizedType) {
                        ParameterizedType setInt = (ParameterizedType) level3Args[0];
                        test("10.6 Pair first is Set<Integer>",
                             setInt.getRawType() == java.util.Set.class &&
                             setInt.getActualTypeArguments()[0] == Integer.class);
                    }

                    // Pair second: MyGeneric<List<MyObj>>
                    if (level3Args[1] instanceof ParameterizedType) {
                        ParameterizedType myGeneric = (ParameterizedType) level3Args[1];
                        test("10.7 Pair second is MyGeneric",
                             myGeneric.getRawType() == MyGeneric.class);

                        Type myGenericArg = myGeneric.getActualTypeArguments()[0];
                        if (myGenericArg instanceof ParameterizedType) {
                            // Level 4: List<MyObj>
                            ParameterizedType listMyObj = (ParameterizedType) myGenericArg;
                            test("10.8 MyGeneric contains List<MyObj>",
                                 listMyObj.getRawType() == java.util.List.class &&
                                 listMyObj.getActualTypeArguments()[0] == MyObj.class);
                        }
                    }
                }
            }
        }
    }

    public static void main(String[] args) {
        System.out.println("=== Type Token Test (Gafter's Pattern) ===");

        testSimpleTypeToken();
        testParameterizedTypeToken();
        testNestedParameterizedType();
        testTypeReference();
        testArrayTypeToken();
        testMultipleInstances();
        testGenericSuperclassChain();
        testComplexNestedGenerics();
        testTripleNestedGenerics();
        testDeeplyNestedGenerics();

        System.out.println("\n=== RESULTS ===");
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);

        if (failed > 0) {
            System.exit(1);
        }
    }
}
