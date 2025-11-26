// Test enhanced for-loop with arrays

public class ForEachArrayTest {
    public static int testIntArray() {
        int[] numbers = {1, 2, 3, 4, 5};
        int sum = 0;

        for (int num : numbers) {
            sum = sum + num;
        }
        return sum;
    }

    public static int testStringArray() {
        String[] names = {"Alice", "Bob", "Charlie"};
        int count = 0;

        for (String name : names) {
            count++;
        }
        return count;
    }

    public static int testNestedLoop() {
        int[][] matrix = {{1, 2}, {3, 4}};
        int sum = 0;

        for (int[] row : matrix) {
            for (int value : row) {
                sum = sum + value;
            }
        }
        return sum;
    }

    public static int testBreakContinue() {
        int[] numbers = {1, 2, 3, 4, 5};
        int sum = 0;

        for (int num : numbers) {
            if (num == 3) {
                continue;
            }
            if (num == 5) {
                break;
            }
            sum = sum + num;
        }
        return sum;  // Should be 1+2+4 = 7
    }

    public static void main(String[] args) {
        if (testIntArray() != 15) {
            System.out.println("FAIL: testIntArray");
            System.exit(1);
        }

        if (testStringArray() != 3) {
            System.out.println("FAIL: testStringArray");
            System.exit(1);
        }

        if (testNestedLoop() != 10) {
            System.out.println("FAIL: testNestedLoop");
            System.exit(1);
        }

        if (testBreakContinue() != 7) {
            System.out.println("FAIL: testBreakContinue");
            System.exit(1);
        }

        System.out.println("PASS: ForEachArrayTest passed");
    }
}
