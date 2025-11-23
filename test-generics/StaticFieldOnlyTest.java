// Test only static field import
import static util.MathUtils.PI;

public class StaticFieldOnlyTest {
    public static void main(String[] args) {
        double pi = PI;
        System.out.println("Got PI value");
        if (pi > 3.0) {
            System.out.println("PI is valid");
        }
    }
}
