package java.net;
public class URISyntaxException extends Exception {
    public URISyntaxException(String input, String reason, int index) { super(input + reason + index); }
    public URISyntaxException(String input, String reason) { super(input + reason); }
}