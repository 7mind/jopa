public enum EnumForwardRef {
    UP, DOWN, CEILING, FLOOR;

    public static EnumForwardRef fromInt(int value) {
        switch (value) {
            case 0: return UP;
            case 1: return DOWN;
            case 2: return CEILING;
            case 3: return FLOOR;
            default: return UP;
        }
    }

    public static void main(String[] args) {
        EnumForwardRef val = fromInt(2);
        if (val == CEILING) {
            System.out.println("EnumForwardRef: PASS");
        } else {
            System.out.println("EnumForwardRef: FAIL");
        }
    }
}
