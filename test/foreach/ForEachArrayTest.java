// Test enhanced for-loop with arrays

public class ForEachArrayTest {
    public void testIntArray() {
        int[] numbers = {1, 2, 3, 4, 5};
        int sum = 0;

        for (int num : numbers) {
            sum = sum + num;
        }
    }

    public void testStringArray() {
        String[] names = {"Alice", "Bob", "Charlie"};

        for (String name : names) {
            // Process name
        }
    }

    public void testObjectArray() {
        Object[] objects = new Object[3];

        for (Object obj : objects) {
            // Process object
        }
    }

    public void testNestedLoop() {
        int[][] matrix = {{1, 2}, {3, 4}};

        for (int[] row : matrix) {
            for (int value : row) {
                // Process value
            }
        }
    }

    public void testBreakContinue() {
        int[] numbers = {1, 2, 3, 4, 5};

        for (int num : numbers) {
            if (num == 3) {
                continue;
            }
            if (num == 4) {
                break;
            }
        }
    }
}
