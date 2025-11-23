// Test static method imports
import static util.MathUtils.max;
import static util.MathUtils.min;

public class StaticMethodImportTest {
    public static void main(String[] args) {
        int a = 5;
        int b = 10;
        int maximum = max(a, b);
        int minimum = min(a, b);

        System.out.println("Testing static method imports");
        if (maximum == 10 && minimum == 5) {
            System.out.println("Static method imports work!");
        }
    }
}
