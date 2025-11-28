enum Color { RED, GREEN, BLUE }

public class EnumConstantsTest<T extends Enum<T>> {
    private Class<T> enumClass;

    EnumConstantsTest(Class<T> clazz) {
        this.enumClass = clazz;
    }

    // Test: field.method()[index] should return type T, not Object
    public T getConstant(int index) {
        return enumClass.getEnumConstants()[index];
    }

    public static void main(String[] args) {
        EnumConstantsTest<Color> test = new EnumConstantsTest<Color>(Color.class);
        Color c = test.getConstant(0);
        System.out.println("Got: " + c);
    }
}
