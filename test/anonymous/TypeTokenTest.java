// Test type tokens (Gafter's pattern) - using anonymous subclasses to capture
// generic type information at runtime via reflection

import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;

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

    public static void main(String[] args) {
        System.out.println("=== Type Token Test (Gafter's Pattern) ===");

        testSimpleTypeToken();
        testParameterizedTypeToken();
        testNestedParameterizedType();
        testTypeReference();
        testArrayTypeToken();
        testMultipleInstances();
        testGenericSuperclassChain();

        System.out.println("\n=== RESULTS ===");
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);

        if (failed > 0) {
            System.exit(1);
        }
    }
}
