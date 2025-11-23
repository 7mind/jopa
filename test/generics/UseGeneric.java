public class UseGeneric {
    public void test() {
        SimpleGeneric<String> box = new SimpleGeneric<String>("hello");
        String value = box.getValue();
    }
}
