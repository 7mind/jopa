// Test wildcard static imports
import static util.MathUtils.*;

public class StaticWildcardImportTest {
    public static void main(String[] args) {
        double pi = PI;
        int maxVal = MAX_VALUE;
        int result = max(5, 10);

        System.out.println("Testing wildcard static imports");
        if (pi > 3.0 && maxVal == 100 && result == 10) {
            System.out.println("Wildcard static imports work!");
        }
    }
}
