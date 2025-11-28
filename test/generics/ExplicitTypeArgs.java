import java.util.Collections;
import java.util.Set;

public class ExplicitTypeArgs {
    public Set<String> getEmptySet() {
        return Collections.<String>emptySet();
    }

    public static void main(String[] args) {
        ExplicitTypeArgs test = new ExplicitTypeArgs();
        Set<String> set = test.getEmptySet();
        System.out.println("ExplicitTypeArgs: PASS");
    }
}
