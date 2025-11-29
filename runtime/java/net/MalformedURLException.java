package java.net;

import java.io.IOException;

public class MalformedURLException extends IOException {
    public MalformedURLException() {
        super();
    }

    public MalformedURLException(String msg) {
        super(msg);
    }
}
