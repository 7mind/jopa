// Test enhanced for-loop with runtime execution
public class ForEachTest {
    public static void main(String[] args) {
        // Test with int array
        int[] numbers = {1, 2, 3, 4, 5};
        int sum = 0;
        for (int num : numbers) {
            sum = sum + num;
        }
        if (sum == 15) {
            System.out.println("✓ Int array for-each works");
        } else {
            System.out.println("✗ FAIL: Int array for-each, sum=" + sum);
            System.exit(1);
        }

        // Test with String array
        String[] words = {"hello", "world"};
        int count = 0;
        for (String word : words) {
            count++;
        }
        if (count == 2) {
            System.out.println("✓ String array for-each works");
        } else {
            System.out.println("✗ FAIL: String array for-each");
            System.exit(1);
        }

        // Test with nested loops
        int[][] matrix = {{1, 2}, {3, 4}};
        int matrixSum = 0;
        for (int[] row : matrix) {
            for (int value : row) {
                matrixSum = matrixSum + value;
            }
        }
        if (matrixSum == 10) {
            System.out.println("✓ Nested for-each works");
        } else {
            System.out.println("✗ FAIL: Nested for-each");
            System.exit(1);
        }

        // Test break and continue
        int breakCount = 0;
        for (int num : numbers) {
            if (num == 3) {
                continue;
            }
            if (num == 4) {
                break;
            }
            breakCount++;
        }
        if (breakCount == 2) {
            System.out.println("✓ Break/continue in for-each works");
        } else {
            System.out.println("✗ FAIL: Break/continue in for-each");
            System.exit(1);
        }

        System.out.println("✓ All for-each tests passed!");
    }
}
