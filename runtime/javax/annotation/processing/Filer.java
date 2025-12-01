package javax.annotation.processing;

import java.io.IOException;
import javax.lang.model.element.Element;
import javax.tools.FileObject;
import javax.tools.JavaFileManager;
import javax.tools.JavaFileObject;

public interface Filer {
    JavaFileObject createSourceFile(CharSequence name, Element... originatingElements) throws IOException;
    JavaFileObject createClassFile(CharSequence name, Element... originatingElements) throws IOException;
    FileObject createResource(JavaFileManager.Location location, CharSequence pkg, CharSequence relativeName, Element... originatingElements) throws IOException;
    FileObject getResource(JavaFileManager.Location location, CharSequence pkg, CharSequence relativeName) throws IOException;
}
