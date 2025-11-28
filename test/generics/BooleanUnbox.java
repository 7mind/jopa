public class BooleanUnbox {
    Boolean getFlag() {
        return Boolean.TRUE;
    }

    void test() {
        if (getFlag()) {  // Should auto-unbox Boolean to boolean
            System.out.println("true");
        }

        Boolean b = Boolean.FALSE;
        while (b) {  // Should auto-unbox
            b = Boolean.FALSE;
        }
    }

    public static void main(String[] args) {
        BooleanUnbox test = new BooleanUnbox();
        if (test.getFlag()) {
            System.out.println("BooleanUnbox: PASS");
        } else {
            System.out.println("BooleanUnbox: FAIL");
        }
    }
}
