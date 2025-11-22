// Test static imports
import static java.lang.Math.PI;
import static java.lang.Math.sqrt;
import static java.lang.System.out;

public class StaticImportTest {
    public void testStaticImports() {
        double radius = 5.0;
        double area = PI * radius * radius;

        double value = sqrt(16.0);

        out.println("Test");
    }
}
