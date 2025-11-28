public class ArrayCloneTest {
    public static void main(String[] args) {
        // Test that array.clone() returns the array type, not Object
        String[] original = {"a", "b", "c"};
        String[] cloned = original.clone();

        int[] intArray = {1, 2, 3};
        int[] intCloned = intArray.clone();

        Object[] objArray = new Object[3];
        Object[] objCloned = objArray.clone();

        if (cloned.length == 3 && intCloned.length == 3) {
            System.out.println("ArrayCloneTest: PASS");
        } else {
            System.out.println("ArrayCloneTest: FAIL");
        }
    }
}
