// Test Java 6 bytecode generation and parameter names
public class SimpleJava6Test {
    // This method's parameters should have names in LocalVariableTable
    public static int calculate(int firstNumber, int secondNumber, int thirdNumber) {
        int result = firstNumber + secondNumber + thirdNumber;
        return result;
    }

    public static void main(String[] args) {
        int value = calculate(10, 20, 30);
        System.out.println(value);
        System.out.println("Java 6 test passed");
    }
}
