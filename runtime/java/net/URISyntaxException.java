package java.net;

public class URISyntaxException extends Exception {
    public URISyntaxException(String input, String reason) {
        super(reason);
    }

    public URISyntaxException(String input, String reason, int index) {
        super(reason);
    }

    public String getInput() {
        return null;
    }

    public String getReason() {
        return null;
    }

    public int getIndex() {
        return 0;
    }
}
