// Comprehensive test for enhanced for-loop with boxing, unboxing, and widening conversions
// Based on JDK7 test tools/javac/boxing/BoxedForeach.java
import java.util.*;

public class BoxedForeachTest {
    static int errors = 0;

    // ========================================
    // 1. Array tests with boxing/unboxing
    // ========================================

    // foreach over Integer[] with int loop variable (unboxing)
    static void testIntegerArrayToInt(Integer[] a) {
        int sum = 0;
        for (int i : a) {
            sum += i;
        }
        check("IntegerArrayToInt", sum, 6);
    }

    // foreach over int[] with Integer loop variable (boxing)
    static void testIntArrayToInteger(int[] a) {
        int sum = 0;
        for (Integer i : a) {
            sum += i;
        }
        check("IntArrayToInteger", sum, 6);
    }

    // foreach over Long[] with long loop variable (unboxing)
    static void testLongArrayToLong(Long[] a) {
        long sum = 0;
        for (long l : a) {
            sum += l;
        }
        check("LongArrayToLong", sum, 6L);
    }

    // foreach over Double[] with double loop variable (unboxing)
    static void testDoubleArrayToDouble(Double[] a) {
        double sum = 0;
        for (double d : a) {
            sum += d;
        }
        check("DoubleArrayToDouble", sum, 6.0);
    }

    // foreach over Boolean[] with boolean loop variable (unboxing)
    static void testBooleanArrayToBoolean(Boolean[] a) {
        int trueCount = 0;
        for (boolean b : a) {
            if (b) trueCount++;
        }
        check("BooleanArrayToBoolean", trueCount, 2);
    }

    // foreach over Character[] with char loop variable (unboxing)
    static void testCharacterArrayToChar(Character[] a) {
        int sum = 0;
        for (char c : a) {
            sum += c;
        }
        check("CharacterArrayToChar", sum, (int)'A' + (int)'B' + (int)'C');
    }

    // ========================================
    // 2. Iterable tests with parameterized types
    // ========================================

    // Basic Iterable<Integer> with Integer loop variable
    static void testIterableInteger(Iterable<Integer> b) {
        int sum = 0;
        for (Integer i : b) {
            sum += i;
        }
        check("IterableInteger", sum, 6);
    }

    // Iterable<Integer> with int loop variable (unboxing)
    static void testIterableIntegerToInt(Iterable<Integer> b) {
        int sum = 0;
        for (int i : b) {
            sum += i;
        }
        check("IterableIntegerToInt", sum, 6);
    }

    // Iterable<Integer> with float loop variable (unboxing + widening)
    static void testIterableIntegerToFloat(Iterable<Integer> b) {
        float sum = 0;
        for (float f : b) {
            sum += f;
        }
        check("IterableIntegerToFloat", (int)sum, 6);
    }

    // Iterable<Integer> with double loop variable (unboxing + widening)
    static void testIterableIntegerToDouble(Iterable<Integer> b) {
        double sum = 0;
        for (double d : b) {
            sum += d;
        }
        check("IterableIntegerToDouble", (int)sum, 6);
    }

    // Iterable<Integer> with long loop variable (unboxing + widening)
    static void testIterableIntegerToLong(Iterable<Integer> b) {
        long sum = 0;
        for (long l : b) {
            sum += l;
        }
        check("IterableIntegerToLong", sum, 6L);
    }

    // ========================================
    // 3. List tests (multi-level inheritance: List -> Collection -> Iterable)
    // ========================================

    // List<String> with String loop variable
    static void testListString(List<String> list) {
        StringBuilder sb = new StringBuilder();
        for (String s : list) {
            sb.append(s);
        }
        check("ListString", sb.toString(), "abc");
    }

    // List<Integer> with int loop variable
    static void testListIntegerToInt(List<Integer> list) {
        int sum = 0;
        for (int i : list) {
            sum += i;
        }
        check("ListIntegerToInt", sum, 6);
    }

    // ArrayList<Double> with double loop variable
    static void testArrayListDoubleToDouble(ArrayList<Double> list) {
        double sum = 0;
        for (double d : list) {
            sum += d;
        }
        check("ArrayListDoubleToDouble", (int)sum, 6);
    }

    // ========================================
    // 4. Generic method tests with bounded type parameters
    // ========================================

