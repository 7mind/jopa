package javax.tools;

import java.io.InputStream;
import java.io.OutputStream;
import java.util.Set;

public interface Tool {
    int run(InputStream in, OutputStream out, OutputStream err, String... arguments);
    Set<SourceVersion> getSourceVersions();
}
