// Simple string switch test
public class SimpleStringSwitch {
    public static void main(String[] args) {
        String s = "hello";
        switch (s) {
            case "hello":
                System.out.println("Hello!");
                break;
            default:
                System.out.println("Unknown");
        }
    }
}