    // Iterable<T> where T extends Integer
    static <T extends Integer> void testBoundedTypeParam(Iterable<T> b) {
        int sum = 0;
        for (int i : b) {
            sum += i;
        }
        check("BoundedTypeParam", sum, 6);
    }

    // Iterable<? extends Integer> (upper bounded wildcard)
    static void testUpperBoundedWildcard(Iterable<? extends Integer> b) {
        int sum = 0;
        for (int i : b) {
            sum += i;
        }
        check("UpperBoundedWildcard", sum, 6);
    }

    // ========================================
    // 5. Enum array test
    // ========================================

    enum Color { RED, GREEN, BLUE }

    static <E extends Enum<E>> void testEnumArray(E[] values) {
        int count = 0;
        for (E e : values) {
            count++;
        }
        check("EnumArray", count, 3);
    }

    // ========================================
    // 6. Nested foreach tests
    // ========================================

    static void testNestedForeach(List<String> outer) {
        int count = 0;
        for (String s1 : outer) {
            for (String s2 : outer) {
                count++;
            }
        }
        check("NestedForeach", count, 9); // 3 * 3
    }

    // ========================================
    // 7. Iterator with explicit call (for comparison)
    // ========================================

    static void testExplicitIterator(Iterable<Integer> b) {
        float sum = 0;
        for (Iterator<Integer> it = b.iterator(); it.hasNext(); ) {
            float f = it.next();  // unboxing + widening
            sum += f;
        }
        check("ExplicitIterator", (int)sum, 6);
    }

    // ========================================
    // Helper methods
    // ========================================

    static void check(String testName, int actual, int expected) {
        if (actual != expected) {
            System.out.println("FAIL: " + testName + " - expected " + expected + ", got " + actual);
            errors++;
        }
    }

    static void check(String testName, long actual, long expected) {
        if (actual != expected) {
            System.out.println("FAIL: " + testName + " - expected " + expected + ", got " + actual);
            errors++;
        }
    }

    static void check(String testName, double actual, double expected) {
        if (Math.abs(actual - expected) > 0.001) {
            System.out.println("FAIL: " + testName + " - expected " + expected + ", got " + actual);
            errors++;
        }
    }

    static void check(String testName, String actual, String expected) {
        if (!actual.equals(expected)) {
            System.out.println("FAIL: " + testName + " - expected " + expected + ", got " + actual);
            errors++;
        }
    }

    public static void main(String[] args) {
        // Array tests
        Integer[] integerArray = { Integer.valueOf(1), Integer.valueOf(2), Integer.valueOf(3) };
        int[] intArray = { 1, 2, 3 };
        Long[] longArray = { Long.valueOf(1), Long.valueOf(2), Long.valueOf(3) };
        Double[] doubleArray = { Double.valueOf(1.0), Double.valueOf(2.0), Double.valueOf(3.0) };
        Boolean[] boolArray = { Boolean.TRUE, Boolean.FALSE, Boolean.TRUE };
        Character[] charArray = { Character.valueOf('A'), Character.valueOf('B'), Character.valueOf('C') };

        testIntegerArrayToInt(integerArray);
        testIntArrayToInteger(intArray);
        testLongArrayToLong(longArray);
        testDoubleArrayToDouble(doubleArray);
        testBooleanArrayToBoolean(boolArray);
        testCharacterArrayToChar(charArray);

        // Iterable tests
        List<Integer> intList = Arrays.asList(integerArray);
        testIterableInteger(intList);
        testIterableIntegerToInt(intList);
        testIterableIntegerToFloat(intList);
        testIterableIntegerToDouble(intList);
        testIterableIntegerToLong(intList);

        // List tests
        List<String> stringList = new ArrayList<String>();
        stringList.add("a");
        stringList.add("b");
        stringList.add("c");
        testListString(stringList);
        testListIntegerToInt(intList);

        ArrayList<Double> doubleList = new ArrayList<Double>();
        doubleList.add(Double.valueOf(1.0));
        doubleList.add(Double.valueOf(2.0));
        doubleList.add(Double.valueOf(3.0));
        testArrayListDoubleToDouble(doubleList);

        // Bounded type parameter tests
        testBoundedTypeParam(intList);
        testUpperBoundedWildcard(intList);

        // Enum test
        testEnumArray(Color.values());

        // Nested foreach
        testNestedForeach(stringList);

        // Explicit iterator
        testExplicitIterator(intList);

        // Report results
        if (errors > 0) {
            throw new RuntimeException(errors + " test(s) failed");
        }
    }
}
