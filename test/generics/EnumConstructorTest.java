public enum EnumConstructorTest {
    RED(0xFF0000), GREEN(0x00FF00), BLUE(0x0000FF);

    private int rgbValue;

    private EnumConstructorTest(int rgb) {
        this.rgbValue = rgb;
    }

    public int getRgb() {
        return rgbValue;
    }

    public static void main(String[] args) {
        if (RED.ordinal() == 0 && GREEN.ordinal() == 1 && BLUE.ordinal() == 2 &&
            RED.getRgb() == 0xFF0000 && GREEN.getRgb() == 0x00FF00) {
            System.out.println("EnumConstructorTest: PASS");
        } else {
            System.out.println("EnumConstructorTest: FAIL");
        }
    }
}
